#include "../include/Network.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <queue>
#include <random>
#include <thread>
#include <vector>

const double INFINITY_VALUE = std::numeric_limits<double>::infinity();

static const CapacityModule &select_unit_cost_module(
    const std::vector<CapacityModule> &modules,
    int unit_cost_module_index)
{
    static CapacityModule empty_module = {0.0, 1000000000000000000.0};

    if (modules.empty())
    {
        return empty_module;
    }

    int selected_index = unit_cost_module_index;

    if (selected_index < 0)
    {
        selected_index = 0;
    }

    if (selected_index >= static_cast<int>(modules.size()))
    {
        selected_index = static_cast<int>(modules.size()) - 1;
    }

    return modules[selected_index];
}

static double random_weight_noise(std::mt19937 &random_generator)
{
    return 0.5 + static_cast<double>(random_generator() % 1500000) / 1000000.0;
}

static double calculate_unit_cost(
    const std::vector<CapacityModule> &modules,
    int unit_cost_module_index)
{
    const CapacityModule &selected_module =
        select_unit_cost_module(modules, unit_cost_module_index);

    if (selected_module.capacity <= 0.0)
    {
        return selected_module.cost;
    }

    return selected_module.cost / selected_module.capacity;
}

static double calculate_lowest_unit_cost(
    const std::vector<CapacityModule> &modules)
{
    const double EPSILON = 1e-9;
    const double INFEASIBLE_COST = 1e18;
    double best_unit_cost = INFEASIBLE_COST;

    for (const auto &module : modules)
    {
        if (module.capacity > EPSILON && module.cost >= 0.0)
        {
            best_unit_cost = std::min(
                best_unit_cost,
                module.cost / module.capacity);
        }
    }

    return best_unit_cost;
}

static double calculate_highest_unit_cost(
    const std::vector<CapacityModule> &modules)
{
    const double EPSILON = 1e-9;
    const double INFEASIBLE_COST = 1e18;
    double highest_unit_cost = 0.0;

    for (const auto &module : modules)
    {
        if (module.capacity > EPSILON && module.cost >= 0.0)
        {
            highest_unit_cost = std::max(
                highest_unit_cost,
                module.cost / module.capacity);
        }
    }

    if (highest_unit_cost <= 0.0)
    {
        return INFEASIBLE_COST;
    }

    return highest_unit_cost;
}

static double calculate_average_unit_cost(
    const std::vector<CapacityModule> &modules)
{
    const double EPSILON = 1e-9;
    const double INFEASIBLE_COST = 1e18;
    double unit_cost_sum = 0.0;
    int unit_cost_count = 0;

    for (const auto &module : modules)
    {
        if (module.capacity > EPSILON && module.cost >= 0.0)
        {
            unit_cost_sum += module.cost / module.capacity;
            unit_cost_count++;
        }
    }

    if (unit_cost_count == 0)
    {
        return INFEASIBLE_COST;
    }

    return unit_cost_sum / unit_cost_count;
}

static double calculate_weighted_average_unit_cost(
    const std::vector<CapacityModule> &modules)
{
    const double EPSILON = 1e-9;
    const double INFEASIBLE_COST = 1e18;
    double cost_sum = 0.0;
    double capacity_sum = 0.0;

    for (const auto &module : modules)
    {
        if (module.capacity > EPSILON && module.cost >= 0.0)
        {
            cost_sum += module.cost;
            capacity_sum += module.capacity;
        }
    }

    if (capacity_sum <= EPSILON)
    {
        return INFEASIBLE_COST;
    }

    return cost_sum / capacity_sum;
}

