
#ifndef NLP_OPTIMIZATION_PARSER_H
#define NLP_OPTIMIZATION_PARSER_H

#include <string>
#include "Network.h"

bool load_instance(const std::string& filename, Network& net);
bool load_model(const std::string& filename, Network& net);

#endif //NLP_OPTIMIZATION_PARSER_H
