#include "lemon_graph.h"


/*
GOALS:
-graph connectivity
-max weighed matching
minimum cost flow analysis
minimum Steiner trees
sub graph matching
min and max cuts
parallel algorithms

*/





Lemon::Lemon(std::vector<Vertex> adjacency_list, ListGraph *g) 
	: graph(g), weights(*g), n2idx(*g), e2n(*g) {

	for (auto v : adjacency_list) {
		ListGraph::Node n = this->graph->addNode();
		this->n2idx[n] = v.id;
		this->idx2n.insert(std::make_pair(v.id, n));
    }
	for (auto v : adjacency_list) {
		for (auto e : v.edges) {
			int e_i = v.id, e_j = e.second;
			float w = e.first;
			ListGraph::Edge E = this->graph->addEdge(idx2n[e_i], idx2n[e_j]);
			this->weights[E] = w;
			this->e2n[E] = std::make_pair(e_i, e_j);	
        }
	}

}

Lemon::~Lemon() {

}

void Lemon::test() {
        /*
	for (ListGraph::NodeIt n(*this->graph); n != INVALID; ++n)
	    std::cout << n2idx[n] << " ";
	std::cout << "\n";
	for (ListGraph::EdgeIt e(*this->graph); e != INVALID; ++e)
        		std::cout << weights[e] << " ";
        std::cout << "\n";
	for (auto n : this->n2idx)
	    std::cout << n << " ";
	std::cout << "\n";
	for (auto n : this->idx2n)
	    std::cout << n << " ";
	std::cout << "\n";

        for (ListDigraph::ArcIt a(*this->digraph); a != INVALID; ++a) {
            std::cout << "(" << this->digraph->id(this->digraph->source(a)) 
                      << "," << this->digraph->id(this->digraph->target(a))
                      << ") -  " << this->weightsDi[a] << std::endl;;
        }*/
}

/*
 *  Runs Dijkstra's algorithm in parallel on each 
 *  node to every other node and selects the node,
 *  that provides the total minimum distance, as
 *  the distribution center.
 *
 *  @param none
 *  @return none
 */
void Lemon::initDistributionCenter() {
    float cur, min = std::numeric_limits<float>::max();
    std::vector<std::future<float>> distances;

    for (unsigned int i = 0; i < this->idx2n.size(); ++i) 
        distances.push_back(std::async(std::launch::async, &Lemon::dijkstrasTotalMinDistance, this, std::ref(this->idx2n[i])));

    for (unsigned int i = 0; i < this->idx2n.size(); ++i) {
        // Thread waits for computation to end on call to get()
        if ((cur = distances[i].get()) <= min) {
            this->disCenter.first = i; 
            this->disCenter.second = this->idx2n[i];
            min = cur;
        }
    }
    
    std::cout << "Found DistributionCenter: " << disCenter.first 
              << " Total Distance: " << min <<  std::endl;
}

void Lemon::initDistributionCenterSeq() {
    float cur, min = std::numeric_limits<float>::max();

    for (auto n : this->idx2n) {
        if ((cur = dijkstrasTotalMinDistance(n.second)) <= min) {
            this->disCenter.first = n.first;
            this->disCenter.second = n.second;
            min = cur;
        }
    }
    std::cout << "Found DistributionCenter: " << disCenter.first 
              << " Total Distane: " << min << std::endl;
}

void Lemon::weightedMatching() {
	MaxWeightedMatching< ListGraph, ListGraph::EdgeMap<float> > 
		M(*(this->graph), this->weights);
	M.run();
	std::vector< std::pair<int, int> > results;
	for (ListGraph::NodeIt n(*(this->graph)); n != INVALID; ++n) {
		if (M.mate(n) == INVALID) continue;
		results.push_back(std::make_pair(this->n2idx[n], this->n2idx[M.mate(n)]));
	}
	std::cout << "Max weighted matching edge set:\n";
	for (auto r : results)
		std::cout << r.first << ":" << r.second << "\n";
}

