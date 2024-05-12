#include "parser.hpp"

Parser::Parser() : nTasks{0} {}
Parser::~Parser() {}

void Parser::read(std::string filepath) {
  std::ifstream inputFile;
  inputFile.open(filepath);
  if (inputFile.is_open()) {
    
    // @tasks - header
    // std::cerr << "Reading @tasks\n";
    std::regex pat(R"((@tasks) (\d{1,}))");
    std::string line; 
    getline(inputFile, line);
    std::smatch matches;
    if (std::regex_search(line, matches, pat))
      nTasks = std::stoi(matches.str(2));
    tasksAdjacencyMatrix.build(nTasks, nTasks);
    // @tasks - body
    while (getline(inputFile, line)) {
      pat = R"((T)(\d{1,}) (\d{1,}))";
      if (std::regex_search(line, matches, pat)) {
        int taskID;
        taskID = std::stoi(matches.str(2));
        line = matches.suffix().str();
        pat = R"((\d{1,})\(\d{1,}\))";
        while (std::regex_search(line, matches, pat)) {
          int nextTaskID = std::stoi(matches.str(1));
          line = matches.suffix().str();
          tasksAdjacencyMatrix[taskID][nextTaskID] = true;
        }
      } else {
        break;
      }
    }

    // @proc - header
    // std::cerr << "Reading @proc\n";
    pat = R"((@proc) (\d{1,}))";
    if (std::regex_search(line, matches, pat))
      nPE = std::stoi(matches.str(2));
    procMatrix.build(nPE, 3);
    // @proc - body
    pat = R"((\d{1,}) (\d{1,}) (\d{1,}))";
    for (int i = 0; i < nPE; ++i) {
      getline(inputFile, line);
      if (std::regex_search(line, matches, pat)) {
        procMatrix[i][0] = std::stod(matches.str(1));
        procMatrix[i][1] = std::stod(matches.str(2));
        procMatrix[i][2] = std::stod(matches.str(3));
      }
    }

    // @times - header
    // std::cerr << "Reading @times\n";
    getline(inputFile, line);
    // @times - body
    pat = R"(\d{1,})";
    timesMatrix.build(nTasks, nPE);
    for (int i = 0; i < nTasks; ++i) {
      getline(inputFile, line);
      for (int j = 0; std::regex_search(line, matches, pat); ++j) {
        timesMatrix[i][j] = std::stod(matches[0]);
        line = matches.suffix().str();
      }
    }

    // @cost - header
    // std::cerr << "Reading @cost\n";
    getline(inputFile, line);
    // @cost - body
    pat = R"(\d{1,})";
    costMatrix.build(nTasks, nPE);
    for (int i = 0; i < nTasks; ++i) {
      getline(inputFile, line);
      for (int j = 0; std::regex_search(line, matches, pat); ++j) {
        costMatrix[i][j] = std::stod(matches[0]);
        line = matches.suffix().str();
      }
    }
  }
  inputFile.close();
}

Matrix<bool>& Parser::getTasksAdjacencyMatrix() { return tasksAdjacencyMatrix; }
Matrix<double>& Parser::getProcMatrix() { return procMatrix; }
Matrix<double>& Parser::getTimesMatrix() { return timesMatrix; }
Matrix<double>& Parser::getCostMatrix() { return costMatrix; }
int Parser::getNumTasks() { return nTasks; }
int Parser::getNumPE() { return nPE; }

void Parser::debug() {
  std::cerr << "\n======== DEBUGGING ========\n";
  std::cerr << "nTasks : " << nTasks << '\n';
  std::cerr << "nPE : " << nPE << "\n\n";

  std::cerr << "taskAdjacencyMatrix:\n";
  for (int i = 0; i < nTasks; ++i) {
    for (auto e : tasksAdjacencyMatrix[i])
      std::cerr << e << "|";
    std::cerr << '\n';
  }
  std::cerr << '\n';

  std::cerr << "procMatrix:\n";
  for (int i = 0; i < nPE; ++i) {
    for (auto e : procMatrix[i])
      std::cerr << e << "|";
    std::cerr << '\n';
  }
  std::cerr << '\n';

  std::cerr << "timesMatrix:\n";
  for (int i = 0; i < nTasks; ++i) {
    for (auto e : timesMatrix[i])
      std::cerr << e << "|";
    std::cerr << '\n';
  }
  std::cerr << '\n';

  std::cerr << "costMatrix:\n";
  for (int i = 0; i < nTasks; ++i) {
    for (auto e : costMatrix[i])
      std::cerr << e << "|";
    std::cerr << '\n';
  }
  std::cerr << "======== ========= ========\n";
}
