#ifndef JRIMAGE_MATH_UTILS_H_
#define JRIMAGE_MATH_UTILS_H_

#include <algorithm>

#include <cassert>

namespace jr {
namespace math_utils {

// Return the smallest number X that is a multiple of "multiple_of" and >= "min_value".
std::size_t UpToNearestMultiple(std::size_t min_value, std::size_t multiple_of);


// Clamp the value in to the range [min_val, max_val]
template<typename T>
inline T Clamp(const T& in, const T& min_val, const T& max_val);

}  // namespace math_utils
}  // namespace jr

// Below this line is only implementation details. ----------------------------

namespace jr {
namespace math_utils {

inline std::size_t UpToNearestMultiple(std::size_t min_value, std::size_t multiple_of) {
  assert(multiple_of > 0);
  if (min_value == 0) {
    return 0;
  } else {
    return ((min_value / multiple_of) + (min_value % multiple_of != 0)) * multiple_of;
  }
}

template<typename T>
inline T Clamp(const T& in, const T& min_val, const T& max_val) {
  return std::max<T>(std::min<T>(in, max_val), min_val);
}

}  // namespace math_utils
}  // namespace jr
#endif  // JRIMAGE_MATH_UTILS_H_
