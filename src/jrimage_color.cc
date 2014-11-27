#include "jrimage_color.h"

namespace jr {

// These declarations have to be here to avoid linker issues.
// http://stackoverflow.com/questions/25954621/access-static-constexpr-member-variable-without-instantiation-c11
constexpr std::array<std::array<double, 3>, 3> ColorSpaceXYZ::MATRIX_TO_XYZ;

}  // namespace jr