static double calculate_linear_capacity_cost(
    double load,
    const std::vector<CapacityModule> &modules,
    int unit_cost_module_index,
    bool use_lowest_unit_cost,
    bool use_highest_unit_cost,
    bool use_average_unit_cost,
    bool use_weighted_average_unit_cost)
{
    const double EPSILON = 1e-9;
    const double INFEASIBLE_COST = 1e18;

    if (load <= EPSILON)
    {
        return 0.0;
    }

    if (modules.empty())
    {
        return INFEASIBLE_COST;
    }

    double unit_cost = calculate_unit_cost(modules, unit_cost_module_index);

    if (use_lowest_unit_cost)
    {
        unit_cost = calculate_lowest_unit_cost(modules);
    }

    if (use_highest_unit_cost)
    {
        unit_cost = calculate_highest_unit_cost(modules);
    }

    if (use_average_unit_cost)
    {
        unit_cost = calculate_average_unit_cost(modules);
    }

    if (use_weighted_average_unit_cost)
    {
        unit_cost = calculate_weighted_average_unit_cost(modules);
    }

    if (unit_cost <= 0.0 || unit_cost >= INFEASIBLE_COST / 2.0)
    {
        return INFEASIBLE_COST;
    }

    return std::ceil(load - EPSILON) * unit_cost;
}

static double calculate_single_modular_capacity_cost(
    double load,
    const std::vector<CapacityModule> &modules)
{
    const double EPSILON = 1e-9;
    const double INFEASIBLE_COST = 1e18;

    if (load <= EPSILON)
    {
        return 0.0;
    }

    if (modules.empty())
    {
        return INFEASIBLE_COST;
    }

    const CapacityModule *smallest_module = nullptr;

    for (const auto &module : modules)
    {
        if (module.capacity > EPSILON)
        {
            if (!smallest_module || module.capacity < smallest_module->capacity)
            {
                smallest_module = &module;
            }
        }
    }

    if (!smallest_module)
    {
        return INFEASIBLE_COST;
    }

    int module_capacity = static_cast<int>(std::ceil(smallest_module->capacity - EPSILON));

    if (module_capacity <= 0)
    {
        return INFEASIBLE_COST;
    }

    int units = static_cast<int>(std::ceil(load / module_capacity - EPSILON));
    return units * smallest_module->cost;
}

static double calculate_modular_capacity_cost(
    double load,
    const std::vector<CapacityModule> &modules)
{
    const double EPSILON = 1e-9;
    const double INFEASIBLE_COST = 1e18;

    if (load <= EPSILON)
    {
        return 0.0;
    }

    if (modules.empty())
    {
        return INFEASIBLE_COST;
    }

    int target_capacity = static_cast<int>(std::ceil(load - EPSILON));
    int max_module_capacity = 0;

    for (const auto &module : modules)
    {
        int module_capacity = static_cast<int>(std::ceil(module.capacity - EPSILON));
        max_module_capacity = std::max(max_module_capacity, module_capacity);
    }

    if (max_module_capacity <= 0)
    {
        return INFEASIBLE_COST;
    }

    int search_capacity = target_capacity + max_module_capacity;
    std::vector<double> dp(search_capacity + 1, INFINITY_VALUE);
    dp[0] = 0.0;

    for (int capacity = 1; capacity <= search_capacity; ++capacity)
    {
        for (const auto &module : modules)
        {
            int module_capacity = static_cast<int>(std::ceil(module.capacity - EPSILON));

            if (module_capacity <= 0 || module.cost < 0.0)
            {
                continue;
            }

            if (capacity - module_capacity >= 0 &&
                dp[capacity - module_capacity] < INFINITY_VALUE / 2.0)
            {
                dp[capacity] = std::min(
                    dp[capacity],
                    dp[capacity - module_capacity] + module.cost);
            }
        }
    }

    double best_cost = INFINITY_VALUE;
    for (int capacity = target_capacity; capacity <= search_capacity; ++capacity)
    {
        best_cost = std::min(best_cost, dp[capacity]);
    }

    if (best_cost >= INFINITY_VALUE / 2.0)
    {
        return INFEASIBLE_COST;
    }

    return best_cost;
}

