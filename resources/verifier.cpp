/***
 * Usage:
 * ./verifier <base dir> <application> <test_file>
 * 
 * Note:
 * 1. The application should be: pagerank, shortest-path
 * 2. Test file must be present in resources/graphs/
 * */


#include <bits/stdc++.h>

using namespace std;
#define INF 1e9

////////////////////////////////////// Graph Definition /////////////////////////////////////////////////


class Edge {
public:
    int src, dst;
    double weight;
    Edge(int src, int dst, double weight) {
        this->src = src;
        this->dst = dst;
        this->weight = weight;
    }
};

class Graph {
public:
    int num_vertices_;
    int num_edges_;
    vector<Edge> edges_;

    Graph(std::string graph_file, bool weighted_edges) {
        std::ifstream graph_file_stream(graph_file);
        if (!graph_file_stream.is_open()) {
            std::cerr << "Error opening file: " << graph_file << ". Exiting..." << std::endl;
            exit(1);
        }
        graph_file_stream >> num_vertices_;
        int src, dst;
        double weight = 1.0;

        while (graph_file_stream >> src >> dst) {
            if(weighted_edges) {
                graph_file_stream >> weight;
            }
            edges_.emplace_back(Edge(src, dst, weight));
        }

        num_edges_ = edges_.size();
    }

    // Get Adjacency List
    vector<vector<int>> getAdjacencyList() {
        vector<vector<int>> adj_list(num_vertices_);
        for (int i = 0; i < num_edges_; i++) {
            adj_list[edges_[i].src].push_back(edges_[i].dst);
        }
        return adj_list;
    }

    // Get Adjacency Matrix
    vector<vector<double>> getAdjacencyMatrix() {
        vector<vector<double>> adj_matrix(num_vertices_);
        for (int i = 0; i < num_vertices_; i++) {
            adj_matrix[i].resize(num_vertices_);
        }
        for (int i = 0; i < num_edges_; i++) {
            adj_matrix[edges_[i].src][edges_[i].dst] = edges_[i].weight;
        }
        return adj_matrix;
    }
};;

//////////////////////////////////////////////////////////////////////////////////////////////////////


void writeResults(vector<double> results, string output_filename){
    std::ofstream outfile;
    outfile.open(output_filename);
    printf("Writing results to %s\n", output_filename.c_str());
    for (int i = 0; i < results.size(); ++i)
        outfile << i << " " << results[i] << std::endl;
    outfile.close();
}
 

/////////////////////////////////////// Application Verifiers ///////////////////////////////////////

//  1. Shortest Path
vector<double> ShortestPath(Graph &graph, int src, int num_iterations)
{
    int V = graph.num_vertices_;
    int E = graph.num_edges_;
    vector<double> dist(V, INF);
    dist[src] = 0;
 
    for (int i = 1; i <= num_iterations; i++) {
        for (int j = 0; j < E; j++) {
            int u = graph.edges_[j].src;
            int v = graph.edges_[j].dst;
            double weight = graph.edges_[j].weight;
            if (dist[u] != INF && dist[u] + weight < dist[v])
                dist[v] = dist[u] + weight;
        }
    }
 
    return dist;
}


//  2. Page Rank
void PageRankHelper(Graph &graph, vector<double> &pagerank) {
	vector<double> pagerank_new(graph.num_vertices_, 0.0);
	
    auto adj_list = graph.getAdjacencyList();
	for(int node = 0;node < graph.num_vertices_; node++){
		double degree = adj_list[node].size();
		for(auto nbh: adj_list[node]){
			pagerank_new[nbh] += (pagerank[node] / degree);
		}
	}

	for(int node = 0;node < graph.num_vertices_; node++){
		pagerank[node] = 0.15 + 0.85 * pagerank_new[node];
	}
}

vector<double> PageRank(Graph &graph, int num_iterations) {
    vector<double> pagerank(graph.num_vertices_, 1.0);
    for(int i = 0; i < num_iterations; i++){
        PageRankHelper(graph, pagerank);
    }

    return pagerank;
}


int64_t findParent(int64_t node, vector<double> &parent){
    if(parent[node] < 0) return node;
    parent[node] = findParent(parent[node], parent);
    return parent[node];
}

void unionParent(int64_t node1, int64_t node2, vector<double> &parent){
    int64_t parent1 = findParent(node1, parent);
    int64_t parent2 = findParent(node2, parent);
    if(parent1 != parent2){
        if(llabs(parent[parent1]) >= llabs(parent[parent2])){
            parent[parent1] -= 1;
            parent[parent2] = parent1;
        } else if(llabs(parent[parent2]) > llabs(parent[parent1]) ){
            parent[parent2] -= 1;
            parent[parent1] = parent2;
        }
    }
}

// 3. Connected Components
vector<double> ConnectedComponents(Graph &graph) {
    vector<double> components(graph.num_vertices_, -1);
    // union for edges
    for(auto edge: graph.edges_) {
        unionParent(edge.src, edge.dst, components);
    }

    // change component for nodes to actual component
    for(int i = 0; i < graph.num_vertices_; i++){
        findParent(i, components);
    }

    for(int i = 0; i < graph.num_vertices_; i++){
        if(components[i] < 0) {
            components[i] = i;
        }
    }

    unordered_map<int64_t, int64_t> component_map;
    vector<double> components_new = components;
    for(int i = 0; i < graph.num_vertices_; i++){
        if(component_map.find(components[i]) == component_map.end()) {
            if(i < components[i]){
                component_map[components[i]] = i;
            } else {
                component_map[components[i]] = components[i];
            }
        }
    }
    for(int i = 0; i < graph.num_vertices_; i++){
        components_new[i] = component_map[components[i]];
    }

    return components_new;
}

// 4. Degree
vector<double> Degree(Graph& graph) {
    vector<double> degree(graph.num_vertices_, 0);
    for(auto edge: graph.edges_) {
        degree[edge.src] += 1;
        degree[edge.dst] += 1;
    }
    return degree;
}

int main(int argc, char* argv[])
{
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << "<base dir> <application> <test file> [optional args]" << std::endl;
        exit(1);
    }
    
    std::string base_dir = argv[1];
    std::string application = argv[2];
    std::string test_file = argv[3];

    std::string input_filepath = base_dir + "/graphs/" + test_file;
    std::string output_filepath = base_dir + "/" + application + "/expected_results/" + test_file;

    Graph graph(input_filepath, false);

    if(application == "shortest-path"){
        int num_iterations = 10;
        if(argc > 4) num_iterations = atoi(argv[4]);
        vector<double> results = ShortestPath(graph, 1, num_iterations);
        writeResults(results, output_filepath);
    }
    else if(application == "pagerank"){
        int num_iterations = 10;
        if(argc > 4) num_iterations = atoi(argv[4]);
        vector<double> results = PageRank(graph, num_iterations);
        writeResults(results, output_filepath);
    } 
    else if(application == "connected-comp"){
        vector<double> results = ConnectedComponents(graph);
        writeResults(results, output_filepath);
    } else if(application == "degree") {
        vector<double> results = Degree(graph);
        writeResults(results, output_filepath);
    }
    else{
        std::cerr << "Invalid application: " << application << std::endl;
        exit(1);
    }

    return 0;
}