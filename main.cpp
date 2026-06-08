#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "include/AntColony.h"
#include "include/BeeColony.h"
#include "include/Network.h"
#include "include/SNDLIBParser.h"

struct ProblemConfig
{
    std::string name;
    std::string instance_file;
    std::string model_file;
};

struct AlgorithmConfig
{
    std::string name = "abc";
    int candidate_paths = 3;
    int population_size = 60;
    int iterations = 20;
    int limit = 8;
};

static std::string link_capacity_model_to_string(Network::LinkCapacityModel model)
{
    switch (model)
    {
    case Network::LinkCapacityModel::LINEAR_LINK_CAPACITIES:
        return "linear";
    case Network::LinkCapacityModel::SINGLE_MODULAR_CAPACITIES:
        return "single modular";
    case Network::LinkCapacityModel::MODULAR_LINK_CAPACITIES:
        return "modular";
    case Network::LinkCapacityModel::EXPLICIT_LINK_CAPACITIES:
        return "explicit";
    }

    return "unknown";
}

static void apply_model_preprocessing(Network &network)
{
    if (network.first_module_only)
    {
        network.keep_only_first_capacity_module();
    }

    if (network.demand_scale != 1.0)
    {
        network.scale_demands(network.demand_scale);
    }

    if (network.cost_scale != 1.0)
    {
        network.scale_costs(network.cost_scale);
    }
}

static void choose_parameters(
    const Network &network,
    AlgorithmConfig &algorithm_config)
{
    algorithm_config.candidate_paths = 6;
    algorithm_config.population_size = 80;
    algorithm_config.iterations = 30;
    algorithm_config.limit = 10;

    if (network.demands.size() > 60)
    {
        algorithm_config.candidate_paths = 3;
        algorithm_config.population_size = 60;
        algorithm_config.iterations = 20;
        algorithm_config.limit = 8;
    }
    else if (network.demands.size() > 30)
    {
        algorithm_config.candidate_paths = 6;
        algorithm_config.population_size = 120;
        algorithm_config.iterations = 40;
        algorithm_config.limit = 15;
    }

    if (network.candidate_paths > 0)
    {
        algorithm_config.candidate_paths = network.candidate_paths;
    }

    if (network.population_size > 0)
    {
        algorithm_config.population_size = network.population_size;
    }

    if (network.iterations > 0)
    {
        algorithm_config.iterations = network.iterations;
    }

    if (network.limit > 0)
    {
        algorithm_config.limit = network.limit;
    }
}

static double solve_problem(
    Network &network,
    const AlgorithmConfig &algorithm_config)
{
    if (algorithm_config.name == "aco")
    {
        std::vector<std::vector<double>> result =
            algorytm_mrowkowy(
                network,
                algorithm_config.population_size,
                algorithm_config.iterations);

        return network.evaluate(result);
    }

    bool allow_split_flows =
        network.get_routing_model() != Network::RoutingModel::SINGLE_PATH;

    BeeColony bee_colony(
        algorithm_config.population_size,
        algorithm_config.iterations,
        algorithm_config.limit,
        allow_split_flows);

    std::pair<double, std::vector<std::vector<double>>> result =
        bee_colony.run(network);

    return result.first;
}

static bool run_problem(
    const ProblemConfig &problem_config,
    const AlgorithmConfig &base_algorithm_config)
{
    Network network;

    if (!load_model(problem_config.model_file, network))
    {
        return false;
    }

    if (!load_instance(problem_config.instance_file, network))
    {
        return false;
    }

    apply_model_preprocessing(network);

    AlgorithmConfig algorithm_config = base_algorithm_config;
    choose_parameters(network, algorithm_config);

    network.generate_candidate_paths(algorithm_config.candidate_paths);

    std::ofstream null_stream("NUL");
    std::streambuf *cout_buffer = std::cout.rdbuf();
    std::streambuf *cerr_buffer = std::cerr.rdbuf();

    std::cout.rdbuf(null_stream.rdbuf());
    std::cerr.rdbuf(null_stream.rdbuf());

    double cost = solve_problem(network, algorithm_config);

    std::cout.rdbuf(cout_buffer);
    std::cerr.rdbuf(cerr_buffer);

    double gap = 0.0;

    if (network.has_reference_cost && network.reference_cost != 0.0)
    {
        gap = 100.0 * (cost - network.reference_cost) / network.reference_cost;
    }

    std::cout << problem_config.name << ";"
              << link_capacity_model_to_string(network.get_link_capacity_model()) << ";"
              << network.unit_cost_module_index << ";"
              << network.demand_scale << ";"
              << (network.first_module_only ? "yes" : "no") << ";"
              << algorithm_config.candidate_paths << ";"
              << algorithm_config.population_size << ";"
              << algorithm_config.iterations << ";"
              << std::fixed << cost << ";"
              << network.reference_cost << ";"
              << gap << "\n";

    return true;
}

int main()
{
    srand(1);

    AlgorithmConfig algorithm_config;
    algorithm_config.name = "abc";

    std::vector<ProblemConfig> problems = {
        {"di-yuan",
         "SNDLIB_instances/yuan.txt",
         "SNDLIB_instances/models/selected/di-yuan.txt"},
        {"pdh",
         "SNDLIB_instances/pdh.txt",
         "SNDLIB_instances/models/selected/pdh.txt"},
        {"nobel-eu",
         "SNDLIB_instances/nobel-eu.txt",
         "SNDLIB_instances/models/selected/nobel-eu.txt"},
        {"norway",
         "SNDLIB_instances/norway.txt",
         "SNDLIB_instances/models/selected/norway.txt"}};

    std::cout << "=== Selected NLP instances ===\n";
    std::cout << "Metaheuristic: " << algorithm_config.name << "\n";
    std::cout << "instance;capacity_model;unit_cost_index;demand_scale;"
              << "first_module_only;paths;population;iterations;cost;reference;gap\n";

    for (const ProblemConfig &problem_config : problems)
    {
        if (!run_problem(problem_config, algorithm_config))
        {
            return 1;
        }
    }

    return 0;
}