static double calculate_explicit_capacity_cost(
    double load,
    const std::vector<CapacityModule> &modules)
{
    const double EPSILON = 1e-9;
    const double INFEASIBLE_COST = 1e18;

    if (load <= EPSILON)
    {
        return 0.0;
    }

    if (modules.empty())
    {
        return INFEASIBLE_COST;
    }

    int required_capacity = static_cast<int>(std::ceil(load - EPSILON));
    double best_cost = INFINITY_VALUE;

    for (const auto &module : modules)
    {
        int module_capacity = static_cast<int>(std::ceil(module.capacity - EPSILON));

        if (module_capacity >= required_capacity)
        {
            best_cost = std::min(best_cost, module.cost);
        }
    }

    return best_cost < INFINITY_VALUE / 2.0 ? best_cost : INFEASIBLE_COST;
}

static double calculate_capacity_cost(
    double load,
    const Link &link,
    Network::LinkCapacityModel model,
    int unit_cost_module_index,
    bool use_lowest_unit_cost,
    bool use_highest_unit_cost,
    bool use_average_unit_cost,
    bool use_weighted_average_unit_cost)
{
    switch (model)
    {
    case Network::LinkCapacityModel::LINEAR_LINK_CAPACITIES:
        return calculate_linear_capacity_cost(
            load,
            link.modules,
            unit_cost_module_index,
            use_lowest_unit_cost,
            use_highest_unit_cost,
            use_average_unit_cost,
            use_weighted_average_unit_cost);

    case Network::LinkCapacityModel::SINGLE_MODULAR_CAPACITIES:
        return calculate_single_modular_capacity_cost(load, link.modules);

    case Network::LinkCapacityModel::MODULAR_LINK_CAPACITIES:
        return calculate_modular_capacity_cost(load, link.modules);

    case Network::LinkCapacityModel::EXPLICIT_LINK_CAPACITIES:
        return calculate_explicit_capacity_cost(load, link.modules);
    }

    return INFINITY_VALUE;
}

static bool paths_are_equal(const Path &first_path, const Path &second_path)
{
    return first_path.links_ids == second_path.links_ids;
}

static bool path_already_exists(
    const std::vector<Path> &paths,
    const Path &candidate_path)
{
    for (const auto &path : paths)
    {
        if (paths_are_equal(path, candidate_path))
        {
            return true;
        }
    }

    return false;
}

void Network::set_nodes_number(int number_of_nodes)
{
    nodes_number = number_of_nodes;
    adjacency.clear();
    adjacency.resize(number_of_nodes);
}

void Network::set_link_model(LinkModel model)
{
    link_model = model;
    directed = (model == LinkModel::DIRECTED);
}

void Network::set_demand_model(DemandModel model)
{
    demand_model = model;
}

void Network::set_directed(bool is_directed)
{
    directed = is_directed;
    link_model = directed ? LinkModel::DIRECTED : LinkModel::UNDIRECTED;
}

void Network::set_use_lowest_unit_cost(bool enabled)
{
    use_lowest_unit_cost = enabled;
    set_unit_cost_module_index(unit_cost_module_index);
}

void Network::set_use_highest_unit_cost(bool enabled)
{
    use_highest_unit_cost = enabled;
    set_unit_cost_module_index(unit_cost_module_index);
}

void Network::set_use_average_unit_cost(bool enabled)
{
    use_average_unit_cost = enabled;
    set_unit_cost_module_index(unit_cost_module_index);
}

void Network::set_use_weighted_average_unit_cost(bool enabled)
{
    use_weighted_average_unit_cost = enabled;
    set_unit_cost_module_index(unit_cost_module_index);
}

