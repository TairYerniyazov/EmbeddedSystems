#ifndef RESOURCE_ALLOCATOR_H
#define RESOURCE_ALLOCATOR_H

#include <cmath>
#include <iostream>
#include <string>
#include "matrix.hpp"
#include <cmath>
#include <vector>
#include <random>

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

struct PE {
  double totalActiveTime;
  int totalNumOfJobs;
  double lastTaskStartTime;
  double lastTaskEndTime;
  PE(double totalActiveTime_, int totalNumOfJobs_, double lastTaskStartTime_,
  double lastTaskEndTime_) : totalActiveTime{totalActiveTime_},
  totalNumOfJobs{totalNumOfJobs_}, lastTaskStartTime{lastTaskStartTime_},
  lastTaskEndTime{lastTaskEndTime_} {}
};

struct AllocatedPEs {
  std::vector<std::vector<PE>> *elem;
  int id;
  AllocatedPEs(int nPEs) { elem = new std::vector<std::vector<PE>>(nPEs); }
  ~AllocatedPEs() { delete elem; }
};

struct Task {
  int id;
  int resourceID = -1;
  int resourceCopyIndex;
  double startTime;
  double endTime;
  Task() {}
  Task(int id_, int resourceID_, double startTime_, double endTime_, 
    int resourceCopyIndex_) : id{id_}, resourceID{resourceID_}, 
    resourceCopyIndex{resourceCopyIndex_},
    startTime{startTime_}, endTime{endTime_} {}
  ~Task() {}
};

