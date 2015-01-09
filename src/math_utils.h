#ifndef JRIMAGE_MATH_UTILS_H_
#define JRIMAGE_MATH_UTILS_H_

#include <algorithm>
#include <type_traits>
#include <limits>
#include <iostream>

#include <cassert>

namespace jr {
namespace math_utils {

// Convert in_val from type InT to type OutT.
// If in_val exceeds the range of type OutT, we clamp the return value
// to the allowable range of the type OutT.
// InT and OutT must be arithmetic types.  InT and OutT must also:
//   both be signed type OR
//   both be unsigned types
// This function will not work for mixed-sign InT and OutT.
// This function is similar to OpenCV's "saturating cast."
template<typename InT, typename OutT>
constexpr OutT ConvertWithSaturation(const InT& in_val);

// Return the smallest number X that is a multiple of "multiple_of" and >=
// "min_value".
std::size_t UpToNearestMultiple(std::size_t min_value, std::size_t multiple_of);

// Clamp the value in to the range [min_val, max_val]
template<typename T>
constexpr T Clamp(const T& in, const T& min_val, const T& max_val);

// SignStatusEqual<U,V> will have
// SignStatusEqual<U,V>::value true if types U and V are both signed or
// types U and V are both unsigned.
// Obviously, this trait only applies to arithmetic types.
template<typename U, typename V>
struct SignStatusEqual {
  static_assert(std::is_arithmetic<U>::value, "Arithmetic types only!");
  static_assert(std::is_arithmetic<V>::value, "Arithmetic types only!");
  static constexpr bool value =
      !(std::is_signed<U>::value ^ std::is_signed<V>::value);
};

}  // namespace math_utils
}  // namespace jr

// Below this line is only implementation details. ----------------------------

namespace jr {

namespace math_utils {

inline std::size_t UpToNearestMultiple(std::size_t min_value,
                                       std::size_t multiple_of) {
  assert(multiple_of > 0);
  if (min_value == 0) {
    return 0;
  } else {
    return ((min_value / multiple_of) + (min_value % multiple_of != 0)) *
           multiple_of;
  }
}

template <typename T>
inline constexpr T Clamp(const T& in, const T& min_val, const T& max_val) {
  return std::max<T>(std::min<T>(in, max_val), min_val);
}

template<typename InT, typename OutT>
constexpr inline OutT ConvertWithSaturation(const InT& in_val) {
  // Note - is_specialized function will become constexpr in C++14 so
  // at that point we can make these static_asserts.
  //
  //static_assert(std::numeric_limits::is_specialized<InT>::value,
  //              "Invalid InT!");
  //static_assert(std::numeric_limits::is_specialized<OutT>::value,
  //              "Invalid OutT!");
  //
  // Normally I would just assert(...) these now but we want this function
  // to remain constexpr (which means no asserts(...)).

  // TODO(cbraley): Remove this limitation at some point -
  // this is harder than it sounds!
  static_assert(SignStatusEqual<InT, OutT>::value,
                "Mixed sign expressions not allowed "
                "with ConvertWithSaturation(...)!");

  // Note - We don't use std::numeric_limits::min here since it is the smallest
  // positive floating point number! (min() would be fine for integer types).
  return
      in_val >= std::numeric_limits<OutT>::max()    ? std::numeric_limits<OutT>::max() :
      in_val <= std::numeric_limits<OutT>::lowest() ? std::numeric_limits<OutT>::lowest() :
      OutT(in_val);
}

}  // namespace math_utils
}  // namespace jr
#endif  // JRIMAGE_MATH_UTILS_H_