void Network::set_unit_cost_module_index(int module_index)
{
    unit_cost_module_index = module_index;
    base_link_costs.clear();
    base_link_costs.reserve(links.size());

    for (const auto &link : links)
    {
        double heuristic_cost = 1.0;

        if (!link.modules.empty())
        {
            heuristic_cost = calculate_unit_cost(
                link.modules,
                unit_cost_module_index);

            if (use_lowest_unit_cost)
            {
                heuristic_cost = calculate_lowest_unit_cost(link.modules);
            }

            if (use_highest_unit_cost)
            {
                heuristic_cost = calculate_highest_unit_cost(link.modules);
            }

            if (use_average_unit_cost)
            {
                heuristic_cost = calculate_average_unit_cost(link.modules);
            }

            if (use_weighted_average_unit_cost)
            {
                heuristic_cost = calculate_weighted_average_unit_cost(link.modules);
            }
        }

        base_link_costs.push_back(heuristic_cost);
    }
}

void Network::set_link_capacity_model(LinkCapacityModel model)
{
    capacity_model = model;
}

void Network::set_include_routing_cost(bool enabled)
{
    include_routing_cost = enabled;
}

Network::LinkCapacityModel Network::get_link_capacity_model() const
{
    return capacity_model;
}

void Network::set_routing_model(RoutingModel model)
{
    routing_model = model;
}

Network::RoutingModel Network::get_routing_model() const
{
    return routing_model;
}

void Network::set_admissible_path_model(AdmissiblePathModel model)
{
    admissible_path_model = model;
}

void Network::set_hop_limit_model(HopLimitModel model)
{
    hop_limit_model = model;
}

void Network::set_survivability_model(SurvivabilityModel model)
{
    survivability_model = model;
}

void Network::set_node_hardware(bool enabled)
{
    node_hardware = enabled;
}

void Network::set_fixed_charge(bool enabled)
{
    fixed_charge = enabled;
}

void Network::add_link(
    int identifier,
    int source,
    int target,
    double routing_cost,
    const std::vector<CapacityModule> &modules)
{
    links.push_back({identifier, source, target, routing_cost, modules});

    if (source >= static_cast<int>(adjacency.size()) ||
        target >= static_cast<int>(adjacency.size()))
    {
        int new_size = std::max(source, target) + 1;
        adjacency.resize(new_size);
        nodes_number = new_size;
    }

    adjacency[source].push_back({target, identifier});

    if (!directed)
    {
        adjacency[target].push_back({source, identifier});
    }

    double heuristic_cost = 1.0;

    if (!modules.empty())
    {
        heuristic_cost = calculate_unit_cost(modules, unit_cost_module_index);

        if (use_lowest_unit_cost)
        {
            heuristic_cost = calculate_lowest_unit_cost(modules);
        }

        if (use_highest_unit_cost)
        {
            heuristic_cost = calculate_highest_unit_cost(modules);
        }

        if (use_average_unit_cost)
        {
            heuristic_cost = calculate_average_unit_cost(modules);
        }

        if (use_weighted_average_unit_cost)
        {
            heuristic_cost = calculate_weighted_average_unit_cost(modules);
        }
    }

    base_link_costs.push_back(heuristic_cost);
}

void Network::add_demand(
    int identifier,
    int source,
    int target,
    double volume)
{
    demands.push_back({identifier, source, target, volume, {}});
}

void Network::scale_demands(double factor)
{
    for (auto &demand : demands)
    {
        demand.volume *= factor;
    }
}

void Network::scale_costs(double factor)
{
    for (auto &link : links)
    {
        for (auto &module : link.modules)
        {
            module.cost *= factor;
        }
    }

    set_unit_cost_module_index(unit_cost_module_index);
}

void Network::keep_only_first_capacity_module()
{
    for (auto &link : links)
    {
        if (link.modules.size() > 1)
        {
            CapacityModule first_module = link.modules[0];
            link.modules.clear();
            link.modules.push_back(first_module);
        }
    }

    set_unit_cost_module_index(0);
}

