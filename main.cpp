#include <iostream>
#include <string>
#include "include/Network.h"
#include "include/SNDLIBParser.h"
#include "include/TabuSearch.h"

int main() {
    std::string filename = "../SNDLIB_instances/yuan.txt";
    std::string output_folder = "taboosearch";

    Network net;

    if (load_instance(filename, net)) {
        net.generate_candidate_paths(3);
        net.display_data();

        if (!run_tabu_search(net, output_folder)) {
            std::cerr << "Tabu Search failed." << std::endl;
            return 1;
        }

        std::cout << "Tabu Search result saved to folder: " << output_folder << std::endl;
    } else {
        std::cerr << "Failed to open file" << filename << std::endl;
        return 1;
    }

    return 0;
}
