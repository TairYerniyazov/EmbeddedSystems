#ifndef MATRIX_H
#define MATRIX_H

#include <vector>

template<typename T>
class Matrix {
 typedef std::vector<std::vector<T>> TwoDimMatrix;
 typedef std::vector<T> OneDimMatrix;
 private:
  TwoDimMatrix elem;
 public:
  int d1;
  int d2;
  Matrix() : elem{}, d1{}, d2{} {}
  ~Matrix() {}
  OneDimMatrix& operator[](int i) { return elem[i]; }
  OneDimMatrix operator[](int i) const { return elem[i]; }
  void build(int d1_, int d2_) {
    for (int i = 0; i < d1_; ++i)
      elem.push_back(std::vector<T>(d2_));
    d1 = d1_;
    d2 = d2_;
  }
  Matrix(const Matrix<T>& m) {
    for (auto e : m.elem)
      elem.push_back(e);
    d1 = m.d1;
    d2 = m.d2;
  };
  Matrix<T>& operator=(const Matrix<T>& m) {
    TwoDimMatrix temp{};
    for (auto e : m.elem)
      temp.push_back(e);
    elem = temp;
    d1 = m.d1;
    d2 = m.d2;
    return *this;
  };
};

#endif