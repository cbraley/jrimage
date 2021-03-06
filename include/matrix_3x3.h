#ifndef JRIMAGE_COMPILE_TIME_MATRIX_MUL_H_
#define JRIMAGE_COMPILE_TIME_MATRIX_MUL_H_

#include <cassert>

namespace jr {

/// 3x3 matrix class.  All functions are constexpr so they can be evaluated at
/// compile time if their arguments are constexpr.
template<typename NumericT>
class CTMat3x3 {
 public:
  constexpr CTMat3x3(
      NumericT mat_r0_c0, NumericT mat_r0_c1, NumericT mat_r0_c2,
      NumericT mat_r1_c0, NumericT mat_r1_c1, NumericT mat_r1_c2,
      NumericT mat_r2_c0, NumericT mat_r2_c1, NumericT mat_r2_c2);

  constexpr NumericT operator()(const int r, const int c) const;
  constexpr double GetD(const int r, const int c) const;

 private:
  const NumericT data_[9];
};


// Compile time 3x3 matrix multiplication.
template<typename NumericT>
constexpr CTMat3x3<NumericT>
operator *(const CTMat3x3<NumericT> &lhs, const CTMat3x3<NumericT> &rhs);

// Compile time matrix * scalar.
template <typename NumericT>
constexpr CTMat3x3<NumericT> operator*(const NumericT &lhs,
                                       const CTMat3x3<NumericT> &rhs);
template <typename NumericT>
constexpr CTMat3x3<NumericT> operator*(const CTMat3x3<NumericT> &lhs,
                                       const NumericT &rhs);

// Compile time determinant computation.
template<typename NumericT>
constexpr NumericT Determinant(const CTMat3x3<NumericT> &mat);

// Compile time type conversion.
template<typename InNumericT, typename OutNumericT>
constexpr CTMat3x3<OutNumericT> ConvertType(const CTMat3x3<InNumericT> &mat);


// Compile time matrix inversion.
template<typename NumericT>
constexpr CTMat3x3<NumericT> Inverse(const CTMat3x3<NumericT> &mat);


// Function definitions only beyond this line. --------------------------------

template <typename NumericT>
constexpr CTMat3x3<NumericT>::CTMat3x3(NumericT mat_r0_c0, NumericT mat_r0_c1,
                                       NumericT mat_r0_c2, NumericT mat_r1_c0,
                                       NumericT mat_r1_c1, NumericT mat_r1_c2,
                                       NumericT mat_r2_c0, NumericT mat_r2_c1,
                                       NumericT mat_r2_c2)
    : data_{mat_r0_c0, mat_r0_c1, mat_r0_c2, mat_r1_c0, mat_r1_c1,
            mat_r1_c2, mat_r2_c0, mat_r2_c1, mat_r2_c2} {}

template<typename NumericT>
constexpr NumericT CTMat3x3<NumericT>::operator()(const int r,
                                                  const int c) const {
  return data_[r * 3 + c];
}

template <typename NumericT>
constexpr double CTMat3x3<NumericT>::GetD(const int r, const int c) const {
  return double((*this)(r, c));
}

template<typename NumericT>
constexpr CTMat3x3<NumericT>
operator *(const CTMat3x3<NumericT> &lhs, const CTMat3x3<NumericT> &rhs) {
  return CTMat3x3<NumericT>(
      // First row of the result.
      lhs(0, 0)*rhs(0, 0) + lhs(0, 1)*rhs(1, 0) + lhs(0, 2)*rhs(2, 0),   // 00
      lhs(0, 0)*rhs(0, 1) + lhs(0, 1)*rhs(1, 1) + lhs(0, 2)*rhs(2, 1),   // 01
      lhs(0, 0)*rhs(0, 2) + lhs(0, 1)*rhs(1, 2) + lhs(0, 2)*rhs(2, 2),   // 02
      // Second row of the result.
      lhs(1, 0)*rhs(0, 0) + lhs(1, 1)*rhs(1, 0) + lhs(1, 2)*rhs(2, 0),   // 10
      lhs(1, 0)*rhs(0, 1) + lhs(1, 1)*rhs(1, 1) + lhs(1, 2)*rhs(2, 1),   // 11
      lhs(1, 0)*rhs(0, 2) + lhs(1, 1)*rhs(1, 2) + lhs(1, 2)*rhs(2, 2),   // 12
      // Second row of the result.
      lhs(2, 0)*rhs(0, 0) + lhs(2, 1)*rhs(1, 0) + lhs(2, 2)*rhs(2, 0),   // 20
      lhs(2, 0)*rhs(0, 1) + lhs(2, 1)*rhs(1, 1) + lhs(2, 2)*rhs(2, 1),   // 21
      lhs(2, 0)*rhs(0, 2) + lhs(2, 1)*rhs(1, 2) + lhs(2, 2)*rhs(2, 2));  // 22
}

template <typename NumericT>
constexpr CTMat3x3<NumericT> operator*(const NumericT &lhs,
                                       const CTMat3x3<NumericT> &rhs) {
  return CTMat3x3<NumericT>(
      lhs * rhs(0, 0), lhs * rhs(0, 1), lhs * rhs(0, 2),
      lhs * rhs(1, 0), lhs * rhs(1, 1), lhs * rhs(1, 2),
      lhs * rhs(2, 0), lhs * rhs(2, 1), lhs * rhs(2, 2));
}


template<typename InNumericT, typename OutNumericT>
constexpr CTMat3x3<OutNumericT> ConvertType(const CTMat3x3<InNumericT> &mat) {
  return CTMat3x3<OutNumericT>(
      OutNumericT(mat(0, 0)), OutNumericT(mat(0, 1)), OutNumericT(mat(0, 2)),
      OutNumericT(mat(1, 0)), OutNumericT(mat(1, 1)), OutNumericT(mat(1, 2)),
      OutNumericT(mat(2, 0)), OutNumericT(mat(2, 1)), OutNumericT(mat(2, 2)));
}

template <typename NumericT>
constexpr CTMat3x3<NumericT> operator*(const CTMat3x3<NumericT> &lhs,
                                       const NumericT &rhs) {
  return rhs * lhs;  // Matrix * scalar is commutative.
}

template<typename NumericT>
constexpr NumericT Determinant(const CTMat3x3<NumericT> &mat) {
  return NumericT(
         (mat.GetD(0, 0) * mat.GetD(1, 1) * mat.GetD(2, 2) +
          mat.GetD(0, 1) * mat.GetD(1, 2) * mat.GetD(2, 0) +
          mat.GetD(0, 2) * mat.GetD(1, 0) * mat.GetD(2, 1)) -
         (mat.GetD(0, 0) * mat.GetD(1, 2) * mat.GetD(2, 1) +
          mat.GetD(0, 1) * mat.GetD(1, 0) * mat.GetD(2, 2) +
          mat.GetD(0, 2) * mat.GetD(1, 1) * mat.GetD(2, 0)));
}

template<typename NumericT>
constexpr CTMat3x3<NumericT> Inverse(const CTMat3x3<NumericT> &mat) {
  return ConvertType<double, NumericT>(
      (1.0 / Determinant(ConvertType<NumericT, double>(mat))) *
      CTMat3x3<double>(
       (mat.GetD(1, 1) * mat.GetD(2, 2) - mat.GetD(1, 2) * mat.GetD(2, 1)),
      -(mat.GetD(0, 1) * mat.GetD(2, 2) - mat.GetD(0, 2) * mat.GetD(2, 1)),
       (mat.GetD(0, 1) * mat.GetD(1, 2) - mat.GetD(0, 2) * mat.GetD(1, 1)),
      -(mat.GetD(1, 0) * mat.GetD(2, 2) - mat.GetD(1, 2) * mat.GetD(2, 0)),
       (mat.GetD(0, 0) * mat.GetD(2, 2) - mat.GetD(0, 2) * mat.GetD(2, 0)),
      -(mat.GetD(0, 0) * mat.GetD(1, 2) - mat.GetD(0, 2) * mat.GetD(1, 0)),
       (mat.GetD(1, 0) * mat.GetD(2, 1) - mat.GetD(1, 1) * mat.GetD(2, 0)),
      -(mat.GetD(0, 0) * mat.GetD(2, 1) - mat.GetD(0, 1) * mat.GetD(2, 0)),
       (mat.GetD(0, 0) * mat.GetD(1, 1) - mat.GetD(0, 1) * mat.GetD(1, 0))));
}

}  // namespace jr

#endif  // JRIMAGE_COMPILE_TIME_MATRIX_MUL_H_
