cmake_minimum_required(VERSION 4.1)
project(NLP_optimization)

set(CMAKE_CXX_STANDARD 20)

add_executable(NLP_optimization main.cpp
        src/Network.cpp
        include/Network.h
        src/SNDLIBParser.cpp
        include/SNDLIBParser.h
        src/BeeColony.cpp
        include/BeeColony.h
        src/AntColony.cpp
        include/AntColony.h
        src/BruteForce.cpp
        include/BruteForce.h)

set_target_properties(NLP_optimization PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        VS_DEBUGGER_COMMAND_ARGUMENTS "")
