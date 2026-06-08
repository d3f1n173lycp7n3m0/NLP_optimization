#ifndef NLP_OPTIMIZATION_NETWORK_H
#define NLP_OPTIMIZATION_NETWORK_H

#include <vector>

struct CapacityModule
{
    double capacity;
    double cost;
};

struct Link
{
    int id;
    int source;
    int target;
    double routing_cost;
    std::vector<CapacityModule> modules;
};

struct Neighbor
{
    int to_node;
    int link_id;
};

struct Path
{
    std::vector<int> links_ids;
    double routing_cost;
};

struct Demand
{
    int id;
    int source;
    int target;
    double volume;
    std::vector<Path> candidate_paths;
};

class Network
{
public:
    enum class LinkModel
    {
        DIRECTED,
        UNDIRECTED,
        BIDIRECTED
    };

    enum class DemandModel
    {
        DIRECTED,
        UNDIRECTED
    };

    enum class LinkCapacityModel
    {
        LINEAR_LINK_CAPACITIES,
        SINGLE_MODULAR_CAPACITIES,
        MODULAR_LINK_CAPACITIES,
        EXPLICIT_LINK_CAPACITIES
    };

    enum class RoutingModel
    {
        CONTINUOUS,
        INTEGER,
        SINGLE_PATH
    };

    enum class AdmissiblePathModel
    {
        ALL_PATHS,
        EXPLICIT_LIST
    };

    enum class HopLimitModel
    {
        IGNORE_HOP_LIMITS,
        INDIVIDUAL_HOP_LIMITS
    };

    enum class SurvivabilityModel
    {
        NO_SURVIVABILITY,
        ONE_PLUS_ONE_PROTECTION,
        SHARED_PATH_PROTECTION,
        UNRESTRICTED_FLOW_RECONFIGURATION
    };

    int nodes_number = 0;
    std::vector<Link> links;
    std::vector<std::vector<Neighbor>> adjacency;
    std::vector<Demand> demands;
    std::vector<double> base_link_costs;
    bool has_routing_unit_data = false;
    double min_routing_unit = 0.0;
    double max_routing_unit = 0.0;
    LinkModel link_model = LinkModel::UNDIRECTED;
    DemandModel demand_model = DemandModel::UNDIRECTED;
    LinkCapacityModel capacity_model = LinkCapacityModel::LINEAR_LINK_CAPACITIES;
    RoutingModel routing_model = RoutingModel::CONTINUOUS;
    AdmissiblePathModel admissible_path_model = AdmissiblePathModel::ALL_PATHS;
    HopLimitModel hop_limit_model = HopLimitModel::IGNORE_HOP_LIMITS;
    SurvivabilityModel survivability_model = SurvivabilityModel::NO_SURVIVABILITY;
    bool node_hardware = false;
    bool fixed_charge = false;
    bool directed = false;
    bool use_lowest_unit_cost = false;
    bool use_highest_unit_cost = false;
    bool use_average_unit_cost = false;
    bool use_weighted_average_unit_cost = false;
    bool include_routing_cost = false;
    bool first_module_only = false;
    bool has_reference_cost = false;
    double demand_scale = 1.0;
    double cost_scale = 1.0;
    double reference_cost = 0.0;
    int candidate_paths = 0;
    int population_size = 0;
    int iterations = 0;
    int limit = 0;
    int unit_cost_module_index = 1;

    void set_nodes_number(int n);
    void set_link_model(LinkModel model);
    void set_demand_model(DemandModel model);
    void set_directed(bool is_directed);
    void set_use_lowest_unit_cost(bool enabled);
    void set_use_highest_unit_cost(bool enabled);
    void set_use_average_unit_cost(bool enabled);
    void set_use_weighted_average_unit_cost(bool enabled);
    void set_unit_cost_module_index(int module_index);
    void set_include_routing_cost(bool enabled);
    void add_link(int id, int source, int target, double routing_cost, const std::vector<CapacityModule> &modules);
    void add_demand(int id, int source, int target, double volume);
    void scale_demands(double factor);
    void scale_costs(double factor);
    void keep_only_first_capacity_module();
    void apply_smallest_batch_preprocessing();
    void set_link_capacity_model(LinkCapacityModel model);
    LinkCapacityModel get_link_capacity_model() const;
    void set_routing_model(RoutingModel model);
    RoutingModel get_routing_model() const;
    void set_admissible_path_model(AdmissiblePathModel model);
    void set_hop_limit_model(HopLimitModel model);
    void set_survivability_model(SurvivabilityModel model);
    void set_node_hardware(bool enabled);
    void set_fixed_charge(bool enabled);
    Path run_dijkstra(int src, int dst, const std::vector<double> &current_weights) const;
    void generate_candidate_paths(int num_paths);
    double evaluate(const std::vector<std::vector<double>> &solution_flows) const;
    void display_data() const;
    void print_basic_stats() const;
};

#endif // NLP_OPTIMIZATION_NETWORK_H
