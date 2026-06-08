#include "../include/SNDLIBParser.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

static void clean_string(std::string &text)
{
    for (char &character : text)
    {
        if (character == '(' || character == ')' ||
            character == '{' || character == '}' ||
            character == ':' || character == ';' ||
            character == '=')
        {
            character = ' ';
        }
    }
}

static std::vector<std::string> split_into_words(const std::string &text)
{
    std::vector<std::string> words;
    std::stringstream stream(text);
    std::string word;

    while (stream >> word)
    {
        words.push_back(word);
    }

    return words;
}

static bool set_link_capacity_model(
    const std::string &value,
    Network &network)
{
    if (value == "LINEAR_LINK_CAPACITIES")
    {
        network.set_link_capacity_model(Network::LinkCapacityModel::LINEAR_LINK_CAPACITIES);
        return true;
    }

    if (value == "SINGLE_MODULAR_CAPACITIES")
    {
        network.set_link_capacity_model(Network::LinkCapacityModel::SINGLE_MODULAR_CAPACITIES);
        return true;
    }

    if (value == "MODULAR_LINK_CAPACITIES")
    {
        network.set_link_capacity_model(Network::LinkCapacityModel::MODULAR_LINK_CAPACITIES);
        return true;
    }

    if (value == "EXPLICIT_LINK_CAPACITIES")
    {
        network.set_link_capacity_model(Network::LinkCapacityModel::EXPLICIT_LINK_CAPACITIES);
        return true;
    }

    return false;
}

static bool apply_model_value(
    const std::string &key,
    const std::string &value,
    Network &network)
{
    if (key == "LINK_MODEL" || key == "linkModel")
    {
        if (value == "DIRECTED")
        {
            network.set_link_model(Network::LinkModel::DIRECTED);
            return true;
        }
        if (value == "UNDIRECTED")
        {
            network.set_link_model(Network::LinkModel::UNDIRECTED);
            return true;
        }
        if (value == "BIDIRECTED")
        {
            network.set_link_model(Network::LinkModel::BIDIRECTED);
            return true;
        }
    }

    if (key == "NODE_MODEL" || key == "nodeModel")
    {
        if (value == "NODE_HARDWARE")
        {
            network.set_node_hardware(true);
            return true;
        }
        if (value == "NO_NODE_HARDWARE")
        {
            network.set_node_hardware(false);
            return true;
        }
    }

    if (key == "DEMAND_MODEL" || key == "demandModel")
    {
        if (value == "DIRECTED")
        {
            network.set_demand_model(Network::DemandModel::DIRECTED);
            return true;
        }
        if (value == "UNDIRECTED")
        {
            network.set_demand_model(Network::DemandModel::UNDIRECTED);
            return true;
        }
    }

    if (key == "LINK_CAPACITY_MODEL" || key == "linkCapacityModel")
    {
        return set_link_capacity_model(value, network);
    }

    if (key == "ROUTING_MODEL" || key == "routingModel")
    {
        if (value == "CONTINUOUS")
        {
            network.set_routing_model(Network::RoutingModel::CONTINUOUS);
            return true;
        }
        if (value == "INTEGER")
        {
            network.set_routing_model(Network::RoutingModel::INTEGER);
            return true;
        }
        if (value == "SINGLE_PATH" || value == "OSPF_SINGLE_PATH")
        {
            network.set_routing_model(Network::RoutingModel::SINGLE_PATH);
            return true;
        }
    }

    if (key == "ADMISSIBLE_PATH_MODEL" || key == "admissiblePathModel")
    {
        if (value == "ALL_PATHS")
        {
            network.set_admissible_path_model(Network::AdmissiblePathModel::ALL_PATHS);
            return true;
        }
        if (value == "EXPLICIT_LIST")
        {
            network.set_admissible_path_model(Network::AdmissiblePathModel::EXPLICIT_LIST);
            return true;
        }
    }

    if (key == "HOP_LIMIT_MODEL" || key == "hopLimitModel")
    {
        if (value == "IGNORE_HOP_LIMITS")
        {
            network.set_hop_limit_model(Network::HopLimitModel::IGNORE_HOP_LIMITS);
            return true;
        }
        if (value == "INDIVIDUAL_HOP_LIMITS")
        {
            network.set_hop_limit_model(Network::HopLimitModel::INDIVIDUAL_HOP_LIMITS);
            return true;
        }
    }

    if (key == "SURVIVABILITY_MODEL" || key == "survivabilityModel")
    {
        if (value == "NO_SURVIVABILITY")
        {
            network.set_survivability_model(Network::SurvivabilityModel::NO_SURVIVABILITY);
            return true;
        }
        if (value == "ONE_PLUS_ONE_PROTECTION")
        {
            network.set_survivability_model(Network::SurvivabilityModel::ONE_PLUS_ONE_PROTECTION);
            return true;
        }
        if (value == "SHARED_PATH_PROTECTION")
        {
            network.set_survivability_model(Network::SurvivabilityModel::SHARED_PATH_PROTECTION);
            return true;
        }
        if (value == "UNRESTRICTED_FLOW_RECONFIGURATION")
        {
            network.set_survivability_model(Network::SurvivabilityModel::UNRESTRICTED_FLOW_RECONFIGURATION);
            return true;
        }
    }

    if (key == "FIXED_CHARGE_MODEL" || key == "fixedChargeModel")
    {
        if (value == "YES")
        {
            network.set_fixed_charge(true);
            return true;
        }
        if (value == "NO")
        {
            network.set_fixed_charge(false);
            return true;
        }
    }

    if (key == "UNIT_COST_MODULE_INDEX")
    {
        network.set_unit_cost_module_index(std::stoi(value));
        return true;
    }

    if (key == "DEMAND_SCALE")
    {
        network.demand_scale = std::stod(value);
        return true;
    }

    if (key == "COST_SCALE")
    {
        network.cost_scale = std::stod(value);
        return true;
    }

    if (key == "FIRST_MODULE_ONLY")
    {
        network.first_module_only = (value == "YES" || value == "TRUE" || value == "1");
        return true;
    }

    if (key == "INCLUDE_ROUTING_COST")
    {
        network.set_include_routing_cost(value == "YES" || value == "TRUE" || value == "1");
        return true;
    }

    if (key == "REFERENCE_COST")
    {
        network.reference_cost = std::stod(value);
        network.has_reference_cost = true;
        return true;
    }

    if (key == "CANDIDATE_PATHS")
    {
        network.candidate_paths = std::stoi(value);
        return true;
    }

    if (key == "POPULATION_SIZE")
    {
        network.population_size = std::stoi(value);
        return true;
    }

    if (key == "ITERATIONS" || key == "MAX_ITERATIONS")
    {
        network.iterations = std::stoi(value);
        return true;
    }

    if (key == "LIMIT")
    {
        network.limit = std::stoi(value);
        return true;
    }

    return false;
}

