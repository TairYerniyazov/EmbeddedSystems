/* FUNKCJE I STRUKTURY POMOCNICZE */

#ifndef UTILITIES_H
#define UTILITIES_H

#include "matrix.hpp"
#include <iostream>
#include <cmath>
#include <vector>
#include <numeric>
#include <math.h>

auto standardiseData(Matrix<double> data, bool firstColumnOnly) {
  int d1 = data.d1;
  int d2 = firstColumnOnly ? 1 : data.d2;
  double mean = 0;
  for (int i = 0; i < d1; ++i)
    for (int j = 0; j < d2; ++j) mean += data[i][j];
  mean /= d1 * d2;
  double std = 0;
  for (int i = 0; i < d1; ++i)
    for (int j = 0; j < d2; ++j) std += std::pow(data[i][j] - mean, 2);
  std /= std::sqrt(std / (d1 * d2));
  for (int i = 0; i < d1; ++i)
    for (int j = 0; j < d2; ++j) data[i][j] = (data[i][j] - mean) / std;
  return data;
}

// *****************************************************************************

double computeUsingStd(double p, double c, double t, double x, double y,
                       double z) {
  // Zwraca wynik wzoru na sumę składowych zależnych od standaryzowanych
  // danych i współczynników.
  return x * p + y * c + z * t;
}

// *****************************************************************************

struct PE {
  double totalActiveTime; // ile do tej pory zasob byl uzywany
  int totalNumOfJobs; // dotychczasowa ilosc zadan
  double lastTaskStartTime; 
  double lastTaskEndTime; // kiedy zaczelo sie i skonczylo ostatnie zadanie
  int procID;
  std::string label;
  std::vector<int> channelIDs;
  PE(double totalActiveTime_, int totalNumOfJobs_, double lastTaskStartTime_,
     double lastTaskEndTime_, int procID_, std::string label_)
      : totalActiveTime{totalActiveTime_},
        totalNumOfJobs{totalNumOfJobs_},
        lastTaskStartTime{lastTaskStartTime_},
        lastTaskEndTime{lastTaskEndTime_},
        procID{procID_},
        label{label_},
        channelIDs{std::vector<int>()} {}
  PE(int procID_, std::string label_) : PE(-1, -1, -1, -1, procID_, label_) {}
};

// *****************************************************************************

struct Task {
  int id;
  int resourceID; // ID zasobu obliczeniowego przypisanego do zadania
  bool reallocated; // czy zasob zostal juz zaalokowany czy tez nie
  Task() : id{-1}, resourceID{-1}, reallocated{false} {}
  Task(int id_, int resourceID_, double pathTime_)
      : id{id_}, resourceID{resourceID_}, reallocated{false} {}
  Task(int id_) : Task(id_, -1, -1) {}
  ~Task() {}
};

// Informacje o kosztach są w tabelach costMatrix oraz procMatrix. 
// Informacje o koszcie uzyskujemy z tych tabel biorąc pod uwagę task -ID oraz 
// proc - ID.

// *****************************************************************************

struct Channel {
  double cost; // koszt szyny kounikacyjnej z tabeli 
  double bandwidth; 
  std::vector<bool> connections; // ?
  int id;
  Channel(double c, double b, std::vector<bool> conn, int id_)
      : cost{c}, bandwidth{b}, connections{conn}, id{id_} {}
  ~Channel() {}
};

// *****************************************************************************

std::vector<double> softmax(double a, double b, double c) {
  // Obliczamy wartości wykładnicze dla każdego współczynnika
  // std::exp jest używany do przekształcenia każdego elementu wejściowego
  // (współczynnika) w jego wartość wykładniczą.
  // Dzięki temu możemy później łatwo obliczyć prawdopodobieństwa, które sumują
  // się do 1.
  auto exp_a = std::exp(a);
  auto exp_b = std::exp(b);
  auto exp_c = std::exp(c);
  // Sumujemy wartości wykładnicze
  auto sum_exp = exp_a + exp_b + exp_c;
  // Obliczamy wartości softmax
  std::vector<double> result = {exp_a / sum_exp, exp_b / sum_exp, 
    exp_c / sum_exp};
  return result;
}

#endif
