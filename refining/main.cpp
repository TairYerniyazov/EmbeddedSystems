#include "parser.hpp"
#include "resourceAllocator.hpp"

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cout << "Provide the task graph file name (e.g. "
                 "graph.20.dat.txt), as well as the maximum allowable time "
                 "as two additional command line arguments.\n";
    return 0;
  }
  
  Parser p{};
  p.read(std::string(argv[1]));
  auto tasksAdjacencyMatrix = p.getTasksAdjacencyMatrix();
  auto procMatrix = p.getProcMatrix();
  auto timesMatrix = p.getTimesMatrix();
  auto costMatrix = p.getCostMatrix();
  auto commMatrix = p.getCommMatrix();
  auto tasksMatrix = p.getTasksMatrix();
  p.debug();

  ResourceAllocator ra(tasksAdjacencyMatrix, procMatrix, timesMatrix, 
    costMatrix, commMatrix, tasksMatrix);
  ra.debug();

  std::cout
      << "\n\e[1mInitial resource allocation (minimum time criterium):\e[0m\n";
  for (int i = 0; i < tasksAdjacencyMatrix.d1; ++i)
    ra.allocateMinTime(i);
  ra.recomputeOverallTimeAndCost();
  std::cout << "Overall time: " << ra.getOverallTime() << '\n';
  std::cout << "Overall cost: " << ra.getOverallCost() << '\n';
  ra.setMaxTime(std::stod(std::string(argv[2])));
  std::cout << "Maximum allowable time: " << ra.getMaxTime() << '\n';

  std::cout << "\n\e[1mOptimisation:\e[0m\n";
  ra.optimise();
}