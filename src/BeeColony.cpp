#include "../include/BeeColony.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

static int random_int(std::mt19937& rng, int max_value)
{
    if (max_value <= 0)
    {
        return 0;
    }

    return static_cast<int>(rng() % max_value);
}

static double random_double(std::mt19937& rng)
{
    return static_cast<double>(rng() % 1000000) / 1000000.0;
}

static bool can_split_integer(double volume)
{
    return std::abs(volume - std::round(volume)) < 0.000001 &&
           static_cast<int>(std::round(volume)) > 1;
}

BeeColony::BeeColony(
    int pop_size,
    int max_iter,
    int lim,
    bool allow_splitting)
    : population_size(pop_size),
      max_iterations(max_iter),
      limit(lim),
      allow_split_flows(allow_splitting)
{
    std::random_device random_device;
    rng = std::mt19937(random_device());
}

double BeeColony::calculate_fitness(double cost)
{
    return 1.0 / (1.0 + cost);
}

FoodSource BeeColony::generate_random_solution(
    const Network& network,
    std::mt19937& local_rng)
{
    FoodSource solution;
    solution.trial_counter = 0;
    solution.flows.resize(network.demands.size());

    for (int demand_index = 0;
         demand_index < static_cast<int>(network.demands.size());
         demand_index++)
    {
        int path_count =
            static_cast<int>(network.demands[demand_index].candidate_paths.size());

        solution.flows[demand_index].assign(path_count, 0.0);

        if (path_count == 0)
        {
            continue;
        }

        int selected_path = random_int(local_rng, path_count);
        solution.flows[demand_index][selected_path] =
            network.demands[demand_index].volume;
    }

    solution.cost = network.evaluate(solution.flows);
    solution.fitness = calculate_fitness(solution.cost);

    return solution;
}

void BeeColony::modify_solution(
    FoodSource& source,
    const Network& network,
    std::mt19937& local_rng)
{
    const double EPSILON = 0.000000001;

    if (network.demands.empty())
    {
        return;
    }

    std::vector<std::vector<double>> best_flows = source.flows;
    double best_cost = source.cost;

    int demand_trials = 4;
    if (static_cast<int>(network.demands.size()) < demand_trials)
    {
        demand_trials = static_cast<int>(network.demands.size());
    }

    for (int trial = 0; trial < demand_trials; trial++)
    {
        int demand_index =
            random_int(local_rng, static_cast<int>(network.demands.size()));

        int path_count =
            static_cast<int>(network.demands[demand_index].candidate_paths.size());

        if (path_count <= 1)
        {
            continue;
        }

        for (int path_index = 0; path_index < path_count; path_index++)
        {
            std::vector<std::vector<double>> candidate_flows = source.flows;

            for (int other_path = 0; other_path < path_count; other_path++)
            {
                candidate_flows[demand_index][other_path] = 0.0;
            }

            candidate_flows[demand_index][path_index] =
                network.demands[demand_index].volume;

            double candidate_cost = network.evaluate(candidate_flows);

            if (candidate_cost < best_cost - EPSILON)
            {
                best_cost = candidate_cost;
                best_flows = candidate_flows;
            }
        }

        if (allow_split_flows)
        {
            int split_trials = path_count * 2;

            if (split_trials > 20)
            {
                split_trials = 20;
            }

            for (int split_trial = 0; split_trial < split_trials; split_trial++)
            {
                int first_path = random_int(local_rng, path_count);
                int second_path = random_int(local_rng, path_count);

                if (first_path == second_path)
                {
                    second_path = (second_path + 1) % path_count;
                }

                std::vector<std::vector<double>> candidate_flows = source.flows;

                for (int path_index = 0; path_index < path_count; path_index++)
                {
                    candidate_flows[demand_index][path_index] = 0.0;
                }

                if (network.get_routing_model() == Network::RoutingModel::INTEGER &&
                    can_split_integer(network.demands[demand_index].volume))
                {
                    int demand_units =
                        static_cast<int>(std::round(network.demands[demand_index].volume));
                    int first_units = 1 + random_int(local_rng, demand_units - 1);

                    candidate_flows[demand_index][first_path] = first_units;
                    candidate_flows[demand_index][second_path] =
                        demand_units - first_units;
                }
                else if (network.get_routing_model() == Network::RoutingModel::CONTINUOUS)
                {
                    double first_fraction = 0.2 + random_double(local_rng) * 0.6;

                    candidate_flows[demand_index][first_path] =
                        first_fraction * network.demands[demand_index].volume;
                    candidate_flows[demand_index][second_path] =
                        (1.0 - first_fraction) * network.demands[demand_index].volume;
                }
                else
                {
                    continue;
                }

                double candidate_cost = network.evaluate(candidate_flows);

                if (candidate_cost < best_cost - EPSILON)
                {
                    best_cost = candidate_cost;
                    best_flows = candidate_flows;
                }
            }
        }
    }

    int rebuild_trials = 6;

    for (int trial = 0; trial < rebuild_trials; trial++)
    {
        std::vector<std::vector<double>> candidate_flows = source.flows;

        int rebuild_count = 2 + random_int(local_rng, 3);

        for (int rebuild_index = 0;
             rebuild_index < rebuild_count;
             rebuild_index++)
        {
            int demand_index =
                random_int(local_rng, static_cast<int>(network.demands.size()));

            int path_count =
                static_cast<int>(network.demands[demand_index].candidate_paths.size());

            if (path_count == 0)
            {
                continue;
            }

            for (int path_index = 0; path_index < path_count; path_index++)
            {
                candidate_flows[demand_index][path_index] = 0.0;
            }

            if (!allow_split_flows || path_count == 1 || random_double(local_rng) < 0.65)
            {
                int selected_path = random_int(local_rng, path_count);
                candidate_flows[demand_index][selected_path] =
                    network.demands[demand_index].volume;
            }
            else
            {
                int first_path = random_int(local_rng, path_count);
                int second_path = random_int(local_rng, path_count);

                if (first_path == second_path)
                {
                    second_path = (second_path + 1) % path_count;
                }

                if (network.get_routing_model() == Network::RoutingModel::INTEGER &&
                    can_split_integer(network.demands[demand_index].volume))
                {
                    int demand_units =
                        static_cast<int>(std::round(network.demands[demand_index].volume));
                    int first_units = 1 + random_int(local_rng, demand_units - 1);

                    candidate_flows[demand_index][first_path] = first_units;
                    candidate_flows[demand_index][second_path] =
                        demand_units - first_units;
                }
                else if (network.get_routing_model() == Network::RoutingModel::CONTINUOUS)
                {
                    double first_fraction = 0.2 + random_double(local_rng) * 0.6;

                    candidate_flows[demand_index][first_path] =
                        first_fraction * network.demands[demand_index].volume;
                    candidate_flows[demand_index][second_path] =
                        (1.0 - first_fraction) * network.demands[demand_index].volume;
                }
                else
                {
                    int selected_path = random_int(local_rng, path_count);
                    candidate_flows[demand_index][selected_path] =
                        network.demands[demand_index].volume;
                }
            }
        }

        double candidate_cost = network.evaluate(candidate_flows);

        if (candidate_cost < best_cost - EPSILON)
        {
            best_cost = candidate_cost;
            best_flows = candidate_flows;
        }
    }

    if (best_cost < source.cost - EPSILON)
    {
        source.flows = best_flows;
        source.cost = best_cost;
        source.fitness = calculate_fitness(source.cost);
        source.trial_counter = 0;
    }
    else
    {
        source.trial_counter++;
    }
}

