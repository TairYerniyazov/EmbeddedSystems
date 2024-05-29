#include "parser.hpp"
#include "utilities.hpp"
#include "resourceAllocator.hpp"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "Provide the task graph file name (e.g. "
                 "graph.20.dat.txt) as an additional command line arguments.\n";
    return 0;
  }
  
  Parser p{};
  if (p.read(std::string(argv[1])) == -1)
    return 0;
  auto tasksAdjacencyMatrix = p.getTasksAdjacencyMatrix();
  auto procMatrix = p.getProcMatrix();
  auto timesMatrix = p.getTimesMatrix();
  auto costMatrix = p.getCostMatrix();
  auto commMatrix = p.getCommMatrix();
  auto tasksMatrix = p.getTasksMatrix();
  p.debug();

  ResourceAllocator r{tasksAdjacencyMatrix, procMatrix, timesMatrix,
    costMatrix, commMatrix, tasksMatrix, 100, 10000};
  std::cout << "\n\e[32m\e[1mAlokacja zasobów metodą standaryzacji:\e[0m\n";
  for (int t = 0; t < tasksMatrix.d1; ++t) {
    r.allocate(t);
  }
  std::cout << "\n\e[31mOverall cost:\e[0m " << r.getOverallCost() << '\n';
  std::cout << "\e[31mOverall time:\e[0m " << r.getOverallTime() << '\n';
  std::cout << '\n';
}
