#ifndef NLP_OPTIMIZATION_BRUTEFORCE_H
#define NLP_OPTIMIZATION_BRUTEFORCE_H

#include "../include/Network.h"
#include <vector>

struct BruteForceResult
{
    bool solved;
    bool exceeded_limit;
    bool non_integer_demands;
    int missing_demand_index;
    double best_cost;
    long long checked_solutions;
    long long possible_solutions;
    std::vector<std::vector<double>> best_flows;
};

BruteForceResult brute_force_single_path(
    const Network& network,
    long long max_solutions);

BruteForceResult brute_force_integer_split(
    const Network& network,
    long long max_solutions);

#endif //NLP_OPTIMIZATION_BRUTEFORCE_H
