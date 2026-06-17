#include "../include/TabuSearch.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>

namespace fs = std::filesystem;

static std::vector<std::vector<double>> create_initial_solution(const Network& net,
                                                                std::vector<size_t>& current_path) {
    std::vector<std::vector<double>> solution(net.demands.size());
    current_path.assign(net.demands.size(), 0);

    for (size_t d = 0; d < net.demands.size(); ++d) {
        size_t paths_count = net.demands[d].candidate_paths.size();
        solution[d].assign(paths_count, 0.0);
        if (paths_count > 0) {
            solution[d][0] = net.demands[d].volume;
            current_path[d] = 0;
        }
    }

    return solution;
}

static bool find_best_neighbor(const Network& net,
                               const std::vector<std::vector<double>>& current_solution,
                               const std::vector<size_t>& current_path,
                               const std::vector<std::vector<int>>& tabu_list,
                               int iteration,
                               std::vector<std::vector<double>>& best_candidate,
                               size_t& best_demand,
                               size_t& best_path) {
    double best_cost = std::numeric_limits<double>::infinity();
    bool found = false;

    for (size_t d = 0; d < net.demands.size(); ++d) {
        size_t active_path = current_path[d];
        size_t path_count = net.demands[d].candidate_paths.size();

        if (path_count <= 1) {
            continue;
        }

        for (size_t p = 0; p < path_count; ++p) {
            if (p == active_path) {
                continue;
            }

            if (tabu_list[d][p] > iteration) {
                continue;
            }

            auto candidate = current_solution;
            candidate[d][active_path] = 0.0;
            candidate[d][p] = net.demands[d].volume;

            double candidate_cost = net.evaluate(candidate);
            if (candidate_cost < best_cost) {
                best_cost = candidate_cost;
                best_candidate = candidate;
                best_demand = d;
                best_path = p;
                found = true;
            }
        }
    }

    return found;
}

static void save_solution(const Network& net,
                          const std::vector<std::vector<double>>& solution,
                          const fs::path& filename,
                          double objective_value) {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "Unable to open output file: " << filename << std::endl;
        return;
    }

    out << "objective_value " << objective_value << "\n";
    out << "demand_count " << net.demands.size() << "\n";

    for (size_t d = 0; d < net.demands.size(); ++d) {
        out << "demand " << net.demands[d].id << " " << net.demands[d].source << " "
            << net.demands[d].target << " " << net.demands[d].volume << "\n";
        for (size_t p = 0; p < solution[d].size(); ++p) {
            if (solution[d][p] > 0.0) {
                out << "  path " << p << " flow " << solution[d][p] << " cost "
                    << net.demands[d].candidate_paths[p].routing_cost << "\n";
            }
        }
    }
}

bool run_tabu_search(const Network& net, const std::string& output_folder, int max_iterations, int tabu_tenure) {
    if (net.demands.empty()) {
        std::cerr << "Network has no demands for Tabu Search." << std::endl;
        return false;
    }

    fs::path folder(output_folder);
    if (!fs::exists(folder)) {
        fs::create_directories(folder);
    }

    std::vector<size_t> current_path;
    auto current_solution = create_initial_solution(net, current_path);
    auto best_solution = current_solution;
    double best_value = net.evaluate(best_solution);

    std::vector<std::vector<int>> tabu_list(net.demands.size());
    for (size_t d = 0; d < net.demands.size(); ++d) {
        tabu_list[d].assign(net.demands[d].candidate_paths.size(), 0);
    }

    for (int iter = 0; iter < max_iterations; ++iter) {
        std::vector<std::vector<double>> candidate_solution;
        size_t candidate_demand = 0;
        size_t candidate_path = 0;

        bool found = find_best_neighbor(net, current_solution, current_path, tabu_list,
                                        iter, candidate_solution, candidate_demand, candidate_path);
        if (!found) {
            break;
        }

        size_t old_path = current_path[candidate_demand];
        current_solution = candidate_solution;
        current_path[candidate_demand] = candidate_path;

        tabu_list[candidate_demand][old_path] = iter + tabu_tenure;

        double candidate_cost = net.evaluate(current_solution);
        if (candidate_cost < best_value) {
            best_value = candidate_cost;
            best_solution = current_solution;
        }
    }

    fs::path out_filename = folder / "taboo_solution.txt";
    save_solution(net, best_solution, out_filename, best_value);
    return true;
}
