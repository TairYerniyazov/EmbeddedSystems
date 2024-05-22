/* W tym pliku mamy funkcje i strukture pomocnicze */
#ifndef UTILITIES_H
#define UTILITIES_H

#include "matrix.hpp"

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

struct PE {
  double totalActiveTime;
  int totalNumOfJobs;
  double lastTaskStartTime;
  double lastTaskEndTime;
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
  int resourceID;
  bool reallocated;
  Task() : id{-1}, resourceID{-1}, reallocated{false} {}
  Task(int id_, int resourceID_, double pathTime_)
      : id{id_}, resourceID{resourceID_}, reallocated{false} {}
  Task(int id_) : Task(id_, -1, -1) {}
  ~Task() {}
};

// *****************************************************************************

struct Channel {
  double cost;
  double bandwidth;
  std::vector<bool> connections;
  int id;
  Channel(double c, double b, std::vector<bool> conn, int id_)
      : cost{c}, bandwidth{b}, connections{conn}, id{id_} {}
  ~Channel() {}
};

#endif