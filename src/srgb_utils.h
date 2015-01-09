#ifndef JRIMAGE_SRGB_UTILS_H_
#define JRIMAGE_SRGB_UTILS_H_

#include <cmath>
#include <type_traits>

namespace jr {

template<typename NumericFloatT>
constexpr NumericFloatT LinearizesRGBValue(const NumericFloatT in_srgb) {
  static_assert(std::is_floating_point<NumericFloatT>::value,
                "Input must be floating point.");
  return in_srgb < 0.04045 ?
      in_srgb / 12.92 :
      std::pow((in_srgb + 0.055) / (1.0 + 0.055), 2.4);
}

}  // namespace jr

#endif  // JRIMAGE_SRGB_UTILS_H_
