
#include "../include/SNDLIBParser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>

void clean_string(std::string& str) {
    for (char& c : str) {
        if (c == '(' || c == ')' || c == '{' || c == '}') {
            c = ' ';
        }
    }
}

std::vector<std::string> get_words(const std::string& str) {
    std::vector<std::string> words;
    std::stringstream ss(str);
    std::string word;
    while (ss >> word) {
        words.push_back(word);
    }
    return words;
}

bool load_instance(const std::string& filename, Network& net) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    std::string line;
    std::string current_section = "";

    std::unordered_map<std::string, int> node_name_to_id;
    int node_counter = 0;
    int link_counter = 0;
    int demand_counter = 0;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        clean_string(line);
        std::vector<std::string> words = get_words(line);
        if (words.empty()) continue;
        if (words[0] == "NODES") {
            current_section = "NODES";
            continue;
        } else if (words[0] == "LINKS") {
            current_section = "LINKS";
            net.set_nodes_number(node_counter);
            continue;
        } else if (words[0] == "DEMANDS") {
            current_section = "DEMANDS";
            continue;
        } else if (words[0] == "ADMISSIBLE_PATHS" || words[0] == "META") {
            current_section = "IGNORE";
            continue;
        }

        if (current_section == "NODES") {
            std::string node_name = words[0];

            if (node_name_to_id.find(node_name) == node_name_to_id.end()) {
                node_name_to_id[node_name] = node_counter++;
            }
        }
        else if (current_section == "LINKS") {

            int src_id = node_name_to_id[words[1]];
            int dst_id = node_name_to_id[words[2]];

            std::vector<CapacityModule> modules;
            for (size_t i = 7; i + 1 < words.size(); i += 2) {
                double cap = std::stod(words[i]);
                double cost = std::stod(words[i + 1]);
                modules.push_back({cap, cost});
            }

            net.add_link(link_counter++, src_id, dst_id, modules);
        }
        else if (current_section == "DEMANDS") {

            int src_id = node_name_to_id[words[1]];
            int dst_id = node_name_to_id[words[2]];
            double volume = std::stod(words[4]);

            net.add_demand(demand_counter++, src_id, dst_id, volume);
        }
    }

    file.close();
    return true;
}