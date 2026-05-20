
#include "../include/Network.h"
#include <queue>
#include <limits>
#include <iostream>
#include <algorithm>
#include <cmath>

const double INF = std::numeric_limits<double>::infinity();

void Network::set_nodes_number(int n) {
    nodes_number = n;
    adj.resize(n);
}

void Network::add_link(int id, int source, int target, const std::vector<CapacityModule>& modules) {
    links.push_back({id, source, target, modules});

    if (source >= adj.size() || target >= adj.size()) {
        int new_size = std::max(source, target) + 1;
        adj.resize(new_size);
        nodes_number = new_size;
    }

    adj[source].push_back({target, id});
    adj[target].push_back({source, id});

    double heuristic_cost = modules.empty() ? 1.0 : modules[0].cost;
    base_link_costs.push_back(heuristic_cost);
}

void Network::add_demand(int id, int source, int target, double volume) {
    demands.push_back({id, source, target, volume, {}});
}

Path Network::run_dijkstra(int src, int dst, const std::vector<double>& current_weights) const {
    std::vector<double> dist(nodes_number, INF);
    std::vector<int> parent_link(nodes_number, -1);
    std::vector<int> parent_node(nodes_number, -1);

    std::priority_queue< std::pair<double, int>,
                         std::vector<std::pair<double, int>>,
                         std::greater<std::pair<double, int>> > pq;

    dist[src] = 0.0;
    pq.push({0.0, src});

    while (!pq.empty()) {
        auto [current_dist, u] = pq.top();
        pq.pop();

        if (current_dist > dist[u]) continue;

        if (u == dst) break;

        for (const auto& neighbor : adj[u]) {
            int v = neighbor.to_node;
            int link_id = neighbor.link_id;
            double weight = current_weights[link_id];

            if (dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
                parent_link[v] = link_id;
                parent_node[v] = u;

                pq.push({dist[v], v});
            }
        }
    }

    Path path;
    path.routing_cost = dist[dst];

    if (dist[dst] == INF) {
        return path;
    }

    int curr = dst;
    while (curr != src) {
        int link_id = parent_link[curr];
        path.links_ids.push_back(link_id);
        curr = parent_node[curr];
    }

    std::reverse(path.links_ids.begin(), path.links_ids.end());

    return path;
}

void Network::generate_candidate_paths(int num_paths) {
    for (auto& demand : demands) {
        std::vector<double> current_weights = base_link_costs;

        for (int i = 0; i < num_paths; ++i) {
            Path p = run_dijkstra(demand.source, demand.target, current_weights);

            if (p.links_ids.empty()) {
                if (i == 0) {
                    std::cerr << "Couldn't find path for the demand " << demand.id << "\n";
                }
                break;
            }

            demand.candidate_paths.push_back(p);
            for (int link_id : p.links_ids) {
                current_weights[link_id] *= 10.0;
            }
        }
    }
}

double Network::evaluate(const std::vector<std::vector<double>>& solution_flows) const {

    std::vector<double> link_loads(links.size(), 0.0);

    for (size_t d = 0; d < demands.size(); ++d) {
        for (size_t p = 0; p < demands[d].candidate_paths.size(); ++p) {

            double flow = solution_flows[d][p];

            if (flow > 0.0) {
                for (int link_id : demands[d].candidate_paths[p].links_ids) {
                    link_loads[link_id] += flow;
                }
            }
        }
    }

    double total_network_cost = 0.0;

    for (size_t l = 0; l < links.size(); ++l) {
        double load = link_loads[l];

        if (load <= 0.0) continue;

        int required_cap = static_cast<int>(std::ceil(load));

        int max_mod_cap = 0;
        for (const auto& mod : links[l].modules) {
            int cap = static_cast<int>(std::round(mod.capacity));
            if (cap > max_mod_cap) max_mod_cap = cap;
        }

        std::vector<double> dp(required_cap + max_mod_cap + 1, INF);
        dp[0] = 0.0;

        for (int i = 0; i < required_cap; ++i) {
            if (dp[i] == INF) continue;

            for (const auto& mod : links[l].modules) {
                int cap = static_cast<int>(std::round(mod.capacity));

                if (dp[i] + mod.cost < dp[i + cap]) {
                    dp[i + cap] = dp[i] + mod.cost;
                }
            }
        }

        double min_cost_for_link = INF;
        for (int i = required_cap; i < dp.size(); ++i) {
            if (dp[i] < min_cost_for_link) {
                min_cost_for_link = dp[i];
            }
        }

        total_network_cost += min_cost_for_link;
    }

    return total_network_cost;
}

void Network::display_data() const {
    std::cout << "NODES: " << nodes_number << "\n\n";

    std::cout << "LINKS:\n";
    for (const auto& link : links) {
        std::cout << link.id << " ( " << link.source << " " << link.target << " ) ( ";
        for (const auto& mod : link.modules) {
            std::cout << mod.capacity << " " << mod.cost << " ";
        }
        std::cout << ")\n";
    }

    std::cout << "\nDEMANDS:\n";
    for (const auto& demand : demands) {
        std::cout << demand.id << " ( " << demand.source << " " << demand.target << " ) "
                  << demand.volume << "\n";

        if (!demand.candidate_paths.empty()) {
            std::cout << "  CANDIDATE_PATHS:\n";
            for (size_t p = 0; p < demand.candidate_paths.size(); ++p) {
                std::cout << "  " << p << " ( ";
                for (int link_id : demand.candidate_paths[p].links_ids) {
                    std::cout << link_id << " ";
                }
                std::cout << ")\n";
            }
        }
    }
    std::cout << "\n";
}