void Network::apply_smallest_batch_preprocessing()
{
    const double EPSILON = 1e-9;

    double smallest_batch = INFINITY_VALUE;

    for (const auto &link : links)
    {
        for (const auto &module : link.modules)
        {
            if (module.capacity > EPSILON &&
                module.capacity < smallest_batch)
            {
                smallest_batch = module.capacity;
            }
        }
    }

    if (smallest_batch == INFINITY_VALUE)
    {
        return;
    }

    for (auto &demand : demands)
    {
        demand.volume /= smallest_batch;
    }

    if (!directed)
    {
        std::map<std::pair<int, int>, double> merged_demands;

        for (const auto &demand : demands)
        {
            int source = std::min(demand.source, demand.target);
            int target = std::max(demand.source, demand.target);

            merged_demands[{source, target}] += demand.volume;
        }

        demands.clear();

        int demand_id = 0;
        for (const auto &entry : merged_demands)
        {
            demands.push_back({
                demand_id++,
                entry.first.first,
                entry.first.second,
                entry.second,
                {}});
        }
    }

    for (auto &link : links)
    {
        if (link.modules.empty())
        {
            continue;
        }

        const CapacityModule *smallest_module = nullptr;

        for (const auto &module : link.modules)
        {
            if (module.capacity > EPSILON)
            {
                if (!smallest_module ||
                    module.capacity < smallest_module->capacity)
                {
                    smallest_module = &module;
                }
            }
        }

        if (!smallest_module)
        {
            continue;
        }

        double smallest_module_cost = smallest_module->cost;
        link.modules.clear();
        link.modules.push_back({1.0, smallest_module_cost});
    }

    set_unit_cost_module_index(0);
}

Path Network::run_dijkstra(
    int source,
    int target,
    const std::vector<double> &current_weights) const
{
    std::vector<double> shortest_distances(nodes_number, INFINITY_VALUE);
    std::vector<int> parent_link(nodes_number, -1);
    std::vector<int> parent_node(nodes_number, -1);

    std::priority_queue<
        std::pair<double, int>,
        std::vector<std::pair<double, int>>,
        std::greater<std::pair<double, int>>>
        priority_queue_nodes;

    shortest_distances[source] = 0.0;
    priority_queue_nodes.push({0.0, source});

    while (!priority_queue_nodes.empty())
    {
        std::pair<double, int> current_pair = priority_queue_nodes.top();
        double current_distance = current_pair.first;
        int current_node = current_pair.second;
        priority_queue_nodes.pop();

        if (current_distance > shortest_distances[current_node])
        {
            continue;
        }

        if (current_node == target)
        {
            break;
        }

        for (const auto &neighbor : adjacency[current_node])
        {
            int next_node = neighbor.to_node;
            int link_identifier = neighbor.link_id;

            if (link_identifier < 0 ||
                link_identifier >= static_cast<int>(current_weights.size()))
            {
                continue;
            }

            double link_weight = current_weights[link_identifier];

            if (shortest_distances[current_node] + link_weight <
                shortest_distances[next_node])
            {
                shortest_distances[next_node] =
                    shortest_distances[current_node] + link_weight;

                parent_link[next_node] = link_identifier;
                parent_node[next_node] = current_node;

                priority_queue_nodes.push({shortest_distances[next_node],
                                           next_node});
            }
        }
    }

    Path path;
    path.routing_cost = shortest_distances[target];

    if (shortest_distances[target] == INFINITY_VALUE)
    {
        return path;
    }

    int current_node = target;

    while (current_node != source)
    {
        int link_identifier = parent_link[current_node];

        if (link_identifier == -1)
        {
            path.links_ids.clear();
            path.routing_cost = INFINITY_VALUE;
            return path;
        }

        path.links_ids.push_back(link_identifier);
        current_node = parent_node[current_node];
    }

    std::reverse(path.links_ids.begin(), path.links_ids.end());

    return path;
}

