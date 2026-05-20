#ifndef NLP_OPTIMIZATION_NETWORK_H
#define NLP_OPTIMIZATION_NETWORK_H

#include <vector>

struct CapacityModule {
    double capacity;
    double cost;
};

struct Link {
    int id;
    int source;
    int target;
    std::vector<CapacityModule> modules;
};

struct Neighbor {
    int to_node;
    int link_id;
};

struct Path {
    std::vector<int> links_ids;
    double routing_cost;
};

struct Demand {
    int id;
    int source;
    int target;
    double volume;
    std::vector<Path> candidate_paths;
};

class Network {
public:
    int nodes_number = 0;
    std::vector<Link> links;
    std::vector<std::vector<Neighbor>> adj;
    std::vector<Demand> demands;
    std::vector<double> base_link_costs;

    void set_nodes_number(int n);
    void add_link(int id, int source, int target, const std::vector<CapacityModule>& modules);
    void add_demand(int id, int source, int target, double volume);
    Path run_dijkstra(int src, int dst, const std::vector<double>& current_weights) const;
    void generate_candidate_paths(int num_paths);
    double evaluate(const std::vector<std::vector<double>>& solution_flows) const;
    void display_data() const;
};

#endif //NLP_OPTIMIZATION_NETWORK_H