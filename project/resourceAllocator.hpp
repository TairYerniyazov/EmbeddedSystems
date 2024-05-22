#ifndef RESOURCE_ALLOCATOR_H
#define RESOURCE_ALLOCATOR_H

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <algorithm>
#include "matrix.hpp"
#include "utilities.hpp"

class ResourceAllocator {
 private:
  Matrix<bool> tasksAdjacencyMatrix;
  Matrix<double> proc;
  Matrix<double> times;
  Matrix<double> cost;
  Matrix<double> procStd;
  Matrix<double> costStd;
  Matrix<double> timesStd;
  std::vector<Channel> channels;
  std::vector<Task> tasks;
  std::vector<PE> resources;
  int nTasks;
  int nPEs;
  int nChannels;
  std::vector<int> PE_instances_ids;  // [nHC, nPP, nPE_1, nPE_2, ...]
  Matrix<double> tasksMatrix;
  double overallTime; // całkowity czas
  double overallCost; // całkowity koszt
  double t_max; // maksymalny czas
  double c_max; // maksymalny koszt
  std::vector<double> x_y_z; 
 public:
  ResourceAllocator(const Matrix<bool>& tAM, const Matrix<double>& proc_, 
  const Matrix<double>& times_, const Matrix<double>& cost_,
  const Matrix<double>& comm, const Matrix<double>& tasks_) : 
    tasksAdjacencyMatrix{tAM}, proc{proc_},
    times{times_}, cost{cost_}, nTasks{tAM.d1}, nPEs{proc_.d1},
    nChannels{comm.d1}, PE_instances_ids{std::vector<int>(2 + proc.d1)},
    tasksMatrix{tasks_}, overallTime{-1}, overallCost{-1}, t_max{-1} {
      for (int i = 0; i < nTasks; ++i)
        tasks.push_back(Task(i));
      for (int i = 0; i < nChannels; ++i) {
        channels.push_back(Channel(comm[i][0], comm[i][1], 
          std::vector<bool>(nPEs), i));
        for (int j = 0; j < nPEs; ++j)
          channels[i].connections[j] = comm[i][2 + j];
      }
      for (int i = 0; i < nPEs; ++i)
        if (proc[i][2] == 0)
          PE_instances_ids[0]++;
        else
          PE_instances_ids[1]++;
   x_y_z.push_back(1/3);   x_y_z.push_back(1/3);    x_y_z.push_back(1/3);   // trzykrotnie, poniewaz jest to wektor 3-elementowy
   // jezeli doftmax przyjmuje ten wektor, to zwraca nam nowy wektor (tak następuje aktualizacja)
    }
  ~ResourceAllocator() {}
  
  std::vector<int> findAllParents(int taskID) {
    // Znajduje wszystkich rodziców (bezpośrednich poprzedników rozważanego
    // zadania o numerze TaskID). Zwraca wektor zawierający numery tych zadań.
    std::vector<int> results{};
    for (int i = 0; i < nTasks; ++i)
      if (tasksAdjacencyMatrix[i][taskID])
        results.push_back(i);
    return results;
  }

  int findBest_std(int taskID) {
    // Znajduje najlepszy zasób zgodnie ze wzorem ComputeUsingStd
    std::vector<double> overallValues{};
    for (int i = 0; i < proc.d1; ++i)
      overallValues.push_back(computeUsingStd(procStd[i][0], costStd[taskID][i],
        timesStd[taskID][i], 1/3, 1/3, 1/3));
    int bestResourceID = 0;
    double minValue = overallValues[bestResourceID]; // tu znajdujemy wartosc minimalną (x*p + y*c + z*t) obliczaną z computeUsingStd (powyżej)
    for (int i = 0; i < (int)overallValues.size(); ++i) {
      if (overallValues[i] < minValue) {
        minValue = overallValues[i];
        bestResourceID = i;
      }
    }
    return bestResourceID;
  }

  // TODO: dodać funkcję aktualizującą współczynniki i normalizujące je
  // za pomocą Softmax.














  
  int findBestParent(int taskID) {
    // Znajduje rodzica, który kończy się wykonywać najwcześniej, więc
    // jest poprzednikiem, który dla dziecka wyznacza ścieżkę
    auto allParentsIDs = findAllParents(taskID);
    if (allParentsIDs.size() == 0)
      return -1;
    int bestParentID = allParentsIDs[0];
    double minEndTime = resources[tasks[bestParentID].resourceID].lastTaskEndTime;
    for (auto parentID : allParentsIDs) {
      double endTime = resources[tasks[parentID].resourceID].lastTaskEndTime;
      if (endTime < minEndTime) {
        bestParentID = parentID;
        minEndTime = endTime;
      }
    }
    return bestParentID;
  }

