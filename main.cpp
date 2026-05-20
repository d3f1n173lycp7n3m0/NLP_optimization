#include <iostream>
#include <string>
#include "include/Network.h"
#include "include/SNDLIBParser.h"

int main() {
    std::string filename = "../SNDLIB_instances/yuan.txt";

    Network net;

    if (load_instance(filename, net)) {
        net.generate_candidate_paths(3);
        net.display_data();

    } else {
        std::cerr << "Failed to open file" << filename << std::endl;
        return 1;
    }

    return 0;
}
