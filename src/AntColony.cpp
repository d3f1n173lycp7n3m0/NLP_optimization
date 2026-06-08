#include "../include/Network.h"

#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

vector<vector<double>> algorytm_mrowkowy(Network& net, int liczba_mrowek, int liczba_iteracji)
{
    srand((unsigned int)time(NULL));

    for (int i = 0; i < (int)net.demands.size(); i++)
    {
        if (net.demands[i].candidate_paths.size() == 0)
        {
            net.generate_candidate_paths(3);
            break;
        }
    }


    vector<vector<double>> feromony;

    for (int i = 0; i < (int)net.demands.size(); i++)
    {
        vector<double> jeden;

        for (int j = 0; j < (int)net.demands[i].candidate_paths.size(); j++)
        {
            jeden.push_back(1.0);
        }

        feromony.push_back(jeden);
    }


    vector<vector<double>> najlepszy_wynik;
    double najleprzy_koszt = 999999999999999.0;

    double parowanie = 0.30;


    for (int iteracja = 0; iteracja < liczba_iteracji; iteracja++)
    {
        vector<vector<int>> sciezki_mrowek;
        vector<double> koszty_mrowek;

        for (int mrowka = 0; mrowka < liczba_mrowek; mrowka++)
        {
            vector<vector<double>> wynik;
            vector<int> wybrane_sciezki;

            for (int i = 0; i < (int)net.demands.size(); i++)
            {
                vector<double> jeden_demand;

                for (int j = 0; j < (int)net.demands[i].candidate_paths.size(); j++)
                {
                    jeden_demand.push_back(0.0);
                }

                wynik.push_back(jeden_demand);
            }


            for (int i = 0; i < (int)net.demands.size(); i++)
            {
                int ile_sciezek = (int)net.demands[i].candidate_paths.size();

                if (ile_sciezek == 0)
                {
                    wybrane_sciezki.push_back(-1);
                }
                else
                {
                    vector<double> szansa;
                    double suma = 0.0;

                    for (int j = 0; j < ile_sciezek; j++)
                    {
                        double koszt_drogi = net.demands[i].candidate_paths[j].routing_cost;

                        if (koszt_drogi <= 0.0)
                        {
                            koszt_drogi = 1.0; //t o by nie było dzielenia przez 0
                        }

                        double x = feromony[i][j] / koszt_drogi; // jak bardzo mrówka che iść ta drogą

                        if (x < 0.000001)
                        {
                            x = 0.000001; //jeśli było by 0 mrówki nigdy by nie wybrały
                        }

                        szansa.push_back(x);
                        suma = suma + x;
                    }

                    int wybrana = 0;

                    if (suma <= 0.0)
                    {
                        wybrana = rand() % ile_sciezek;
                    }
                    else
                    {
                        double los = ((double)rand() / (double)RAND_MAX) * suma;
                        double teraz = 0.0;

                        for (int j = 0; j < ile_sciezek; j++)
                        {
                            teraz = teraz + szansa[j];

                            if (los <= teraz)
                            {
                                wybrana = j;
                                break;
                            }
                        }
                    }

                    wybrane_sciezki.push_back(wybrana);
                    wynik[i][wybrana] = net.demands[i].volume;
                }
            }


            double koszt = net.evaluate(wynik);

            for (int i = 0; i < (int)wybrane_sciezki.size(); i++)
            {
                if (wybrane_sciezki[i] == -1)
                {
                    koszt = koszt + 100000000.0;
                }
            }

            sciezki_mrowek.push_back(wybrane_sciezki);
            koszty_mrowek.push_back(koszt);

            if (koszt < najleprzy_koszt)
            {
                najleprzy_koszt = koszt;
                najlepszy_wynik = wynik;
            }
        }


        for (int i = 0; i < (int)feromony.size(); i++)
        {
            for (int j = 0; j < (int)feromony[i].size(); j++)
            { // częsc feromonów paruje 
                feromony[i][j] = feromony[i][j] * (1.0 - parowanie); /// to by nie pamiętał tych gorszych wyborów
               

                if (feromony[i][j] < 0.01)
                {
                    feromony[i][j] = 0.01;
                }
            }
        }

//tu dodawanie feromonu dla najlepszych ścieżek
        for (int mrowka = 0; mrowka < (int)sciezki_mrowek.size(); mrowka++)
        {
            double dodatek = 0.0;

            if (koszty_mrowek[mrowka] > 0.0)
            {
                dodatek = 1000.0 / koszty_mrowek[mrowka];
            }

            for (int i = 0; i < (int)sciezki_mrowek[mrowka].size(); i++)
            {
                int nr = sciezki_mrowek[mrowka][i];

                if (nr >= 0)
                {
                    feromony[i][nr] = feromony[i][nr] + dodatek;
                }
            }
        }


        if (iteracja % 10 == 0)
        {
            cout << "iteracja " << iteracja << " koszt " << najleprzy_koszt << endl;
        }
    }

    return najlepszy_wynik;
}


void wypisz_mrowkowy(Network& net, vector<vector<double>> wynik)
{
    double koszt = net.evaluate(wynik);

    cout << endl;
    cout << "wynik mrowek koszt = " << koszt << endl;

    for (int i = 0; i < (int)wynik.size(); i++)
    {
        cout << "demand " << net.demands[i].id << " sciezka: ";

        for (int j = 0; j < (int)wynik[i].size(); j++)
        {
            if (wynik[i][j] > 0.0)
            {
                cout << j << " linki: ";

                for (int k = 0; k < (int)net.demands[i].candidate_paths[j].links_ids.size(); k++)
                {
                    cout << net.demands[i].candidate_paths[j].links_ids[k] << " ";
                }
            }
        }

        cout << endl;
    }
}