static void generate_paths_worker(
    Network *network,
    int number_of_paths,
    int start_index,
    int step,
    unsigned int seed)
{
    std::mt19937 random_generator(seed);

    for (int demand_index = start_index;
         demand_index < static_cast<int>(network->demands.size());
         demand_index += step)
    {
        Demand &demand = network->demands[demand_index];
        demand.candidate_paths.clear();

        std::vector<double> current_weights = network->base_link_costs;

        for (int path_index = 0; path_index < number_of_paths / 2; path_index++)
        {
            Path path = network->run_dijkstra(
                demand.source,
                demand.target,
                current_weights);

            if (path.links_ids.empty())
            {
                break;
            }

            if (!path_already_exists(demand.candidate_paths, path))
            {
                demand.candidate_paths.push_back(path);
            }

            for (int index = 0;
                 index < static_cast<int>(path.links_ids.size());
                 index++)
            {
                int link_identifier = path.links_ids[index];

                if (link_identifier >= 0 &&
                    link_identifier < static_cast<int>(current_weights.size()))
                {
                    current_weights[link_identifier] *= 5.0;
                }
            }
        }

        int attempts = 0;
        int maximum_attempts = number_of_paths * 50;

        if (maximum_attempts < 10)
        {
            maximum_attempts = 10;
        }

        while (static_cast<int>(demand.candidate_paths.size()) < number_of_paths &&
               attempts < maximum_attempts)
        {
            attempts++;

            std::vector<double> random_weights = network->base_link_costs;

            for (int link_index = 0;
                 link_index < static_cast<int>(random_weights.size());
                 link_index++)
            {
                random_weights[link_index] *= random_weight_noise(random_generator);
            }

            Path path = network->run_dijkstra(
                demand.source,
                demand.target,
                random_weights);

            if (path.links_ids.empty())
            {
                continue;
            }

            if (!path_already_exists(demand.candidate_paths, path))
            {
                demand.candidate_paths.push_back(path);
            }
        }

        if (demand.candidate_paths.empty())
        {
            std::cerr << "Could not find a path for demand "
                      << demand.id << "\n";
        }
    }
}

void Network::generate_candidate_paths(int number_of_paths)
{
    int demand_count = static_cast<int>(demands.size());
    int thread_count = static_cast<int>(std::thread::hardware_concurrency());

    if (thread_count <= 0)
    {
        thread_count = 1;
    }

    if (demand_count > 0 && thread_count > demand_count)
    {
        thread_count = demand_count;
    }

    if (thread_count <= 1)
    {
        generate_paths_worker(this, number_of_paths, 0, 1, 12345u);
        return;
    }

    std::vector<std::thread> threads;

    for (int thread_index = 0; thread_index < thread_count; thread_index++)
    {
        threads.push_back(std::thread(
            generate_paths_worker,
            this,
            number_of_paths,
            thread_index,
            thread_count,
            static_cast<unsigned int>(12345u + thread_index)));
    }

    for (int thread_index = 0;
         thread_index < static_cast<int>(threads.size());
         thread_index++)
    {
        threads[thread_index].join();
    }
}