void Lemon::weightedMatching(Lemon &L) {
	MaxWeightedMatching< ListGraph, ListGraph::EdgeMap<float> > 
		M(*(L.graph), L.weights);
	M.run();
	std::vector< std::pair<int, int> > results;
	for (ListGraph::NodeIt n(*(L.graph)); n != INVALID; ++n) {
		if (M.mate(n) == INVALID) continue;
		results.push_back(std::make_pair(L.getN2idx(n), L.getN2idx(M.mate(n))));
	}
	std::cout << "Max weighted matching edge set:\n";
	for (auto r : results)
		std::cout << r.first << ":" << r.second << "\n";
}

int Lemon::getN2idx(ListGraph::Node n){
	return n2idx[n];
}

void Lemon::minCost() {
    MinCostArborescence< ListGraph, ListGraph::EdgeMap<float >>
        M(*(this->graph), this->weights);

    M.init();
    M.addSource(disCenter.second);
    M.start();
    std::cout << "Running Min Cost Arborescence: "
              << M.arborescenceCost() << std::endl;
}

void Lemon::kruskalsTrim() {
    std::cout << "Running KruskalsTrim...\n";

    ListGraph::EdgeMap<bool> treeMap(*this->graph);

	std::cout << "\nWeight of the minimum spanning tree: " 
		<< kruskal(*this->graph, this->weights, treeMap) << "\n";

	for (ListGraph::EdgeIt e(*this->graph); e != INVALID; ++e) {
		if (treeMap[e]) {
           this->graph->erase(e); 
           this->weights[e] = std::numeric_limits<float>::max();
        }
	}
}

void Lemon::kruskalsMinSpanningTree() {
	ListGraph::EdgeMap<bool> treeMap(*this->graph);
	std::cout << "\nWeight of the minimum spanning tree: " 
		<< kruskal(*this->graph, this->weights, treeMap) << "\n";
	int k = 0;
	for (ListGraph::EdgeIt e(*this->graph); e != INVALID; ++e) {
		if (treeMap[e]) {
			// std::cout << this->graph->id(e) << "\n";
			//std::cout << e2n[e].first << ":" << e2n[e].second << "\n";
			++k;
		} 
	}
	std::cout << "\n";	
}

/**
 *  Determines the minimum total distance from the
 *  source node to every other node in the graph.
 *
 *  @param s            Potential facility node 
 *  @return             Total minimum distance to every node
 */
float Lemon::dijkstrasTotalMinDistance(ListGraph::Node &s) {
    float distance = 0;
    for (auto t : this->idx2n) {
        auto d = Dijkstra<ListGraph, ListGraph::EdgeMap<float> >(*(this->graph), this->weights);
        d.run(s);
        distance += d.dist(t.second);
    }
    //std::cout << this->n2idx[s] << " total min distance: " << distance << std::endl;
    return distance;
}

void Lemon::dijkstrasShortestPath() {

    std::cout << "Dijkstra Shortest Path" << std::endl;
        
    for (auto t : this->idx2n) {
        auto d = Dijkstra<ListGraph, ListGraph::EdgeMap<float> >(*(this->graph), this->weights);
        d.run(this->disCenter.second);

        std::cout << "The distance of node t from node s: "
                  << d.dist(t.second) << std::endl;

        std::cout << "Shortest path from " << this->graph->id(disCenter.second) << " to "
                  << this->graph->id(t.second)
                  << " goes through the following nodes: ";
        
        for (ListGraph::Node v = t.second; v != disCenter.second; v = d.predNode(v)) {
            std::cout << this->graph->id(v) << "->";
        }
        std::cout << this->graph->id(disCenter.second) << std::endl;
    }
    
}



// ListGraph *graph;
// ListGraph::EdgeMap<float> weights;
// ListGraph::NodeMap<int> n2idx;
// std::map<int, ListGraph::Node> idx2n;



// std::cout << v.id << " : ";
// for (auto e : v.edges) {
	// std::cout << e.first << "," << e.second << " ";
// }
// std::cout << std::endl;
