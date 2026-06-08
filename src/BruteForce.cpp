#include "../include/BruteForce.h"

#include <cmath>
#include <limits>

static void brute_force_recursive(
    const Network& network,
    int demand_index,
    std::vector<std::vector<double>>& current_flows,
    BruteForceResult& result)
{
    if (demand_index == static_cast<int>(network.demands.size()))
    {
        double cost = network.evaluate(current_flows);
        result.checked_solutions++;

        if (cost < result.best_cost)
        {
            result.best_cost = cost;
            result.best_flows = current_flows;
        }

        return;
    }

    int path_count =
        static_cast<int>(network.demands[demand_index].candidate_paths.size());

    for (int path_index = 0; path_index < path_count; path_index++)
    {
        for (int other_path = 0; other_path < path_count; other_path++)
        {
            current_flows[demand_index][other_path] = 0.0;
        }

        current_flows[demand_index][path_index] =
            network.demands[demand_index].volume;

        brute_force_recursive(
            network,
            demand_index + 1,
            current_flows,
            result);
    }
}

BruteForceResult brute_force_single_path(
    const Network& network,
    long long max_solutions)
{
    BruteForceResult result;
    result.solved = false;
    result.exceeded_limit = false;
    result.non_integer_demands = false;
    result.missing_demand_index = -1;
    result.best_cost = std::numeric_limits<double>::infinity();
    result.checked_solutions = 0;
    result.possible_solutions = 1;
    result.best_flows.clear();

    if (network.demands.empty())
    {
        result.solved = true;
        result.best_cost = 0.0;
        return result;
    }

    for (int demand_index = 0;
         demand_index < static_cast<int>(network.demands.size());
         demand_index++)
    {
        int path_count =
            static_cast<int>(network.demands[demand_index].candidate_paths.size());

        if (path_count == 0)
        {
            result.missing_demand_index = demand_index;
            return result;
        }

        if (result.possible_solutions > max_solutions / path_count)
        {
            result.exceeded_limit = true;
            result.possible_solutions = max_solutions + 1;
            return result;
        }

        result.possible_solutions *= path_count;
    }

    std::vector<std::vector<double>> current_flows;
    current_flows.resize(network.demands.size());

    for (int demand_index = 0;
         demand_index < static_cast<int>(network.demands.size());
         demand_index++)
    {
        int path_count =
            static_cast<int>(network.demands[demand_index].candidate_paths.size());

        current_flows[demand_index].assign(path_count, 0.0);
    }

    brute_force_recursive(network, 0, current_flows, result);
    result.solved = true;

    return result;
}

static long long count_split_combinations(int units, int path_count)
{
    long long result = 1;

    for (int i = 1; i <= path_count - 1; i++)
    {
        result = result * (units + i) / i;
    }

    return result;
}

static void brute_force_split_recursive(
    const Network& network,
    int demand_index,
    std::vector<std::vector<double>>& current_flows,
    BruteForceResult& result)
{
    if (demand_index == static_cast<int>(network.demands.size()))
    {
        double cost = network.evaluate(current_flows);
        result.checked_solutions++;

        if (cost < result.best_cost)
        {
            result.best_cost = cost;
            result.best_flows = current_flows;
        }

        return;
    }

    int path_count =
        static_cast<int>(network.demands[demand_index].candidate_paths.size());
    int units = static_cast<int>(network.demands[demand_index].volume + 0.5);
    std::vector<int> split(path_count, 0);

    int path_index = 0;

    while (path_index >= 0)
    {
        int used_units = 0;

        for (int i = 0; i < path_index; i++)
        {
            used_units += split[i];
        }

        if (path_index == path_count - 1)
        {
            split[path_index] = units - used_units;

            for (int i = 0; i < path_count; i++)
            {
                current_flows[demand_index][i] = split[i];
            }

            brute_force_split_recursive(
                network,
                demand_index + 1,
                current_flows,
                result);

            path_index--;

            if (path_index >= 0)
            {
                split[path_index]++;
            }
        }
        else if (used_units + split[path_index] <= units)
        {
            path_index++;
            split[path_index] = 0;
        }
        else
        {
            split[path_index] = 0;
            path_index--;

            if (path_index >= 0)
            {
                split[path_index]++;
            }
        }
    }
}

BruteForceResult brute_force_integer_split(
    const Network& network,
    long long max_solutions)
{
    BruteForceResult result;
    result.solved = false;
    result.exceeded_limit = false;
    result.non_integer_demands = false;
    result.missing_demand_index = -1;
    result.best_cost = std::numeric_limits<double>::infinity();
    result.checked_solutions = 0;
    result.possible_solutions = 1;
    result.best_flows.clear();

    if (network.demands.empty())
    {
        result.solved = true;
        result.best_cost = 0.0;
        return result;
    }

    for (int demand_index = 0;
         demand_index < static_cast<int>(network.demands.size());
         demand_index++)
    {
        int path_count =
            static_cast<int>(network.demands[demand_index].candidate_paths.size());

        if (path_count == 0)
        {
            result.missing_demand_index = demand_index;
            return result;
        }

        double volume = network.demands[demand_index].volume;
        int units = static_cast<int>(volume + 0.5);

        if (std::abs(volume - units) > 0.000001)
        {
            result.non_integer_demands = true;
            return result;
        }

        long long demand_combinations =
            count_split_combinations(units, path_count);

        if (result.possible_solutions > max_solutions / demand_combinations)
        {
            result.exceeded_limit = true;
            result.possible_solutions = max_solutions + 1;
            return result;
        }

        result.possible_solutions *= demand_combinations;
    }

    std::vector<std::vector<double>> current_flows;
    current_flows.resize(network.demands.size());

    for (int demand_index = 0;
         demand_index < static_cast<int>(network.demands.size());
         demand_index++)
    {
        int path_count =
            static_cast<int>(network.demands[demand_index].candidate_paths.size());

        current_flows[demand_index].assign(path_count, 0.0);
    }

    brute_force_split_recursive(network, 0, current_flows, result);
    result.solved = true;

    return result;
}