std::pair<double, std::vector<std::vector<double>>> BeeColony::run(
    const Network& network)
{
    std::vector<FoodSource> population(population_size);

    int thread_count = static_cast<int>(std::thread::hardware_concurrency());

    if (thread_count <= 0)
    {
        thread_count = 1;
    }

    if (thread_count > population_size)
    {
        thread_count = population_size;
    }

    std::vector<std::thread> threads;
    std::vector<unsigned int> init_seeds(thread_count);

    for (int thread_index = 0; thread_index < thread_count; thread_index++)
    {
        init_seeds[thread_index] = rng() + thread_index + 1;
    }

    for (int thread_index = 0; thread_index < thread_count; thread_index++)
    {
        threads.push_back(std::thread(
            [&, thread_index]()
            {
                std::mt19937 local_rng(init_seeds[thread_index]);

                for (int source_index = thread_index;
                     source_index < population_size;
                     source_index += thread_count)
                {
                    population[source_index] =
                        generate_random_solution(network, local_rng);
                }
            }));
    }

    for (int thread_index = 0; thread_index < static_cast<int>(threads.size()); thread_index++)
    {
        threads[thread_index].join();
    }

    FoodSource global_best = population[0];

    for (int iteration = 0; iteration < max_iterations; iteration++)
    {
        threads.clear();
        std::vector<unsigned int> modify_seeds(thread_count);

        for (int thread_index = 0; thread_index < thread_count; thread_index++)
        {
            modify_seeds[thread_index] = rng() + iteration + thread_index + 1;
        }

        for (int thread_index = 0; thread_index < thread_count; thread_index++)
        {
            threads.push_back(std::thread(
                [&, thread_index]()
                {
                    std::mt19937 local_rng(modify_seeds[thread_index]);

                    for (int source_index = thread_index;
                         source_index < population_size;
                         source_index += thread_count)
                    {
                        modify_solution(population[source_index], network, local_rng);
                    }
                }));
        }

        for (int thread_index = 0; thread_index < static_cast<int>(threads.size()); thread_index++)
        {
            threads[thread_index].join();
        }

        double worst_cost = population[0].cost;

        for (int source_index = 1; source_index < population_size; source_index++)
        {
            if (population[source_index].cost > worst_cost)
            {
                worst_cost = population[source_index].cost;
            }
        }

        std::vector<double> selection_weights(population_size, 1.0);
        double weights_sum = 0.0;

        for (int source_index = 0; source_index < population_size; source_index++)
        {
            selection_weights[source_index] =
                worst_cost - population[source_index].cost + 1.0;

            if (selection_weights[source_index] < 1.0)
            {
                selection_weights[source_index] = 1.0;
            }

            weights_sum += selection_weights[source_index];
        }

        for (int onlooker = 0; onlooker < population_size; onlooker++)
        {
            double selected_value = random_double(rng) * weights_sum;
            double current_sum = 0.0;
            int selected_source = 0;

            for (int source_index = 0; source_index < population_size; source_index++)
            {
                current_sum += selection_weights[source_index];

                if (selected_value <= current_sum)
                {
                    selected_source = source_index;
                    break;
                }
            }

            modify_solution(population[selected_source], network, rng);
        }

        for (int source_index = 0; source_index < population_size; source_index++)
        {
            if (population[source_index].cost < global_best.cost)
            {
                global_best = population[source_index];
            }
        }

        for (int source_index = 0; source_index < population_size; source_index++)
        {
            if (population[source_index].trial_counter >= limit)
            {
                population[source_index] =
                    generate_random_solution(network, rng);
            }
        }

        if (iteration % 10 == 0)
        {
            std::cout << "Iteration " << iteration
                      << " | Best network cost: "
                      << global_best.cost << "\n";
        }
    }

    return {global_best.cost, global_best.flows};
}
