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
  Matrix<bool> tasksAdjacencyMatrix{};
  Matrix<double> procMatrix{}; 
  Matrix<double> timesMatrix{};
  Matrix<double> costMatrix{};
 public:
  Parser();
  ~Parser();
  void read(std::string filepath);
  void debug();
  Matrix<bool>& getTasksAdjacencyMatrix();
  Matrix<double>& getProcMatrix();
  Matrix<double>& getTimesMatrix();
  Matrix<double>& getCostMatrix();
  int getNumTasks(); 
  int getNumPE(); 
};

#endif