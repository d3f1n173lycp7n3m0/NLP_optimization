#ifndef NLP_OPTIMIZATION_TABUSEARCH_H
#define NLP_OPTIMIZATION_TABUSEARCH_H

#include <string>
#include "Network.h"

bool run_tabu_search(const Network& net, const std::string& output_folder, int max_iterations = 500, int tabu_tenure = 7);

#endif // NLP_OPTIMIZATION_TABUSEARCH_H