  int findBestChannel(int parentID, int childID) {
    // Znalezienie nalepszej szyny danych, czyli takiej, która zapewnia
    // łączność między poprzednikiem a następnikiem. Najpierw sprawdzamy czy
    // poprzednik już nie był wcześniej podpięty do którejś z szyn spełniających
    // warunek łączności z rozważanym następnikiem. Jeśli takiej szyny nie było,
    // to podpinamy zarówno rodzica jak i dziecko do nowej szyny danych. Nowa
    // szyna danych wybierana jest wśród dostępnych i spełniających wymogi na
    // podstawie kosztu podpięcia (interesuje nas najmniejszy koszt podpięcia)
    if (parentID == -1) {
      int channID;
      for (auto channel : channels)
        if (channel.connections[resources[tasks[childID].resourceID].procID])
          channID = channel.id;
      double minCost = channels[channID].cost;
      for (auto channel : channels)
        if (channel.cost < minCost) {
          channID = channel.id;
          minCost = channel.cost;
        }
      return channID;
    } else {
      int parentProcID = resources[tasks[parentID].resourceID].procID;
      int childProcID = resources[tasks[childID].resourceID].procID;
      // Checking all the channels to which the parent has already been connected
      int choice = -1;
      for (auto channelID : resources[tasks[parentID].resourceID].channelIDs)
        if (channels[channelID].connections[childProcID])
          choice = channelID;
      int firstAvailableForChild;
      for (auto channel : channels)
        if (channel.connections[childProcID])
          firstAvailableForChild = channel.id;
      double minimum = 
        channels[choice != -1 ? choice : firstAvailableForChild].cost;
      for (auto channelID : resources[tasks[parentID].resourceID].channelIDs)
        if (channels[channelID].connections[childProcID] &&
          channels[channelID].cost < minimum) {
            minimum = channels[channelID].cost;
            choice = channelID;
          }
      if (choice != -1)
        return choice;
      // Checking all the channels
      int firstAvailableChannel;
      for (auto channel : channels) {
        auto connections = channel.connections;
        if (connections[parentProcID] && connections[childProcID])
          firstAvailableChannel = channel.id;
      }
      int idealChannel = firstAvailableChannel;
      // Wybór najtańszego z punktu widzenia dostępnych
      double minCost = channels[idealChannel].cost;
      for (auto channel : channels) {
        auto connections = channel.connections;
        if (connections[parentProcID] && connections[childProcID] &&
        channel.cost < minCost) {
          minCost = channel.cost;
          idealChannel = channel.id;
        }
      }
      return idealChannel;
    }
  }

  bool allParentsHaveResources(int taskID) {
    // Sprawdzenie czy wszystkie bezpośrednie poprzedniki rozważanego zadania
    // mają zaalokowane dla nich zasoby. Ta funkcja jest przydatna dla
    // rekurencyjnego przydziału zasobów (z dołu do góry wstecz, czyli jeśli
    // któryś z rodziców zadania nie posiada zasobu, to trzeba najpierw jemu
    // przydzielić, żeby móc wyznaczyć czas i, co najważniejsze, szynę danych)
    auto parentsIDs = findAllParents(taskID);
    for (auto parentID : parentsIDs)
      if (tasks[parentID].resourceID == -1)
        return false;
    return true;
  }

  bool canBeConnectedToBestParent(int taskID, int procID) {
    // Sprawdzenie czy da się podpiąć rozważane zadanie z najwcześniej kończącym
    // się poprzednikiem za pomocą którejś z szyn danych
    int nParents = findAllParents(taskID).size();
    if (nParents == 0)
      return true;
    int parentID = findBestParent(taskID);
    if (tasks[parentID].resourceID == -1)
      throw std::invalid_argument("Parent T" + std::to_string(parentID)
        + " does not have any resource allocated");
    int parentResourceProcID = resources[tasks[parentID].resourceID].procID;
    bool specificParentConnection = false;
    for (auto channel : channels) {
      auto connections = channel.connections;
      if (connections[parentResourceProcID] && connections[procID]) {
        std::cerr << "Connection between the parent PE (" << parentResourceProcID
          << ") and the current task (" << taskID << ") PE (" << procID
          << ") is possible on " << channel.id << '\n';  
        specificParentConnection = true;
        break;
      }
    }
    if (!specificParentConnection)
      return false;
    return true;
  }

