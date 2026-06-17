cmake_minimum_required(VERSION 4.1)
project(NLP_optimization)

set(CMAKE_CXX_STANDARD 20)

add_executable(NLP_optimization main.cpp
        src/Network.cpp
        include/Network.h
        src/SNDLIBParser.cpp
        include/SNDLIBParser.h
        src/TabuSearch.cpp
        include/TabuSearch.h)
