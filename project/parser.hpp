#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <string>
#include "matrix.hpp"
#include <iostream>
#include <fstream>
#include <regex>

class Parser {
 private:
  int nTasks;
  int nPE;
  int nChannels;
  Matrix<bool> tasksAdjacencyMatrix{};
  Matrix<double> tasksMatrix{};
  Matrix<double> procMatrix{}; 
  Matrix<double> timesMatrix{};
  Matrix<double> costMatrix{};
  Matrix<double> commMatrix{};
  std::vector<bool> unpredictedTasksMask{}; // [True, False, False] - T1 i T2
                                            // są prewidziane, T0 nieprzewidziane.
 public:
  Parser();
  ~Parser();
  int read(std::string filepath);
  void debug();
  Matrix<bool>& getTasksAdjacencyMatrix();
  Matrix<double>& getTasksMatrix();
  Matrix<double>& getProcMatrix();
  Matrix<double>& getTimesMatrix();
  Matrix<double>& getCostMatrix();
  Matrix<double>& getCommMatrix();
  std::vector<bool>& getUnpredictedTasksMask();
};

#endif