  void recomputeAllPathsTime() {
    // Przeliczenie (aktualizacja odpowiednich pól) i wypisanie na wyjściu 
    for (int t = 0; t < nTasks; ++t) {
      auto resource = resources[tasks[t].resourceID];
      std::cerr << "Recomputing time for " << resource.label << '\n';
      int bestParentID = findBestParent(t);
      std::cerr << "Best parent for T" << t << " is " << bestParentID << '\n';
      if (bestParentID != -1) {
        auto taskChannelIDs = resource.channelIDs;
        auto parentChannelIDs = 
          resources[tasks[bestParentID].resourceID].channelIDs;
        int channelChoice = -1;
        
        std::cerr << "T" << t << " channels:\n";
        for (auto channel : resources[tasks[t].resourceID].channelIDs)
          std::cerr << channel << ' ';
        std::cerr << '\n';
        std::cerr << "T" << bestParentID << " (parent) channels:\n";
        for (auto channel : resources[tasks[bestParentID].resourceID].channelIDs)
          std::cerr << channel << ' ';
        std::cerr << '\n';

        for (auto task : taskChannelIDs) {
          if (std::find(parentChannelIDs.begin(), parentChannelIDs.end(), task)
            != parentChannelIDs.end())
            channelChoice = task;
        }
        // Best parent can be changed, so we need to make sure the parent can 
        // be connected to its new child
        if (channelChoice == -1) {
          int channel = findBestChannel(bestParentID, t);
          // std::cerr << "FindBestChannel for parent T" << bestParentID  << " and"
            // << " child T" << t << " : " << channel << '\n';
          if (std::find(taskChannelIDs.begin(), taskChannelIDs.end(), channel)
            == taskChannelIDs.end())
            resources[tasks[t].resourceID].channelIDs.push_back(channel);
          if (std::find(parentChannelIDs.begin(), parentChannelIDs.end(),
                        channel) == parentChannelIDs.end())
            resources[tasks[bestParentID].resourceID].channelIDs.push_back(channel);
          channelChoice = channel;
        }
        std::cerr << channelChoice << '\n';

        int minCost = channels[channelChoice].cost;
        for (auto channel : taskChannelIDs) {
          std::cerr << "T" << t << " has parent T"
                    << bestParentID << " [channel " << channel << "]\n";
          if (std::find(parentChannelIDs.begin(), parentChannelIDs.end(),
                        channel) != parentChannelIDs.end() &&
              channels[channel].cost < minCost) {
            channelChoice = channel;
          }
        }
        resources[tasks[t].resourceID].lastTaskStartTime = 
          resources[tasks[bestParentID].resourceID].lastTaskEndTime 
          + tasksMatrix[bestParentID][t] / channels[channelChoice].bandwidth;
        resources[tasks[t].resourceID].lastTaskEndTime = 
          resources[tasks[t].resourceID].lastTaskStartTime +
          times[t][resources[tasks[t].resourceID].procID];
        std::cerr << "T" << t << " startTime "
                  << resources[tasks[t].resourceID].lastTaskStartTime << '\n';
        std::cerr << "T" << t << " endTime "
                  << resources[tasks[t].resourceID].lastTaskEndTime << '\n';
      } else {
        resources[tasks[t].resourceID].lastTaskStartTime = 0;
        resources[tasks[t].resourceID].lastTaskEndTime =
            resources[tasks[t].resourceID].lastTaskStartTime + times[t][
              resources[tasks[t].resourceID].procID];
      }
    }
    std::cout << "Time intervals have been recomputed:\n";
    for (int t = 0; t < nTasks; ++t) {
      std::cout << "  T" << t << " -> " << resources[tasks[t].resourceID].label
                << " [startTime: "
                << resources[tasks[t].resourceID].lastTaskStartTime
                << ", endTime: "
                << resources[tasks[t].resourceID].lastTaskEndTime << "]\n";
    }
  }

  void recomputeOverallTimeAndCost() {
    // Przeliczenie całkowitego kosztu i czasu (aktualizacja pól prywatnych
    // overallTime i overallCost)
    overallTime = 0;
    for (auto task : tasks)
      if (resources[task.resourceID].lastTaskEndTime > overallTime)
        overallTime = resources[task.resourceID].lastTaskEndTime;
    overallCost = 0;
    for (int t = 0; t < nTasks; ++t) {
      overallCost += proc[resources[t].procID][0];
      overallCost += cost[t][resources[tasks[t].resourceID].procID];
    }
    for (auto task : tasks)
      for (auto channelID : resources[task.resourceID].channelIDs)
        overallCost += channels[channelID].cost;
  }

  void debug() {
    // Funkcja do debugowania. Wypisuje zawartość niektórych struktur, które
    // mamy w grafie zadań.
    std::cerr << "ResourceAllocator::debug()\n";
    std::cerr << "ResourceAllocator::tasks\n";
    for (auto task : tasks)
      std::cerr << "  id: " << task.id << "; reallocated: " 
      << (task.reallocated ? "true" : "false") << "; resourceID: " 
      << task.resourceID << '\n';
    std::cerr << "\nResourceAllocator::channels\n";
    for (int i = 0; i < (int)channels.size(); ++i) {
      std::cerr << "  name: CHAN" << i << ";\n    bandwidth: " 
        << channels[i].bandwidth << ";\n    cost: " 
        << channels[i].cost << "\n   connections: ";
      for (int j = 0; j < (int)channels[i].connections.size(); ++j)
        std::cerr << (channels[i].connections[j] ? "↑" : "↓") << ' ';
      std::cerr << "\n\n";
    }
  }

  // Getter'y i Setter'y do zwracania atrybutów prywatnych
  double getOverallTime() { return overallTime; }
  double getOverallCost() { return overallCost; }
  void setMaxTime(double t) { t_max = t; }
  double getMaxTime() { return t_max; }
};

#endif