double Network::evaluate(
    const std::vector<std::vector<double>> &solution_flows) const
{
    const double EPSILON = 1e-9;
    const double PENALTY_COST = 1e12;
    const double INFEASIBLE_COST = 1e18;

    std::vector<double> link_loads(links.size(), 0.0);
    double total_network_cost = 0.0;

    // The solution is first converted from path flows into link loads.
    // Demand conservation is checked at the same time.
    for (size_t demand_index = 0; demand_index < demands.size(); ++demand_index)
    {
        double routed_volume = 0.0;
        int used_paths = 0;

        if (demand_index >= solution_flows.size())
        {
            total_network_cost += PENALTY_COST;
            continue;
        }

        for (size_t path_index = 0;
             path_index < demands[demand_index].candidate_paths.size();
             ++path_index)
        {
            if (path_index >= solution_flows[demand_index].size())
            {
                total_network_cost += PENALTY_COST;
                continue;
            }

            double flow = solution_flows[demand_index][path_index];

            if (flow < -EPSILON)
            {
                total_network_cost += PENALTY_COST;
                continue;
            }

            if (routing_model == RoutingModel::INTEGER &&
                std::abs(flow - std::round(flow)) > 1e-6)
            {
                total_network_cost += PENALTY_COST;
                continue;
            }

            routed_volume += flow;

            if (flow > EPSILON)
            {
                used_paths++;

                for (int link_identifier :
                     demands[demand_index].candidate_paths[path_index].links_ids)
                {
                    if (link_identifier >= 0 &&
                        link_identifier < static_cast<int>(link_loads.size()))
                    {
                        link_loads[link_identifier] += flow;

                        if (include_routing_cost)
                        {
                            total_network_cost +=
                                flow * links[link_identifier].routing_cost;
                        }
                    }
                    else
                    {
                        total_network_cost += PENALTY_COST;
                    }
                }
            }
        }

        if (routing_model == RoutingModel::SINGLE_PATH && used_paths > 1)
        {
            total_network_cost += PENALTY_COST;
        }

        if (std::abs(routed_volume - demands[demand_index].volume) > 1e-6)
        {
            total_network_cost += PENALTY_COST;
        }
    }

    // Linear link capacities are evaluated as f_e * y_e.
    // The unit cost f_e is reconstructed as module_cost / module_capacity.
    for (size_t link_index = 0; link_index < links.size(); ++link_index)
    {
        double load = link_loads[link_index];

        if (load <= EPSILON)
        {
            continue;
        }

        if (links[link_index].modules.empty())
        {
            std::cerr << "Link " << links[link_index].id
                      << " has no capacity data.\n";
            total_network_cost += PENALTY_COST;
            continue;
        }

        double link_cost = calculate_capacity_cost(
            load,
            links[link_index],
            capacity_model,
            unit_cost_module_index,
            use_lowest_unit_cost,
            use_highest_unit_cost,
            use_average_unit_cost,
            use_weighted_average_unit_cost);

        if (link_cost >= INFEASIBLE_COST / 2.0)
        {
            std::cerr << "Could not calculate linear capacity cost for link "
                      << links[link_index].id
                      << ", load = " << load << "\n";

            total_network_cost += PENALTY_COST;
            continue;
        }

        total_network_cost += link_cost;
    }

    return total_network_cost;
}

void Network::display_data() const
{
    std::cout << "NODES: " << nodes_number << "\n\n";

    std::cout << "LINKS:\n";
    for (const auto &link : links)
    {
        std::cout << link.id << " ( "
                  << link.source << " "
                  << link.target << " ) ( ";

        for (const auto &module : link.modules)
        {
            std::cout << module.capacity << " "
                      << module.cost << " ";
        }

        std::cout << ")\n";
    }

    std::cout << "\nDEMANDS:\n";
    for (const auto &demand : demands)
    {
        std::cout << demand.id << " ( "
                  << demand.source << " "
                  << demand.target << " ) "
                  << demand.volume << "\n";

        if (!demand.candidate_paths.empty())
        {
            std::cout << "  CANDIDATE PATHS:\n";

            for (size_t path_index = 0;
                 path_index < demand.candidate_paths.size();
                 ++path_index)
            {
                std::cout << "  " << path_index << " ( ";

                for (int link_identifier :
                     demand.candidate_paths[path_index].links_ids)
                {
                    std::cout << link_identifier << " ";
                }

                std::cout << ")\n";
            }
        }
    }

    std::cout << "\n";
}

