#ifndef NLP_OPTIMIZATION_ANTCOLONY_H
#define NLP_OPTIMIZATION_ANTCOLONY_H

#include "Network.h"

#include <vector>

std::vector<std::vector<double>> algorytm_mrowkowy(
    Network& net,
    int liczba_mrowek,
    int liczba_iteracji
);

void wypisz_mrowkowy(
    Network& net,
    std::vector<std::vector<double>> wynik
);

#endif // NLP_OPTIMIZATION_ANTCOLONY_H
