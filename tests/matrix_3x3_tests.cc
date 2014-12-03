#include <string>
#include <iostream>
#include <vector>

#include "gtest/gtest.h"

#include "matrix_3x3.h"

// All of the tests in this file are executed at *compile time*.

namespace {

template<typename T>
constexpr T Abs(const T& v) {
  return v < 0 ? -v : v;
}

template<typename T>
constexpr bool Close(const T& a, const T& b, const T& max_diff) {
  return Abs(a - b) < max_diff;
}

TEST(CompileTimeMatrix3x3, MatConstruct) {
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

TEST(CompileTimeMatrix3x3, CompileTimeMatScalarMul) {
  constexpr jr::CTMat3x3<float> A =
      jr::CTMat3x3<float>(1, 2, 3,
                          3, 2, 1,
                          2, 1, 3) * 3.0f;
  static_assert(A(1,1) == 6, "Mat * scalar code incorrect.");
}

TEST(CompileTimeMatrix3x3, CompileTimeMatMul) {
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

TEST(CompileTimeMatrix3x3, CompileTimeInversion) {
  constexpr double INVERSION_EPSILON = 1e-4;
  constexpr jr::CTMat3x3<double> A(
      -1, -2, -3,
      4, 5, 6,
      12, 13, -14);
  constexpr jr::CTMat3x3<double> A_inv_expected(
      37. / 21., 67. / 84., -1. / 28.,
      -32. / 21., -25. / 42, 1. / 14.,
      2. / 21., 11. / 84., -1. / 28.);

  constexpr jr::CTMat3x3<double> A_inv = Inverse(A);
  static_assert(Close(A_inv(0, 0), A_inv_expected(0, 0), INVERSION_EPSILON),
                "Matrix Inv code incorrect.");
  static_assert(Close(A_inv(0, 1), A_inv_expected(0, 1), INVERSION_EPSILON),
                "Matrix Inv code incorrect.");
  static_assert(Close(A_inv(0, 2), A_inv_expected(0, 2), INVERSION_EPSILON),
                "Matrix Inv code incorrect.");
  static_assert(Close(A_inv(1, 0), A_inv_expected(1, 0), INVERSION_EPSILON),
                "Matrix Inv code incorrect.");
  static_assert(Close(A_inv(1, 1), A_inv_expected(1, 1), INVERSION_EPSILON),
                "Matrix Inv code incorrect.");
  static_assert(Close(A_inv(1, 2), A_inv_expected(1, 2), INVERSION_EPSILON),
                "Matrix Inv code incorrect.");
  static_assert(Close(A_inv(2, 0), A_inv_expected(2, 0), INVERSION_EPSILON),
                "Matrix Inv code incorrect.");
  static_assert(Close(A_inv(2, 1), A_inv_expected(2, 1), INVERSION_EPSILON),
                "Matrix Inv code incorrect.");
  static_assert(Close(A_inv(2, 2), A_inv_expected(2, 2), INVERSION_EPSILON),
                "Matrix Inv code incorrect.");
}

TEST(CompileTimeMatrix3x3, CompileTimeDeterminant) {
  constexpr jr::CTMat3x3<int> A(
      44, 33, 22,
      1, 6, -2,
      4, 6, -11);
  static_assert(Determinant(A) == -2673, "Determinant code incorrect.");
}

}
