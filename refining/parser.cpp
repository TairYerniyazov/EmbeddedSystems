#include "parser.hpp"

Parser::Parser() : nTasks{0} {}
Parser::~Parser() {}

void Parser::read(std::string filepath) {
  std::ifstream inputFile;
  inputFile.open(filepath);
  if (inputFile.is_open()) {
    
    // @tasks - header
    std::regex pat(R"((@tasks) (\d{1,}))");
    std::string line; 
    getline(inputFile, line);
    std::smatch matches;
    if (std::regex_search(line, matches, pat))
      nTasks = std::stoi(matches.str(2));
    tasksAdjacencyMatrix.build(nTasks, nTasks);
    tasksMatrix.build(nTasks, nTasks);
    // @tasks - body
    while (getline(inputFile, line)) {
      pat = R"(u*(T)u*(\d{1,}) (\d{1,}))";
      if (std::regex_search(line, matches, pat)) {
        int taskID;
        taskID = std::stoi(matches.str(2));
        line = matches.suffix().str();
        pat = R"((\d{1,})\((\d{1,})\))";
        while (std::regex_search(line, matches, pat)) {
          int nextTaskID = std::stoi(matches.str(1));
          int edge = std::stoi(matches.str(2));
          line = matches.suffix().str();
          tasksAdjacencyMatrix[taskID][nextTaskID] = true;
          tasksMatrix[taskID][nextTaskID] = edge;
        }
      } else {
        break;
      }
    }

    // @proc - header
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

    // @comm - header
    pat = R"((@comm) (\d{1,}))";
    getline(inputFile, line);
    if (std::regex_search(line, matches, pat))
      nChannels = std::stoi(matches.str(2));
    commMatrix.build(nChannels, 2 + nPE);
    // @comm - body
    std::regex pat0(R"((CHAN\d{1,}))");
    pat = R"(\d{1,})";
    for (int i = 0; i < nChannels; ++i) {
      getline(inputFile, line);
      std::regex_search(line, matches, pat0);
      line = matches.suffix().str();
      for (int j = 0; std::regex_search(line, matches, pat); ++j) {
        commMatrix[i][j] = std::stod(matches[0]);
        line = matches.suffix().str();
      }
    }
  } else { std::cerr << "The input file cannot be opened.\n"; }
  inputFile.close();
}

Matrix<bool>& Parser::getTasksAdjacencyMatrix() { return tasksAdjacencyMatrix; }
Matrix<double>& Parser::getTasksMatrix() { return tasksMatrix; }
Matrix<double>& Parser::getProcMatrix() { return procMatrix; }
Matrix<double>& Parser::getTimesMatrix() { return timesMatrix; }
Matrix<double>& Parser::getCostMatrix() { return costMatrix; }
Matrix<double>& Parser::getCommMatrix() { return commMatrix; }

void Parser::debug() {
  std::cerr << "\nParser::debug()\n";
  std::cerr << "  nTasks : " << nTasks << '\n';
  std::cerr << "  nPE : " << nPE << "\n\n";

  std::cerr << "Parser::tasksAdjacencyMatrix:\n";
  for (int i = 0; i < nTasks; ++i) {
    std::cerr << "  "; 
    for (auto e : tasksAdjacencyMatrix[i]) std::cerr << e
              << "|";
    std::cerr << '\n';
  }
  std::cerr << '\n';

  std::cerr << "Parser::tasksMatrix:\n";
  for (int i = 0; i < nTasks; ++i) {
    std::cerr << "  ";
    for (auto e : tasksMatrix[i]) 
      std::cerr << e << "|";
    std::cerr << '\n';
  }
  std::cerr << '\n';

  std::cerr << "Parser::procMatrix:\n";
  for (int i = 0; i < nPE; ++i) {
    std::cerr << "  ";
    for (auto e : procMatrix[i])
      std::cerr << e << "|";
    std::cerr << '\n';
  }
  std::cerr << '\n';

  std::cerr << "Parser::timesMatrix:\n";
  for (int i = 0; i < nTasks; ++i) {
    std::cerr << "  ";
    for (auto e : timesMatrix[i])
      std::cerr << e << "|";
    std::cerr << '\n';
  }
  std::cerr << '\n';

  std::cerr << "Parser::costMatrix:\n";
  for (int i = 0; i < nTasks; ++i) {
    std::cerr << "  ";
    for (auto e : costMatrix[i])
      std::cerr << e << "|";
    std::cerr << '\n';
  }
  std::cerr << '\n';

  std::cerr << "Parser::commMatrix:\n";
  for (int i = 0; i < nChannels; ++i) {
    std::cerr << "  ";
    for (auto e : commMatrix[i]) 
      std::cerr << e << "|";
    std::cerr << '\n';
  }
  std::cerr << '\n';
}
