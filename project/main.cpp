#include "parser.hpp"
#include "utilities.hpp"
#include "resourceAllocator.hpp"

int main(int argc, char *argv[]) {
  if (argc < 5) {
    std::cout << "\nRun the program as the following:\n\n"
      << "  $ ./program [data] [max time] [max cost] [choice]\n"
      << "\n  Choice = 1: using a structural algorithm;\n"
      << "  Choice = 2: handling unpredicted tasks.\n\n";
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
  auto unpredictedTasksMask = p.getUnpredictedTasksMask();
  p.debug();

  ResourceAllocator r{tasksAdjacencyMatrix,
                      procMatrix,
                      timesMatrix,
                      costMatrix,
                      commMatrix,
                      tasksMatrix,
                      unpredictedTasksMask,
                      std::stod(std::string(argv[2])),
                      std::stod(std::string(argv[3]))};
  
  int choice = std::stod(std::string(argv[4]));
  if (choice == 1) {
    std::cout << "\n\e[32m\e[1mAlokacja zasobów metodą standaryzacji:\e[0m\n";
    for (int t = 0; t < tasksMatrix.d1; ++t) {
      r.allocate(t);
    }
    std::cout << "\n\e[34mCałkowity czas wykonania:\e[0m " << r.getOverallTime() 
      << '\n';
    std::cout << "\e[34mCałkowity koszt:\e[0m " << r.getOverallCost() << '\n';
    std::cout << '\n';
  } else if (choice == 2) {
    r.debug();
    std::cout << "\n\e[32m\e[1mPoczątkowy przydział zasobów\e[0m\n";
    r.allocateMinTime();
    std::cout << "\n\e[34m\e[1mPoszeregowane zadania "
      << "(w tym nieprzewidziane):\e[0m\n";
    r.scheduleAllTasks();
    std::cout << '\n';
  }
}