class ResourceAllocator {
 private:
  AllocatedPEs *allocatedSoFar;
  std::vector<Task> *tasks;
  Matrix<double> proc;
  Matrix<double> cost;
  Matrix<double> times;
  Matrix<double> procStd;
  Matrix<double> costStd;
  Matrix<double> timesStd;
  Matrix<bool> taskAdjacencyMatrix;
  int nTasks;
  int nPEs;
  double maxTime;
  double z_std;
  std::vector<std::vector<int>> *lastChoice;
  int find_minTime(int taskID) {
    double minTime = times[taskID][0];
    int bestResourceID = 0;
    for (int i = 0; i < nPEs; ++i)
      if (times[taskID][i] < minTime) {
        minTime = times[taskID][i];
        bestResourceID = i;
      }
    return bestResourceID;
  }
  int findBest_std(int taskID) {
    std::vector<double> overallValues{};
    for (int i = 0; i < proc.d1; ++i)
      overallValues.push_back(computeUsingStd(procStd[i][0], costStd[taskID][i],
                                              timesStd[taskID][i], 1.0, 1.0, z_std));
    int bestResourceID = 0;
    double minValue = overallValues[bestResourceID];
    for (int i = 0; i < (int)overallValues.size(); ++i) {
      if (overallValues[i] < minValue) {
        minValue = overallValues[i];
        bestResourceID = i;
      }
    }
    return bestResourceID;
  }
  int findBest_timeCost(int taskID) {
    std::vector<double> overallValues{};
    for (int i = 0; i < proc.d1; ++i)
      overallValues.push_back(times[taskID][i] * cost[taskID][i]);
    int bestResourceID = 0;
    double minValue = overallValues[bestResourceID];
    for (int i = 0; i < (int)overallValues.size(); ++i) {
      if (overallValues[i] < minValue) {
        minValue = overallValues[i];
        bestResourceID = i;
      }
    }
    return bestResourceID;
  }
  int* find_minTimeOnAllocatedPEs(int taskID, int* results) {
    int firstPEavailable;
    for (int i = 0; i < nPEs; ++i) {
      if (allocatedSoFar->elem->operator[](i).size() > 0)
        firstPEavailable = i;
    }
    double minTime = times[taskID][firstPEavailable];
    int bestResourceID = firstPEavailable;
    for (int i = 0; i < nPEs; ++i) {
      if (allocatedSoFar->elem->operator[](i).size() > 0 &&
        times[taskID][i] < minTime) {
          bestResourceID = i;
          minTime = times[taskID][i]; 
        }
    }
    int bestIDOnTheList = 0;
    minTime = allocatedSoFar->elem->operator[](bestResourceID)[0].lastTaskEndTime;
    for (int i = 0; i < (int)allocatedSoFar->elem->operator[](bestResourceID).size(); ++i)
      if (allocatedSoFar->elem->operator[](bestResourceID)[i].lastTaskEndTime < minTime) {
        minTime = allocatedSoFar->elem->operator[](bestResourceID)[i].lastTaskEndTime;
        bestIDOnTheList = i;
      }
    results[0] = bestResourceID;
    results[1] = bestIDOnTheList;
    // std::cout << "bestResourceID " << bestResourceID << '\n';
    // std::cout << "bestIDOnTheList " << bestIDOnTheList << '\n';
    return results;
  }
  int find_minCost(int taskID) {
    double minCost = cost[taskID][0];
    int bestResourceID = 0;
    for (int i = 0; i < nPEs; ++i)
      if (cost[taskID][i] < minCost) {
        minCost = cost[taskID][i];
        bestResourceID = i;
      }
    return bestResourceID;
  }
  int* find_minCostOnAllocatedPEs(int taskID, int* results) {
    int firstPEavailable;
    for (int i = 0; i < nPEs; ++i) {
      if (allocatedSoFar->elem->operator[](i).size() > 0) firstPEavailable = i;
    }
    double minCost = cost[taskID][firstPEavailable];
    int bestResourceID = firstPEavailable;
    for (int i = 0; i < nPEs; ++i) {
      if (allocatedSoFar->elem->operator[](i).size() > 0 &&
          cost[taskID][i] < minCost) {
        bestResourceID = i;
        minCost = cost[taskID][i];
      }
    }
    int bestIDOnTheList = 0;
    int minTime =
        allocatedSoFar->elem->operator[](bestResourceID)[0].lastTaskEndTime;
    for (int i = 0; i < (int)allocatedSoFar->elem->operator[](bestResourceID).size();
         ++i)
      if (allocatedSoFar->elem->operator[](bestResourceID)[i].lastTaskEndTime <
          minTime) {
        minTime =
            allocatedSoFar->elem->operator[](bestResourceID)[i].lastTaskEndTime;
        bestIDOnTheList = i;
      }
    results[0] = bestResourceID;
    results[1] = bestIDOnTheList;
    // std::cout << "bestResourceID " << bestResourceID << '\n';
    // std::cout << "bestIDOnTheList " << bestIDOnTheList << '\n';
    return results;
  }
  int* find_leastUsedOnAllocatedPEs(int taskID, int* results) {
    int firstPEavailable;
    for (int i = 0; i < nPEs; ++i) {
      if (allocatedSoFar->elem->operator[](i).size() > 0) firstPEavailable = i;
    }
    double minUsage =
        allocatedSoFar->elem->operator[](firstPEavailable)[0].totalNumOfJobs;
    int bestResourceID = firstPEavailable;
    int bestIDOnTheList = 0;
    for (int i = 0; i < nPEs; ++i) {
      for (int j = 0; j < (int)allocatedSoFar->elem->operator[](i).size(); ++j)
        if (allocatedSoFar->elem->operator[](i)[j].totalNumOfJobs < minUsage) {
          minUsage = allocatedSoFar->elem->operator[](i)[j].totalNumOfJobs;
          bestResourceID = i;
          bestIDOnTheList = j;
        }
    }
    results[0] = bestResourceID;
    results[1] = bestIDOnTheList;
    // std::cout << "bestResourceID " << bestResourceID << '\n';
    // std::cout << "bestIDOnTheList " << bestIDOnTheList << '\n';
    return results;
  }
  int* find_leastWorkingTimeOnAllocatedPEs(int taskID, int* results) {
    int firstPEavailable;
    for (int i = 0; i < nPEs; ++i) {
      if (allocatedSoFar->elem->operator[](i).size() > 0) firstPEavailable = i;
    }
    double minTimeWorking =
        allocatedSoFar->elem->operator[](firstPEavailable)[0].totalActiveTime;
    int bestResourceID = firstPEavailable;
    int bestIDOnTheList = 0;
    for (int i = 0; i < nPEs; ++i) {
      for (int j = 0; j < (int)allocatedSoFar->elem->operator[](i).size(); ++j)
        if (allocatedSoFar->elem->operator[](i)[j].totalActiveTime <
            minTimeWorking) {
          minTimeWorking =
              allocatedSoFar->elem->operator[](i)[j].totalActiveTime;
          bestResourceID = i;
          bestIDOnTheList = j;
        }
    }
    results[0] = bestResourceID;
    results[1] = bestIDOnTheList;
    // std::cout << "bestResourceID " << bestResourceID << '\n';
    // std::cout << "bestIDOnTheList " << bestIDOnTheList << '\n';
    return results;
  }
  int* find_inactiveForVeryLongOnAllocatedPEs(int taskID, double startTime, int* results) {
    int firstPEavailable;
    for (int i = 0; i < nPEs; ++i) {
      if (allocatedSoFar->elem->operator[](i).size() > 0) firstPEavailable = i;
    }
    double timeOfInactivity = std::abs(startTime - 
        allocatedSoFar->elem->operator[](firstPEavailable)[0].lastTaskEndTime);
    int bestResourceID = firstPEavailable;
    int bestIDOnTheList = 0;
    for (int i = 0; i < nPEs; ++i) {
      for (int j = 0; j < (int)allocatedSoFar->elem->operator[](i).size(); ++j)
        if (std::abs(startTime - allocatedSoFar->elem->operator[](i)[j].lastTaskEndTime) 
          < timeOfInactivity) {
          timeOfInactivity = std::abs(
            startTime - allocatedSoFar->elem->operator[](i)[j].lastTaskEndTime);
          bestResourceID = i;
          bestIDOnTheList = j;
        }
    }
    results[0] = bestResourceID;
    results[1] = bestIDOnTheList;
    // std::cout << "bestResourceID " << bestResourceID << '\n';
    // std::cout << "bestIDOnTheList " << bestIDOnTheList << '\n';
    return results;
  }
  int* find_lastChoiceForParent(int taskID, int* results) {
    results[0] = lastChoice->operator[](taskID)[0];
    results[1] = lastChoice->operator[](taskID)[1];
    return results;
  }
  double computeUsingStd(double p, double c, double t, double x, double y,
                         double z) {
    return x * p + y * c + z * t;
  }
  double findStartTime(int taskID) {
    // Recursive limit case (no parents)
    int nParents = 0;
    for (int i = 0; i < nTasks; ++i)
      if (taskAdjacencyMatrix[i][taskID]) {
        nParents++;
      }
    // std::cout << "Task #" << taskID << " has " << nParents << " parents\n";
    if (nParents == 0)
      return 0;
    // Recursive body
    std::vector<double> parentsTimes{};
    std::vector<int> parentsIDs{};
    for (int i = 0; i < nTasks; ++i)
      if (taskAdjacencyMatrix[i][taskID]) {
        parentsTimes.push_back(tasks->operator[](i).endTime);
        parentsIDs.push_back(i);
      }
    double minTime = parentsTimes[0];
    int parentID = parentsIDs[0];
    for (int i = 0; i < (int)parentsTimes.size(); ++i)
      if (parentsTimes[i] < minTime) {
        minTime = parentsTimes[i];
        parentID = i;
      }
    // std::cout << "Parent #" << parentID << ".endTime: " << 
      // tasks->operator[](parentID).endTime << "\n";
    lastChoice->operator[](taskID).push_back(tasks->operator[](parentID).resourceID);
    lastChoice->operator[](taskID).push_back(tasks->operator[](parentID).resourceCopyIndex);
    return tasks->operator[](parentID).endTime;
  }
  void updateStd() {
    double lastTaskTime = 0;
    for (int i = 0; i < nPEs; ++i) {
      for (int j = 0; j < (int)allocatedSoFar->elem->operator[](i).size(); ++j) {
        double lastTaskEndTime = allocatedSoFar->elem->operator[](i)[j].lastTaskEndTime;
        if (lastTaskEndTime > lastTaskTime)
          lastTaskTime = lastTaskEndTime;
      }
    }
    if ((lastTaskTime / maxTime) >= 0.5)
      z_std *= 1 + (lastTaskTime / maxTime * 0.2);
    // std::cout << "The time coefficient is now " << z_std << '\n';
  }
  int find_fastestPP(int taskID) {
    int firstPPid;
    for (int i = 0; i < proc.d1; ++i)
      if (proc[i][2] == 1)
        firstPPid = i;
    double minTime = times[taskID][firstPPid];
    int bestResourceID = firstPPid;
    for (int i = 0; i < nPEs; ++i)
      if (times[taskID][i] < minTime && proc[i][2] == 1) {
        minTime = times[taskID][i];
        bestResourceID = i;
      }
    return bestResourceID;
  }
 public:
  ResourceAllocator(const Matrix<double>& proc_, const Matrix<double>& cost_,
                    const Matrix<double>& times_, const Matrix<bool>& tAM,
                    double maxTime_)
      : proc{proc_},
        cost{cost_},
        times{times_},
        taskAdjacencyMatrix{tAM},
        nTasks{cost_.d1},
        nPEs{proc_.d1},
        maxTime{maxTime_},
        z_std{1.0} {
    allocatedSoFar = new AllocatedPEs(proc_.d1);
    tasks = new std::vector<Task>(cost_.d1);
    procStd = standardiseData(proc, true);
    costStd = standardiseData(cost, false);
    timesStd = standardiseData(times, false);
    lastChoice = new std::vector<std::vector<int>>(nTasks);
  }
  ~ResourceAllocator() { 
    delete allocatedSoFar; 
    delete tasks;  
    delete lastChoice;
  }
  void allocateResource(int taskID, int choice) {
    if (tasks->operator[](taskID).resourceID == -1) {
      bool considerAllocatedOnly = false;
      double startTime = findStartTime(taskID);
      int results[2];
      // =========== Different ways of choosing the next PEs =========
      int bestResourceID;
      if (taskID == 0) {
        bestResourceID = find_fastestPP(taskID);
        // std::cout << "find_fastestPP\n";
      } else {
        // std::cout << choice << '\n';
        if (choice <= 200) {
          bestResourceID = findBest_std(taskID);
          // std::cout << "findBest_std\n";
        }
        if (choice > 200 && choice <= 300) {
          bestResourceID = findBest_timeCost(taskID);
          // std::cout << "findBest_timeCost\n";
        }
        if (choice > 300 && choice <= 325) {
          bestResourceID = find_minTime(taskID);
          // std::cout << "find_minTime\n";
        }
        if (choice > 325 && choice <= 350) {
          bestResourceID = find_minTimeOnAllocatedPEs(taskID, results)[0];
          considerAllocatedOnly = true;
          // std::cout << "find_minTimeOnAllocatedPEs\n";
        }
        if (choice > 350 && choice <= 375) {
          bestResourceID = find_minCost(taskID);
          // std::cout << "find_minCost\n";
        }
        if (choice > 375 && choice <= 400) {
          bestResourceID = find_minCostOnAllocatedPEs(taskID, results)[0];
          considerAllocatedOnly = true;
          // std::cout << "find_minCostOnAllocatedPEs\n";
        }
        if (choice > 400 && choice <= 450) {
          bestResourceID = find_leastUsedOnAllocatedPEs(taskID, results)[0];
          considerAllocatedOnly = true;
          // std::cout << "find_leastUsedOnAllocatedPEs\n";
        }
        if (choice > 450 && choice <= 550) {
          bestResourceID =
              find_leastWorkingTimeOnAllocatedPEs(taskID, results)[0];
          considerAllocatedOnly = true;
          // std::cout << "find_leastWorkingTimeOnAllocatedPEs\n";
        }
        if (choice > 550 && choice <= 850) {
          updateStd();
          bestResourceID = findBest_std(taskID);
          // std::cout << "update + findBest_std\n";
        }
        if (choice > 850 && choice <= 950) {
          bestResourceID = find_inactiveForVeryLongOnAllocatedPEs(
              taskID, startTime, results)[0];
          considerAllocatedOnly = true;
          // std::cout << "find_inactiveForVeryLongOnAllocatedPEs\n";
        }
        if (choice > 950) {
          bestResourceID = find_lastChoiceForParent(taskID, results)[0];
          considerAllocatedOnly = true;
          // std::cout << "find_lastChoiceForParent\n";
        }
      }
      // =============================================================
      std::vector<PE> *allocResList =
          &allocatedSoFar->elem->operator[](bestResourceID);
      bool isAvailable = false; 
      int idOnTheList = allocResList->size();
      double time = times[taskID][bestResourceID];
      if (allocResList->size() != 0)
        for (int t = 0; t < (int)allocResList->size(); ++t)
          if (allocResList->operator[](t).lastTaskEndTime <= startTime) {
            isAvailable = true;
            idOnTheList = t;
          }

      if (considerAllocatedOnly)
        idOnTheList = results[1];
      
      // std::cout << "lastTaskEndTime " << time << " for taskID " << taskID << '\n';
      if (!isAvailable && !considerAllocatedOnly) {
        // std::cout << "Type " << bestResourceID << " is not available\n";
        allocatedSoFar->elem->operator[](bestResourceID)
            .push_back(PE(0, 0, startTime, startTime + time));
        tasks->operator[](taskID) =
            Task(taskID, bestResourceID, startTime, startTime + time, 
              idOnTheList);
      } else {
        // std::cout << bestResourceID << " is available (copy #" << idOnTheList
                  // << ")\n";
        startTime = (allocResList->operator[](idOnTheList).lastTaskEndTime <= 
          startTime) ? startTime : 
          allocResList->operator[](idOnTheList).lastTaskEndTime;
        tasks->operator[](taskID) =
          Task(taskID, bestResourceID, startTime, startTime + time, 
            idOnTheList);
      }
      allocResList->operator[](idOnTheList).lastTaskStartTime = startTime;
      allocResList->operator[](idOnTheList).lastTaskEndTime = 
        startTime + time;
      allocResList->operator[](idOnTheList).totalActiveTime += time;
      allocResList->operator[](idOnTheList).totalNumOfJobs++;
      std::cout << "T" << taskID << " -> PP" << bestResourceID << "_" << 
        idOnTheList << '\n';
      // debug();
    }
  }
  void debug() {
    for (int i = 0; i < nPEs; ++i) {
      int idOnTheList = 0;
      for (auto e : allocatedSoFar->elem->operator[](i)) {
        std::cout << "PE" << i << "_" << idOnTheList << '\n';
        std::cout << "  totalActiveTime: " << e.totalActiveTime << '\n';
        std::cout << "  lastTaskStartTime: " << e.lastTaskStartTime << '\n';
        std::cout << "  lastTaskEndTime: " << e.lastTaskEndTime << '\n';
        std::cout << "  totalNumOfJubs: " << e.totalNumOfJobs << "\n\n";
        idOnTheList++;
      }
    }
  }
  double getLastTime() {
    double lastTaskTime = 0;
    for (int i = 0; i < nPEs; ++i) {
      for (int j = 0; j < (int)allocatedSoFar->elem->operator[](i).size(); ++j) {
        double lastTaskEndTime =
            allocatedSoFar->elem->operator[](i)[j].lastTaskEndTime;
        if (lastTaskEndTime > lastTaskTime) lastTaskTime = lastTaskEndTime;
      }
    }
    return lastTaskTime;
  }
};

#endif