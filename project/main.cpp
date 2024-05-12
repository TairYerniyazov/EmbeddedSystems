#include "parser.hpp"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "Provide the task graph file name (e.g. "
                 "graph.20.dat.txt) as an additional command line arguments.\n";
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
}