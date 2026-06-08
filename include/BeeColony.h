
#ifndef NLP_OPTIMIZATION_BEECOLONY_H
#define NLP_OPTIMIZATION_BEECOLONY_H


#include "../include/Network.h"
#include <vector>
#include <random>

struct FoodSource {
    std::vector<std::vector<double>> flows;
    double cost;
    double fitness;
    int trial_counter;
};

class BeeColony
{
private:
    int population_size;
    int max_iterations;
    int limit;
    bool allow_split_flows;

    std::mt19937 rng;
    FoodSource generate_random_solution(const Network& net, std::mt19937& rng);
    void modify_solution(FoodSource& source, const Network& net, std::mt19937& rng);
    double calculate_fitness(double cost);

public:
    BeeColony(int pop_size, int max_iter, int lim, bool allow_splitting = false);
    std::pair<double, std::vector<std::vector<double>>> run(const Network& net);
};


#endif //NLP_OPTIMIZATION_BEECOLONY_H
