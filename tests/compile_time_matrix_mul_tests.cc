#include <string>
#include <iostream>
#include <vector>

#include "gtest/gtest.h"

#include "compile_time_matrix_mul.h"

namespace {

TEST(CompileTimeMatrixMultiplication, MatConstruct) {
  constexpr jr::CTMat3x3<double> identity(1, 0, 0, 0, 1, 0, 0, 0, 1);
  static_assert(identity(0, 0) == 1, ".");
  static_assert(identity(1, 1) == 1, ".");
  static_assert(identity(2, 2) == 1, ".");
  static_assert(identity(0, 1) == 0, ".");
  static_assert(identity(0, 2) == 0, ".");
  static_assert(identity(1, 0) == 0, ".");
  static_assert(identity(1, 2) == 0, ".");
  static_assert(identity(2, 0) == 0, ".");
  static_assert(identity(2, 1) == 0, ".");

  constexpr jr::CTMat3x3<double> bands(
    1, 2, 3,
    1, 2, 3,
    1, 2, 3);
}

TEST(CompileTimeMatrixMultiplication, CompileTimeMatScalarMul) {
  constexpr jr::CTMat3x3<float> A = 
      jr::CTMat3x3<float>(1, 2, 3,
                          3, 2, 1,
                          2, 1, 3) * 3.0f;
  static_assert(A(1,1) == 6, "Mat * scalar code incorrect.");
}

TEST(CompileTimeMatrixMultiplication, CompileTimeMatMul) {
  constexpr jr::CTMat3x3<float> A(
      1, 2, 3,
      3, 2, 1,
      2, 1, 3);
  constexpr jr::CTMat3x3<float> B(
      4, 5, 6,
      6, 5, 4,
      4, 6, 5);
  constexpr jr::CTMat3x3<float> AxB = A * B;
  static_assert(AxB(0, 0) == 28, "Mat mul code incorrect.");
  static_assert(AxB(0, 1) == 33, "Mat mul code incorrect.");
  static_assert(AxB(0, 2) == 29, "Mat mul code incorrect.");
  static_assert(AxB(1, 0) == 28, "Mat mul code incorrect.");
  static_assert(AxB(1, 1) == 31, "Mat mul code incorrect.");
  static_assert(AxB(1, 2) == 31, "Mat mul code incorrect.");
  static_assert(AxB(2, 0) == 26, "Mat mul code incorrect.");
  static_assert(AxB(2, 1) == 33, "Mat mul code incorrect.");
  static_assert(AxB(2, 2) == 31, "Mat mul code incorrect.");
}

TEST(CompileTimeMatrixMultiplication, CompileTimeInversion) {
  constexpr double INVERSION_EPSILON = 1e-12;
  constexpr jr::CTMat3x3<double> A(
      -1, -2, -3,
      4, 5, 6,
      12, 13, -14);
  constexpr jr::CTMat3x3<double> A_inv = Inverse(A);
  EXPECT_NEAR(A_inv(0, 0), 1.12, INVERSION_EPSILON);
}


TEST(CompileTimeMatrixMultiplication, CompileTimeDeterminant) {
  constexpr jr::CTMat3x3<int> A(
      44, 33, 22,
      1, 6, -2,
      4, 6, -11);
  static_assert(Determinant(A) == -2673, "Determinant code incorrect.");
}

}
