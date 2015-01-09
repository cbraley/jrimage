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
// InT and OutT must be arithmetic types.
// This function is similar to OpenCV's "saturating cast."
template<typename InT, typename OutT>
OutT ConvertWithSaturation(const InT& in_val);
// TODO(cbraley): Make constexpr.

// Return the smallest number X that is a multiple of "multiple_of" and >=
// "min_value".
std::size_t UpToNearestMultiple(std::size_t min_value, std::size_t multiple_of);

// Clamp the value in to the range [min_val, max_val]
template<typename T>
constexpr T Clamp(const T& in, const T& min_val, const T& max_val);

}  // namespace math_utils
}  // namespace jr

// Below this line is only implementation details. ----------------------------

namespace jr {

namespace implementation_details {

// We have several helper functions for each of the cases for
// saturating conversions.  We need to deal with
// InT and OutT being signed vs unsigned.
// The correct function to call is chosen at compile time,
// so this produces no overhead.

// in = signed, out = signed
template<typename InT, typename OutT>
inline OutT ConvSatHelper(const InT& in_val,
                          std::true_type in_signed,
                          std::true_type out_signed) {
  constexpr OutT out_max = std::numeric_limits<OutT>::max();
  constexpr OutT out_min = std::numeric_limits<OutT>::lowest();
  return in_val >= out_max ? out_max :
         in_val <= out_min ? out_min : 
         OutT(in_val);
}

// in = signed, out = unsigned
template<typename InT, typename OutT>
inline OutT ConvSatHelper(const InT& in_val,
                          std::true_type in_signed,
                          std::false_type out_signed) {
  typedef typename std::make_unsigned<InT>::type InTUnsigned;
  constexpr OutT out_max = std::numeric_limits<OutT>::max();
  return (in_val < InT(0))              ? OutT(0) :
         InTUnsigned(in_val) >= out_max ? out_max :
         OutT(in_val);
}

// in = unsigned, out = signed
template<typename InT, typename OutT>
inline OutT ConvSatHelper(const InT& in_val,
                          std::false_type in_signed,
                          std::true_type out_signed) {
  constexpr OutT out_max = std::numeric_limits<OutT>::max();
  constexpr OutT out_min = std::numeric_limits<OutT>::lowest();
  constexpr OutT in_max  = std::numeric_limits<InT>::max();
  constexpr OutT in_min  = 0;

  return in_val > 

  return in_val >= out_max ? out_max :
         in_val <= out_min ? out_min : 
         OutT(in_val);
}

// in = unsigned, out = unsigned
template<typename InT, typename OutT>
inline OutT ConvSatHelper(const InT& in_val,
                          std::false_type in_signed,
                          std::false_type out_signed) {
  constexpr OutT out_max = std::numeric_limits<OutT>::max();
  constexpr OutT out_min = std::numeric_limits<OutT>::lowest();
  return in_val >= out_max ? out_max :
         in_val <= out_min ? out_min : 
         OutT(in_val);

}

}  // namespace implementation_details

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

// TODO(cbraley): This is tricky with
//    signed to unsigned widening and narrowing.
template<typename InT, typename OutT>
inline OutT ConvertWithSaturation(const InT& in_val) {
  // Note - The is_specialized function will become constexpr in C++14 so
  // at that point we can make these static_asserts.
  assert(std::numeric_limits::is_specialized<InT>::value);
  assert(std::numeric_limits::is_specialized<OutT>::value);

  // Note - We don't use std::numeric_limits::min here since it is the smallest
  // positive floating point number! (min() would be fine for integer types).

  std::cout << "Conv(" << +in_val << ")" << std::endl;
  return implementation_details::ConvSatHelper<InT, OutT>(
      in_val, std::is_signed<InT>(), std::is_signed<OutT>());
  /*
  constexpr OutT upper_limit_out =
        std::numeric_limits<OutT>::max() > std::numeric_limits<InT>::max() ?
        std::numeric_limits<InT>::max()  : std::numeric_limits<OutT>::max();
  constexpr OutT lower_limit_out =
        std::numeric_limits<OutT>::lowest() < std::numeric_limits<InT>::lowest() ?
        std::numeric_limits<InT>::lowest()  : std::numeric_limits<OutT>::lowest();

  typedef typename std::make_signed<InT >::type InTSigned;
  typedef typename std::make_signed<OutT>::type OutTSigned;
  constexpr OutT out_max = std::numeric_limits<OutT>::max();
  constexpr OutT out_min = std::numeric_limits<OutT>::lowest();
  std::cout << "Conv(" << +in_val << ")  "
            << "out ranges [" << +out_min << ", " << +out_max << "]" << std::endl;

  return in_val >= out_max ? out_max :
         in_val <= out_min ? out_min :
         OutT(12);
  */

//  return Clamp<OutT>(in_val, lower_limit_out, upper_limit_out);

  // Works if OutType is narrow
  /*
  constexpr InT out_type_lowest   = InT(std::numeric_limits<OutT>::lowest());
  constexpr InT out_type_highgest = InT(std::numeric_limits<OutT>::max());
  return OutT(Clamp<InT>(in_val, out_type_lowest, out_type_highgest));
  */

  // What if InType is narrow and OutType is wide?
  /*
  InType = uchar
  OutType = int
  return OutType(in_val);
  */

}

}  // namespace math_utils
}  // namespace jr
#endif  // JRIMAGE_MATH_UTILS_H_