void Network::print_basic_stats() const
{
    std::cout << "Instance statistics:\n";
    std::cout << "  nodes   = " << nodes_number << "\n";
    std::cout << "  links   = " << links.size() << "\n";
    std::cout << "  demands = " << demands.size() << "\n";

    int links_with_many_capacity_points = 0;

    for (const auto &link : links)
    {
        if (link.modules.size() > 1)
        {
            links_with_many_capacity_points++;
        }
    }

    std::cout << "  links with more than one capacity point = "
              << links_with_many_capacity_points << "\n";

    std::cout << "  link capacity model = ";
    switch (capacity_model)
    {
    case LinkCapacityModel::LINEAR_LINK_CAPACITIES:
        std::cout << "LINEAR_LINK_CAPACITIES\n";
        break;
    case LinkCapacityModel::SINGLE_MODULAR_CAPACITIES:
        std::cout << "SINGLE_MODULAR_CAPACITIES\n";
        break;
    case LinkCapacityModel::MODULAR_LINK_CAPACITIES:
        std::cout << "MODULAR_LINK_CAPACITIES\n";
        break;
    case LinkCapacityModel::EXPLICIT_LINK_CAPACITIES:
        std::cout << "EXPLICIT_LINK_CAPACITIES\n";
        break;
    }

    std::cout << "  link model          = ";
    switch (link_model)
    {
    case LinkModel::DIRECTED:
        std::cout << "DIRECTED\n";
        break;
    case LinkModel::UNDIRECTED:
        std::cout << "UNDIRECTED\n";
        break;
    case LinkModel::BIDIRECTED:
        std::cout << "BIDIRECTED\n";
        break;
    }

    std::cout << "  demand model        = ";
    switch (demand_model)
    {
    case DemandModel::DIRECTED:
        std::cout << "DIRECTED\n";
        break;
    case DemandModel::UNDIRECTED:
        std::cout << "UNDIRECTED\n";
        break;
    }

    std::cout << "  routing model       = ";
    switch (routing_model)
    {
    case RoutingModel::CONTINUOUS:
        std::cout << "CONTINUOUS\n";
        break;
    case RoutingModel::INTEGER:
        std::cout << "INTEGER\n";
        break;
    case RoutingModel::SINGLE_PATH:
        std::cout << "SINGLE_PATH\n";
        break;
    }

    std::cout << "  admissible paths    = ";
    switch (admissible_path_model)
    {
    case AdmissiblePathModel::ALL_PATHS:
        std::cout << "ALL_PATHS\n";
        break;
    case AdmissiblePathModel::EXPLICIT_LIST:
        std::cout << "EXPLICIT_LIST\n";
        break;
    }

    std::cout << "  hop limits          = ";
    switch (hop_limit_model)
    {
    case HopLimitModel::IGNORE_HOP_LIMITS:
        std::cout << "IGNORE_HOP_LIMITS\n";
        break;
    case HopLimitModel::INDIVIDUAL_HOP_LIMITS:
        std::cout << "INDIVIDUAL_HOP_LIMITS\n";
        break;
    }

    std::cout << "  survivability       = ";
    switch (survivability_model)
    {
    case SurvivabilityModel::NO_SURVIVABILITY:
        std::cout << "NO_SURVIVABILITY\n";
        break;
    case SurvivabilityModel::ONE_PLUS_ONE_PROTECTION:
        std::cout << "ONE_PLUS_ONE_PROTECTION\n";
        break;
    case SurvivabilityModel::SHARED_PATH_PROTECTION:
        std::cout << "SHARED_PATH_PROTECTION\n";
        break;
    case SurvivabilityModel::UNRESTRICTED_FLOW_RECONFIGURATION:
        std::cout << "UNRESTRICTED_FLOW_RECONFIGURATION\n";
        break;
    }

    std::cout << "  node hardware       = "
              << (node_hardware ? "YES" : "NO") << "\n";
    std::cout << "  fixed charge        = "
              << (fixed_charge ? "YES" : "NO") << "\n";
    std::cout << "  unit cost module index = "
              << unit_cost_module_index << "\n";

    if (!links.empty() && !links[0].modules.empty())
    {
        const CapacityModule &selected_module =
            select_unit_cost_module(
                links[0].modules,
                unit_cost_module_index);

        double unit_cost =
            calculate_unit_cost(links[0].modules, unit_cost_module_index);

        std::cout << "  first link selected capacity point:\n";
        std::cout << "    capacity  = " << selected_module.capacity << "\n";
        std::cout << "    cost      = " << selected_module.cost << "\n";
        std::cout << "    unit cost = " << unit_cost << "\n";
    }
}