bool load_model(const std::string &filename, Network &network)
{
    std::ifstream file(filename);

    if (!file.is_open())
    {
        std::cerr << "Failed to open model file: " << filename << std::endl;
        return false;
    }

    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#' || line[0] == '?')
        {
            continue;
        }

        clean_string(line);
        std::vector<std::string> words = split_into_words(line);

        if (words.size() < 2)
        {
            continue;
        }

        apply_model_value(words[0], words[1], network);
    }

    file.close();
    return true;
}

bool load_instance(const std::string &filename, Network &network)
{
    std::ifstream file(filename);

    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }

    std::string line;
    std::string current_section;

    std::unordered_map<std::string, int> node_name_to_identifier;

    int node_counter = 0;
    int link_counter = 0;
    int demand_counter = 0;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        clean_string(line);
        std::vector<std::string> words = split_into_words(line);

        if (words.empty())
        {
            continue;
        }

        if (words[0] == "NODES")
        {
            current_section = "NODES";
            continue;
        }

        if (words[0] == "LINKS")
        {
            current_section = "LINKS";
            network.set_nodes_number(node_counter);
            continue;
        }

        if (words[0] == "DEMANDS")
        {
            current_section = "DEMANDS";
            continue;
        }

        if (words[0] == "ADMISSIBLE_PATHS" || words[0] == "META")
        {
            current_section = "IGNORE";
            continue;
        }

        if (words.size() > 1 && apply_model_value(words[0], words[1], network))
        {
            continue;
        }

        if (current_section == "NODES")
        {
            std::string node_name = words[0];

            if (node_name_to_identifier.find(node_name) == node_name_to_identifier.end())
            {
                node_name_to_identifier[node_name] = node_counter++;
            }
        }

        else if (current_section == "LINKS")
        {
            int source_node_identifier = node_name_to_identifier[words[1]];
            int target_node_identifier = node_name_to_identifier[words[2]];
            double routing_cost = std::stod(words[5]);

            std::vector<CapacityModule> capacity_modules;

            for (size_t word_index = 7; word_index + 1 < words.size(); word_index += 2)
            {
                double capacity = std::stod(words[word_index]);
                double cost = std::stod(words[word_index + 1]);

                capacity_modules.push_back({capacity, cost});
            }

            network.add_link(
                link_counter++,
                source_node_identifier,
                target_node_identifier,
                routing_cost,
                capacity_modules
            );
        }

        else if (current_section == "DEMANDS")
        {
            int source_node_identifier = node_name_to_identifier[words[1]];
            int target_node_identifier = node_name_to_identifier[words[2]];
            double routing_unit = std::stod(words[3]);
            double demand_value = std::stod(words[4]);
            double demand_volume = routing_unit * demand_value;

            if (!network.has_routing_unit_data)
            {
                network.has_routing_unit_data = true;
                network.min_routing_unit = routing_unit;
                network.max_routing_unit = routing_unit;
            }
            else
            {
                network.min_routing_unit =
                    std::min(network.min_routing_unit, routing_unit);
                network.max_routing_unit =
                    std::max(network.max_routing_unit, routing_unit);
            }

            network.add_demand(
                demand_counter++,
                source_node_identifier,
                target_node_identifier,
                demand_volume
            );
        }
    }

    file.close();
    return true;
}
