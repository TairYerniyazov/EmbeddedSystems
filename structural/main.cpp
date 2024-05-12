#include <ctime>
#include "parser.hpp"
#include "resourceAllocator.hpp"
#include <string>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "Provide the task graph file name (e.g. "
                 "graph.20.dat.txt) as an additional command line "
                 "argument\n"; 
    return 0;
  }
  Parser p{};
  // p.read("graph.20.dat.txt");
  // p.read("GRAF.200.txt");
  // p.read("GRAF.30.txt");
  p.debug();
  p.read(std::string(argv[1]));
  auto tasksAdjacencyMatrix = p.getTasksAdjacencyMatrix();
  auto procMatrix = p.getProcMatrix();
  auto timesMatrix = p.getTimesMatrix();
  auto costMatrix = p.getCostMatrix();
  int nTasks = p.getNumTasks();
  // int nPE = p.getNumPE();

  ResourceAllocator ra(procMatrix, costMatrix, timesMatrix,
                       tasksAdjacencyMatrix, 2000);

  using r_engine = std::mt19937;
  using r_distr = std::uniform_int_distribution<>;
  std::random_device rd;
  static r_engine re{};
  static r_distr dist{1, 1000};
  re.seed(rd());
  auto die = [](){ return dist(re); };
  // ra.debug();
  std::cout << "Choosing processing units for all the tasks:\n";
  for (int i = 0; i < nTasks; ++i) {
    ra.allocateResource(i, die());
  }
  std::cout << "Total time: " << ra.getLastTime() << '\n';
}