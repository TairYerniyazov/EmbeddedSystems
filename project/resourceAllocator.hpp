#ifndef RESOURCE_ALLOCATOR_H
#define RESOURCE_ALLOCATOR_H

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <algorithm>
#include "matrix.hpp"

struct PE {
  double totalActiveTime;
  int totalNumOfJobs;
  double lastTaskStartTime;
  double lastTaskEndTime;
  int procID;
  std::string label;
  std::vector<int> channelIDs;
  PE(double totalActiveTime_, int totalNumOfJobs_, double lastTaskStartTime_,
  double lastTaskEndTime_, int procID_, std::string label_) : 
    totalActiveTime{totalActiveTime_}, totalNumOfJobs{totalNumOfJobs_}, 
    lastTaskStartTime{lastTaskStartTime_}, lastTaskEndTime{lastTaskEndTime_},
    procID{procID_}, label{label_}, channelIDs{std::vector<int>()} {}
  PE(int procID_, std::string label_) : 
    PE(-1, -1, -1, -1, procID_, label_) {}
};

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

struct Channel {
  double cost;
  double bandwidth;
  std::vector<bool> connections;
  int id;
  Channel(double c, double b, std::vector<bool> conn, int id_) : cost{c}, 
  bandwidth{b}, connections{conn}, id{id_} {}
  ~Channel() {}
};

