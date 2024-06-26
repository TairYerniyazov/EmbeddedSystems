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
  std::vector<Channel> channels; // wektor przechowujący wszystkie kanały w specyfikacji
  std::vector<Task> tasks; // wektor przechowujący wszystkie zadania w specyfikacji
  std::vector<PE> resources; // wszystkie do tej pory zaalokowane jednostki
  int nTasks; // wszystkich w specyfikacji 
  int nPEs; // wszystkich w specyfikacji
  int nChannels;
  std::vector<int> PE_instances_ids;  // [nHC, nPP, nPE_1, nPE_2, ...] //  ile zasobów zosało zaalokowanych do tej pory do każdego z typów. 
   // ten wektor moze wygladac w ten sposob : [2,3,0,20,5,4,6]  czyli 2*HC 3*PP a reszta elementów w wektorze wskazuje 
   // 0 razy użyliśmy HC_1, 20 razy użyliśmy HC_2, 5 razy użyliśmy PP_1 itd.
   // Naużytek pomocniczego pola label
  Matrix<double> tasksMatrix;
  double overallTime; // całkowity czas
  double overallCost; // całkowity koszt
  double t_max; // maksymalny czas
  double c_max; // maksymalny koszt
  std::vector<double> x_y_z; // wektor współczynników do standaryzacji
 public:
  ResourceAllocator(const Matrix<bool>& tAM, const Matrix<double>& proc_, 
  const Matrix<double>& times_, const Matrix<double>& cost_,
  const Matrix<double>& comm, const Matrix<double>& tasks_, 
  std::vector<bool> utm, double t_max_, double c_max_) : 
    tasksAdjacencyMatrix{tAM}, proc{proc_}, times{times_}, cost{cost_}, 
    nTasks{tAM.d1}, nPEs{proc_.d1}, nChannels{comm.d1}, 
    PE_instances_ids{std::vector<int>(2 + proc.d1)}, tasksMatrix{tasks_}, 
    overallTime{0}, overallCost{0}, t_max{t_max_}, c_max{c_max_} {
    for (int i = 0; i < nTasks; ++i)
      tasks.push_back(Task(i, utm[i]));
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
    // Standaryzacja tabel proc, times, cost
    procStd = standardiseData(proc, true);
    timesStd = standardiseData(times, false);
    costStd = standardiseData(cost, false);
    // Początkowe ustawienie współczynników
    for (int c = 0; c < 3; c++)
      x_y_z.push_back(1.0/3);     
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
    // Sprawdź jakie zasoby ze wszystkich da się podpiąć do rodzica
    std::vector<int> resourcesThatCanBeUsed{};
    for (int i = 0; i < nPEs; ++i)
      if (canBeConnectedToBestParent(taskID, i))
        resourcesThatCanBeUsed.push_back(i);
    // Liczenie wartości zgodnie ze wzorem ComputeUsingStd
    std::vector<double> overallValues{};
    for (auto e : resourcesThatCanBeUsed)
      overallValues.push_back(computeUsingStd(procStd[e][0], costStd[taskID][e],
        timesStd[taskID][e], x_y_z[0], x_y_z[1], x_y_z[2]));
    // Znalezienie najlepszego zasobu
    int bestResourceID = resourcesThatCanBeUsed[0];
    double minValue = overallValues[0]; // tu znajdujemy wartosc minimalną (x*p + y*c + z*t) obliczaną z computeUsingStd (powyżej)
    for (int i = 0; i < (int)overallValues.size(); ++i) {
      if (overallValues[i] < minValue) {
        minValue = overallValues[i];
        bestResourceID = resourcesThatCanBeUsed[i];
      }
    }
    return bestResourceID;
  }

  void updateCoefficients() {
    // Aktualizuje współczynniki i normalizuje je za pomocą Softmax
    // Wyliczenie liczby zadań, które nie mają przydzielonych zasobów
    double n = 0;
    for (int t = 0; t < nTasks; ++t)
      if (tasks[t].resourceID != -1)
        n++;
    // Liczenie "masy"
    auto m = 1 + n / nTasks;
    // Liczenie wektora prędkości
    auto v_proc_cost = overallCost / c_max - overallTime / t_max;
    auto v_times = overallTime / t_max - overallCost / c_max;
    // Liczenie wektora pędu
    auto p_proc_cost = m * v_proc_cost;
    auto p_times = m * v_times;
    // Przeliczenie współczynników
    std::cerr << "ResourceAllocator::updateCoefficients: Updating " 
              << "the coefficients\n";
    std::cerr << "  Old values: " << x_y_z[0] << " " << x_y_z[1] << " " 
              << x_y_z[2] << '\n';
    auto new_coeffs = softmax(x_y_z[0] + p_proc_cost, x_y_z[1] + p_proc_cost,
      x_y_z[2] + p_times);
    x_y_z = new_coeffs;
    std::cerr << "  New values: " << x_y_z[0] << " " << x_y_z[1] << " "
              << x_y_z[2] << '\n';
  }

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
        std::cerr << "ResourceAllocator::canBeConnectedToBestParent()\n" 
          << "  Connection between the parent PE (" << parentResourceProcID
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
        if (tasks[t].resourceID != tasks[bestParentID].resourceID) {
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
        }
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
      if (task.resourceID != -1 && resources[task.resourceID].lastTaskEndTime >
          overallTime)
        overallTime = resources[task.resourceID].lastTaskEndTime;
    overallCost = 0;
    for (auto task : tasks) {
      int id = task.resourceID;
      if (id != -1) {
        overallCost += proc[resources[id].procID][0];
        overallCost += cost[task.id][resources[id].procID];
        for (auto channelID : resources[id].channelIDs)
          overallCost += channels[channelID].cost;
      }
    } 
  }

  void allocate(int taskID) {
    // Alokuje zasób najlepszy z punktu widzenia findBest_std(taskID)
    if (tasks[taskID].resourceID == -1) {
      // Recursive bound
      if (allParentsHaveResources(taskID)) {
        std::cerr << "ResourceAllocator::allocate(): Allocating resources for T" 
          << taskID << '\n';
        // Updating the coefficients
        updateCoefficients();
        // Resource allocation
        int procID = findBest_std(taskID);
        // Sprawdzenie dostępności wybranego zasobu wśród już zaalokowanych
        int allParents = findAllParents(taskID).size();
        int bestParentID = allParents == 0 ? -1 : findBestParent(taskID);
        double bestParentEndTime = bestParentID == - 1 ? 0 : 
          resources[tasks[bestParentID].resourceID].lastTaskEndTime;
        bool useAvailablePE = false;
        for (int i = 0; i < (int)resources.size(); ++i) {
          auto r = resources[i];
          if (r.procID == procID)
            if ((bestParentEndTime == 0 && r.lastTaskEndTime == 0) || 
            (bestParentEndTime > 0 && bestParentEndTime > r.lastTaskEndTime)) {
              tasks[taskID].resourceID = i;
              useAvailablePE = true;
            }
        }
        std::string resourceLabel;
        if (useAvailablePE) {
          resourceLabel = resources[tasks[taskID].resourceID].label;
          std::cerr << "Available " << resources[tasks[taskID].resourceID].label
            << " will be used for " << "T" << taskID << '\n';
        } else {
          resourceLabel = "undefined_label";
          if (proc[procID][2] == 0)
            resourceLabel = "HC";
          else
            resourceLabel = "PP";
          int PE_type_count = 1;
          for (int i = 0; i < procID; ++i)
            if (proc[i][2] == proc[procID][2]) PE_type_count++;
          resourceLabel += std::to_string(PE_type_count) + "_" +
            std::to_string(PE_instances_ids[2 + procID]++);
          resources.push_back(PE(procID, resourceLabel));
          tasks[taskID].resourceID = resources.size() - 1;
        }

        int channelID;
        double startTime;
        bool sameResource =
            tasks[bestParentID].resourceID == tasks[taskID].resourceID;
        if (bestParentID == -1) {
          channelID = findBestChannel(-1, taskID);
          startTime = 0;
        } else {
          channelID = findBestChannel(bestParentID, taskID);
          startTime = (sameResource ? 0 : (tasksMatrix[bestParentID][taskID] /
                       channels[channelID].bandwidth)) +
                      resources[tasks[bestParentID].resourceID].lastTaskEndTime;
          if (!sameResource) {
            auto parentChannels =
                resources[tasks[bestParentID].resourceID].channelIDs;
            if (std::find(parentChannels.begin(), parentChannels.end(),
                          channelID) == parentChannels.end())
              resources[tasks[bestParentID].resourceID].channelIDs.push_back(
                  channelID);
          }
        }
        
        if (!useAvailablePE)
          resources[tasks[taskID].resourceID].channelIDs.push_back(channelID);
        double endTime = startTime + times[taskID][procID];
        resources[tasks[taskID].resourceID].lastTaskStartTime = startTime;
        resources[tasks[taskID].resourceID].lastTaskEndTime = endTime;
        std::cout << "  T" << taskID << " --> "
                  << resources[tasks[taskID].resourceID].label
                  << " [startTime: " << startTime << ", endTime: " << endTime
                  << "]\n";
        if (taskID == nTasks - 1) {
          std::cerr << '\n';
          for (int tID = 0; tID < nTasks; ++tID) {
            std::cerr << "ResourceAllocator::allocate:\n  T" << tID
                      << " resource is connected to channels: ";
            for (auto cID : resources[tasks[tID].resourceID].channelIDs)
              std::cerr << cID << " ";
            std::cerr << '\n';
          }
          std::cerr << '\n';
        }
        // Updating the overall time and cost
        recomputeOverallTimeAndCost();
      } else {
        // Recursive execution
        std::vector<int> parentsIDs = findAllParents(taskID);
        for (auto parentID : parentsIDs)
          if (tasks[parentID].resourceID == -1) allocate(parentID);
        allocate(taskID);
      }
    }
  }

  void debug() {
    // Funkcja do debugowania. Wypisuje zawartość niektórych struktur, które
    // mamy w grafie zadań.
    std::cerr << "ResourceAllocator::debug()\n";
    std::cerr << "ResourceAllocator::tasks\n";
    for (auto task : tasks)
      std::cerr << "  unpredicted: " << task.unpredicted << "; id: " << task.id 
      << "; reallocated: " 
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

  int findBest_timeCost(int taskID, bool unpredicted) {
    std::vector<double> overallValues{};
    std::vector<int> procIDs{};
    for (int i = 0; i < proc.d1; ++i)
      if (unpredicted) {
        if (proc[i][2] == 1) {
          overallValues.push_back(times[taskID][i] * cost[taskID][i]);
          procIDs.push_back(i);
        }
      } else {
        overallValues.push_back(times[taskID][i] * cost[taskID][i]);
        procIDs.push_back(i);
      }
    int bestResourceID = procIDs[0];
    double minValue = overallValues[bestResourceID];
    for (int i = 0; i < (int)overallValues.size(); ++i) {
      if (overallValues[i] < minValue) {
        minValue = overallValues[i];
        bestResourceID = procIDs[i];
      }
    }
    return bestResourceID;
  }

  double computeCriticalPath(int taskID) {
    int nNextTasks = 0;
    for (int i = 0; i < nTasks; ++i)
      if (tasksAdjacencyMatrix[taskID][i])
        nNextTasks++;
    // Recursive break
    if (nNextTasks == 0) {
      double singleJobTime = times[taskID][tasks[taskID].resourceID];
      tasks[taskID].pathTime = singleJobTime;
      return singleJobTime;
    }
    // Recursive search
    std::vector<double> possiblePathTimes{}; 
    for (int i = 0; i < nTasks; ++i)
      if (tasksAdjacencyMatrix[taskID][i])
        possiblePathTimes.push_back(computeCriticalPath(i));
    double maxTime = possiblePathTimes[0];
    for (auto e : possiblePathTimes)
      if (e > maxTime)
        maxTime = e;
    tasks[taskID].pathTime = maxTime;
    return maxTime + times[taskID][tasks[taskID].resourceID];
  }

  int findNextTaskInSchedule() {
    int taskID = 0;
    double maxTime = 0;
    for (int i = 0; i < nTasks; ++i)
      if (!tasks[i].scheduled)
        tasks[i].pathTime = computeCriticalPath(i);
    for (int i = 0; i < nTasks; ++i)
      if (!tasks[i].scheduled && 
        tasks[i].pathTime > maxTime) {
        maxTime = tasks[i].pathTime;
        taskID = i;
      }
    return taskID;      
  }

  void scheduleAllTasks() {
    for (int i = 0; i < nTasks; ++i) {
      int nextTask = findNextTaskInSchedule();
      if (i == 0)
        std::cout << "  ";
      std::cout << (tasks[nextTask].unpredicted ? "u" : "") << "T" << nextTask;
      if (i != nTasks - 1)
        std::cout << " --> ";
      tasks[nextTask].scheduled = true;
      tasks[nextTask].pathTime = -1;
    }
    std::cout << '\n';
  }

  void allocateMinTime() {
    // Alokuje wszystkie zasoby zgodnie z kryterium "min(time * cost)", przy
    // czym nieprzewidziany zadania mają dostęp tylko do zasobow
    // uniwersalnych
    for (int t = 0; t < nTasks; ++t) {
      int bestResourceID;
      bestResourceID = findBest_timeCost(t, tasks[t].unpredicted);
      tasks[t].resourceID = bestResourceID;

      std::string resourceLabel = "undefined_label";
      if (proc[bestResourceID][2] == 0)
        resourceLabel = "HC";
      else
        resourceLabel = "PP";
      int PE_type_count = 1;
      for (int i = 0; i < bestResourceID; ++i)
        if (proc[i][2] == proc[bestResourceID][2]) PE_type_count++;
      resourceLabel += std::to_string(PE_type_count);

      std::cout << (tasks[t].unpredicted ? "  u" : "  ") << "T" << t << " --> " 
        << resourceLabel << '\n';
    }
  }

  // Getter'y i Setter'y do zwracania atrybutów prywatnych
  double getOverallTime() { return overallTime; }
  double getOverallCost() { return overallCost; }
  void setMaxTime(double t) { t_max = t; }
  double getMaxTime() { return t_max; }
};

#endif