class ResourceAllocator {
 private:
  Matrix<bool> tasksAdjacencyMatrix;
  Matrix<double> proc;
  Matrix<double> times;
  Matrix<double> cost;
  std::vector<Channel> channels;
  std::vector<Task> tasks;
  std::vector<PE> resources;
  int nTasks;
  int nPEs;
  int nChannels;
  std::vector<int> PE_instances_ids;  // [nHC, nPP, nPE_1, nPE_2, ...]
  Matrix<double> tasksMatrix;
  double overallTime;
  double overallCost;
  double t_max;
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
    }
  ~ResourceAllocator() {}
  std::vector<int> findAllParents(int taskID) {
    std::vector<int> results{};
    for (int i = 0; i < nTasks; ++i)
      if (tasksAdjacencyMatrix[i][taskID])
        results.push_back(i);
    return results;
  }
  int findBestParent(int taskID) {
    // the "best" parent is the one ending the earliest
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
    // The "best" channel is the one offering the lowest cost
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
    auto parentsIDs = findAllParents(taskID);
    for (auto parentID : parentsIDs)
      if (tasks[parentID].resourceID == -1)
        return false;
    return true;
  }
  bool canBeConnectedToBestParent(int taskID, int procID) {
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
  int findMinTimeResourceID(int taskID) {
    std::vector<int> resourcesThatCanBeUsed{};
    for (int i = 0; i < nPEs; ++i)
      if (canBeConnectedToBestParent(taskID, i))
        resourcesThatCanBeUsed.push_back(i);
    int procID = resourcesThatCanBeUsed[0];
    double minTime = times[taskID][procID];
    for (auto id : resourcesThatCanBeUsed) {
      double timeValue = times[taskID][id];
      if (timeValue < minTime) {
        procID = id;
        minTime = timeValue;
      }
    }
    return procID;
  }
  void allocateMinTime(int taskID) {
    if (tasks[taskID].resourceID == -1) {
      // Recursive bound
      if (allParentsHaveResources(taskID)) {
        // Resource allocation
        std::cerr << "  T" << taskID << "has all parents allocated\n";
        int procID = findMinTimeResourceID(taskID);
        std::string resourceLabel = "undefined_label";
        if (proc[procID][2] == 0)
          resourceLabel = "HC";
        else
          resourceLabel = "PP";
        int PE_type_count = 1;
        for (int i = 0; i < procID; ++i)
          if (proc[i][2] == proc[procID][2])
            PE_type_count++;
        resourceLabel += std::to_string(PE_type_count) + "_" +
                         std::to_string(PE_instances_ids[2 + procID]++);
        resources.push_back(PE(procID, resourceLabel));
        tasks[taskID].resourceID = resources.size() - 1;
        int channelID;
        double startTime;
        if (findAllParents(taskID).size() == 0) {
          channelID = findBestChannel(-1, taskID);
          startTime = 0;
        } else {
          int bestParentID = findBestParent(taskID);
          channelID = findBestChannel(bestParentID, taskID);
          startTime = (tasksMatrix[bestParentID][taskID] /
            channels[channelID].bandwidth) +
            resources[tasks[bestParentID].resourceID].lastTaskEndTime;
            auto parentChannels =
                resources[tasks[bestParentID].resourceID].channelIDs;
            if (std::find(parentChannels.begin(), parentChannels.end(), 
              channelID) == parentChannels.end())
              resources[tasks[bestParentID].resourceID].channelIDs.push_back(
                channelID);
        }
        resources[tasks[taskID].resourceID].channelIDs.push_back(channelID);
        double endTime = startTime + times[taskID][procID];
        resources[tasks[taskID].resourceID].lastTaskStartTime = startTime;
        resources[tasks[taskID].resourceID].lastTaskEndTime = endTime;
        std::cout << "  T" << taskID << " --> "
                  << resources[tasks[taskID].resourceID].label
                  << " [startTime: " << startTime << ", endTime: "
                  << endTime << "]\n";
        
        if (taskID == nTasks - 1) {
          std::cerr << '\n';
          for (int tID = 0; tID < nTasks; ++tID) {
            std::cerr << "Channels for T" << tID << "'s resource: \n  ";
            for (auto channelID : resources[tasks[tID].resourceID].channelIDs)
              std::cerr << channelID << " ";
            std::cerr << '\n';
          }
          std::cerr << '\n';
        }        
      } else {
        // Recursive execution
        std::vector<int> parentsIDs = findAllParents(taskID);
        for (auto parentID : parentsIDs)
          if (tasks[parentID].resourceID == -1) 
            allocateMinTime(parentID);
        allocateMinTime(taskID);
      }
    }
  }
  std::vector<int> createPathDownToUp(int taskID) {
    std::vector<int> results{};
    int t = taskID;
    results.push_back(t);
    while(findAllParents(t).size() != 0) {
      int parentID = findBestParent(t);
      results.push_back(parentID);
      t = parentID;
    }
    return results;
  }
  bool reallocateResource(int taskID) {
    // Finding the slowest resource
    std::vector<int> resourcesThatCanBeUsed{};
    for (int i = 0; i < nPEs; ++i)
      if (canBeConnectedToBestParent(taskID, i))
        resourcesThatCanBeUsed.push_back(i);
    int procID = resourcesThatCanBeUsed[0];
    double maxTime = times[taskID][procID];
    for (auto id : resourcesThatCanBeUsed) {
      double timeValue = times[taskID][id];
      bool connectivity = true;
      
      // Skipping if any of its children for which it is the best parent
      // cannot be physically connected to any of the CL channels
      int initialResourceProcID = resources[tasks[taskID].resourceID].procID;
      resources[tasks[taskID].resourceID].procID = id;
      for (int t = 0; t < nTasks; ++t) {
        if (tasksAdjacencyMatrix[taskID][t] && findBestParent(t) == taskID &&
            !canBeConnectedToBestParent(
                t, resources[tasks[t].resourceID].procID)) {
          std::cerr << "T" << t << " cannot be connected to parent T" << taskID
                    << " by any means\n";
          connectivity = false;
        }
      }
      resources[tasks[taskID].resourceID].procID = initialResourceProcID;

      if (timeValue > maxTime && connectivity) {
        procID = id;
        maxTime = timeValue;
      }
    }
    tasks[taskID].reallocated = true;

    // Doing nothing if it is still the same resource
    if (resources[tasks[taskID].resourceID].procID == procID) {
      std::cerr << "There is no need to change the resource for T" << taskID
        << '\n';
      return true;
    }

    // Doing nothing if any of its children for which it is the best parent
    // cannot be physically connected to any of the CL channels
    int initialResourceProcID = resources[tasks[taskID].resourceID].procID;
    resources[tasks[taskID].resourceID].procID = procID;
    for (int t = 0; t < nTasks; ++t) {
      if (tasksAdjacencyMatrix[taskID][t] && findBestParent(t) == taskID &&
          !canBeConnectedToBestParent(t,
                                      resources[tasks[t].resourceID].procID)) {
        std::cerr << "T" << t << " cannot be connected to parent T" << taskID
                  << " by any means\n";
        resources[tasks[taskID].resourceID].procID = initialResourceProcID;
        return true;
      }
    }

    // Changing the resource otherwise
    std::cout << "There is a slower resource available for T" << taskID << '\n';
    std::string resourceLabel = "undefined_label";
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
    std::cerr << "The new resource has been added to the list of available " 
      << "resources as " << resourceLabel << '\n';
    tasks[taskID].resourceID = resources.size() - 1;
    // Updating parent's and siblings' channels;
    int parentID = findBestParent(taskID);
    int idealChannelWithParent = findBestChannel(parentID, taskID);
    resources[resources.size() - 1].channelIDs.push_back(idealChannelWithParent);
    if (parentID != -1) {
      auto parentChannels = resources[tasks[parentID].resourceID].channelIDs;
      if (std::find(parentChannels.begin(), parentChannels.end(),
                    idealChannelWithParent) == parentChannels.end()) {
        resources[tasks[parentID].resourceID].channelIDs.push_back(
            idealChannelWithParent);
      }
      std::vector<int> children{};
      for (int c = 0; c < nTasks; ++c) {
        if (tasksAdjacencyMatrix[parentID][c] &&
            findBestParent(c) == parentID && c != taskID) {
          int bestChannel = findBestChannel(parentID, c);
          if (std::find(parentChannels.begin(), parentChannels.end(),
                        bestChannel) == parentChannels.end()) {
            resources[tasks[parentID].resourceID].channelIDs.push_back(
                bestChannel);
          }
          auto childChannels = resources[tasks[c].resourceID].channelIDs;
          if (std::find(childChannels.begin(), childChannels.end(),
                        bestChannel) == childChannels.end()) {
            resources[tasks[c].resourceID].channelIDs.push_back(
                bestChannel);
          }
        }
      }
    }
    // Recomputing time along all the passes:
    recomputeAllPathsTime();
    // Recomputing the overall time;
    recomputeOverallTimeAndCost();
    std::cout << "Overall Time: " << overallTime << '\n';
    std::cout << "Overall Cost: " << overallCost << '\n';
    if (overallTime > t_max) {
      std::cout << "\e[1mThe maximum time " << t_max
                << " has been exceeded.\n"
                << "Consider the allocation strategy proposed earlier.\e[0m\n";
      std::cout << "\n";
      return false;
    }
    std::cout << "\n";
    return true;
  }
  void recomputeAllPathsTime() {
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
  void optimise() {
    // Creating all paths by running reverse DFS search beginning with
    // those tasks that have no children they could be the best parents for: 
    std::vector<int> lastTasksIDs{};
    for (int t = 0; t < nTasks; ++t) {
      bool isBestParent = false;
      for (int c = 0; c < nTasks; ++c) {
        if (tasksAdjacencyMatrix[t][c] && t == findBestParent(c))
          isBestParent = true;
      }
      if (!isBestParent)
        lastTasksIDs.push_back(t);
    }
    for (auto lt : lastTasksIDs)
      std::cerr << "T" << lt << " ends one of the possible paths\n";
    // Creating a list of all possible paths (each path is a list of task IDs):
    std::vector<std::vector<int>> allPaths{};
    for (auto lt : lastTasksIDs)
      allPaths.push_back(createPathDownToUp(lt));
    for (auto path : allPaths) {
      std::cerr << "Path: ";
      for (auto t : path)
        std::cerr << "T" << t << " ";
      std::cerr << '\n';
    }
    // Locating the critical and the fastest paths:
    int criticalPathIndex = 0;
    int fastestPathIndex = 0;
    double maxTime =
        resources[tasks[allPaths[criticalPathIndex][0]].resourceID]
          .lastTaskEndTime;
    double minTime = 
        resources[tasks[allPaths[fastestPathIndex][0]].resourceID]
          .lastTaskEndTime;
    for (int i = 0; i < (int)allPaths.size(); ++i) {
      if (resources[tasks[allPaths[i][0]].resourceID].lastTaskEndTime > maxTime) {
        maxTime = resources[tasks[allPaths[i][0]].resourceID].lastTaskEndTime;
        criticalPathIndex = i;
      }
      if (resources[tasks[allPaths[i][0]].resourceID].lastTaskEndTime <
          minTime) {
        minTime = resources[tasks[allPaths[i][0]].resourceID].lastTaskEndTime;
        fastestPathIndex = i;
      }
    }
    if (criticalPathIndex == fastestPathIndex)
      return;
    std::cerr << "Path #" << criticalPathIndex << " is critical\n";
    std::cerr << "Path #" << fastestPathIndex << " is fastest\n";
    // Reallocating resources for every task lying on the fastest path
    // but not belonging to the critical one:
    std::vector<int> criticalPath = allPaths[criticalPathIndex];
    std::vector<int> fastestPath = allPaths[fastestPathIndex];
    for (auto t : fastestPath) {
      if (std::find(criticalPath.begin(), criticalPath.end(), t) ==
        criticalPath.end()) {
        if (!reallocateResource(t))
          return;
      }
    }
    // Reallocating all the others starting with the task ending as the last one
    // and that has not been considered yet
    bool notAllocatedRemain = true;
    while (notAllocatedRemain) {
      std::vector<int> notAllocatedTasksIDs{};
      int nAllocated = 0;
      for (auto t : tasks) {
        std::cerr << t.id << " : " << t.reallocated << '\n';
        if (t.reallocated)
          nAllocated++;
        else
          notAllocatedTasksIDs.push_back(t.id);
      }
      if (nAllocated == nTasks) {
        notAllocatedRemain = false;
        break;
      }
      int lastTaskID = notAllocatedTasksIDs[0];
      double maxTime = resources[tasks[lastTaskID].resourceID].lastTaskEndTime;
      for (int t = 0; t < (int)notAllocatedTasksIDs.size(); ++t) {
        double endTime = resources[tasks[notAllocatedTasksIDs[t]].resourceID]
          .lastTaskEndTime;
        if (endTime > maxTime) {
          lastTaskID = notAllocatedTasksIDs[t];
          maxTime = endTime;
        }
      }
      std::cerr << "Optimisation (step 2): reallocating T" << lastTaskID << '\n';
      if (!reallocateResource(lastTaskID))
        break;
    }
  }
  void recomputeOverallTimeAndCost() {
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
  double getOverallTime() { return overallTime; }
  double getOverallCost() { return overallCost; }
  void setMaxTime(double t) { t_max = t; }
  double getMaxTime() { return t_max; }
};

#endif