#ifndef JRIMAGE_COLOR_H_
#define JRIMAGE_COLOR_H_

#include <array>
#include <algorithm>
#include <iostream>
#include <cassert>
#include <type_traits>

#include "template_utils.h"

namespace jr {

// Templated color class.  The type ColorSpaceT describes the color space that is
// used, and ChannelT describes the level of precision used to store each color
// measurement.
// Template argument requirements:
//   - The template type ColorSpaceT must be a valid ColorSpace type.
//     See the documentation TODO(here) for more info.
//   - The type ChannelT must be a fundamental type.
template<typename ColorSpaceT, typename ChannelT>
class Color {
 public:
  // Color space template argument.
  typedef ColorSpaceT ColorSpaceType;

  // Numeric type used to store data values.
  typedef ChannelT ChannelType;

  // Numeric type used for intermediate computations.  For integer types,
  // we perform intermediate computation with single precision floats.  For
  // non-integral types, we use the type itself.
  // For example: if ChannelT == double, we use doubles for internal
  // computations.
  typedef typename  std::conditional<
      std::is_floating_point<ChannelT>::value,
      ChannelT, float>::type WorkingChannelTypeT;

  // Default constructor.  Creates a color with all 0s.
  Color() {
    SetAllTo(ChannelT(0.0));
  }

  // Set all pixel values to a single value.
  void SetAllTo(ChannelT value) {
    std::fill(values, values + 3, value);
  }

  // 3 color values.
  ChannelT values[3];  // 32 * 3

  // TODO*cbraley): What about a union with pointers to colors ?
  //ChannelT* ptr;       // 64 * 1

  static_assert(std::is_fundamental<ChannelT>::value,
                "Colors only work with fundamental numeric types");
};

// A color transformation matrix is stored as a 3x3 std::array of doubles.
typedef std::array<std::array<double, 3>, 3> ColorTransformationMat;

// CIE XYZ color space.
class ColorSpaceXYZ {
 public:
  // XYZ color is linear.
  typedef std::integral_constant<bool, true> is_linear;

  // XYZ color is not perceptually uniform.
  typedef std::integral_constant<bool, false> is_perceptually_uniform;

  // Since XYZ color is linear we provide a matrix to convert to
  // the reference color space (XYZ).  In this case, this is the
  // identity matrix.  Note that we use template specializations
  // in most cases to avoid multiplying by the identity matrix when
  // such a decision can be made at compile time.
  static constexpr ColorTransformationMat MATRIX_TO_XYZ = {{
      {1.0, 0.0, 0.0},
      {0.0, 1.0, 0.0},
      {0.0, 0.0, 1.0}}};
};



// sRGB color space.
class ColorSpaceSRGB {
 public:
  // sRGB color is *NOT* linear.
  typedef std::integral_constant<bool, false> is_linear;

  // sRGB color is not perceptually uniform.
  typedef std::integral_constant<bool, false> is_perceptually_uniform;

  // Since sRGB color is nonlinear, we can't provide a matrix to
  // convert to XYZ.  Instead, use ToXYZ(...) and FromXYZ(...) functions.
  template<typename ChannelT>
  static constexpr Color<ColorSpaceXYZ, ChannelT> ToXYZ(
      const Color<ColorSpaceSRGB, ChannelT>& from) {
    return Color<ColorSpaceXYZ, ChannelT>();
  }
  template<typename ChannelT>
  static constexpr Color<ColorSpaceSRGB, ChannelT> FromXYZ(
      const Color<ColorSpaceXYZ, ChannelT>& from) {
    return Color<ColorSpaceSRGB, ChannelT>();
  }
};


// Convert colors from color space from_data to color space to_data.
template<typename ColorSpaceFrom, typename ColorSpaceTo, typename ChannelT>
void ConvertColorSpace(
    const Color<ColorSpaceFrom, ChannelT> *from_data,
    Color<ColorSpaceTo, ChannelT> *to_data,
    int count);


// Implementation details only below this line. -------------------------------

// Forward declarations of implementation details.
namespace implementation_details {

template<typename ColorSpaceFrom, typename ColorSpaceTo, typename ChannelT>
void ColorConvertImpl(
    const Color<ColorSpaceFrom, ChannelT> *from_data,
    Color<ColorSpaceTo, ChannelT> *to_data, int count,
    std::true_type from_color_space_is_linear,
    std::true_type dest_color_space_is_linear);

template<typename ColorSpaceFrom, typename ColorSpaceTo, typename ChannelT>
void ColorConvertImpl(
    const Color<ColorSpaceFrom, ChannelT> *from_data,
    Color<ColorSpaceTo, ChannelT> *to_data, int count,
    std::true_type  from_color_space_is_linear,
    std::false_type dest_color_space_is_linear);

template<typename ColorSpaceFrom, typename ColorSpaceTo, typename ChannelT>
void ColorConvertImpl(
    const Color<ColorSpaceFrom, ChannelT> *from_data,
    Color<ColorSpaceTo, ChannelT> *to_data, int count,
    std::false_type from_color_space_is_linear,
    std::true_type  dest_color_space_is_linear);

template<typename ColorSpaceFrom, typename ColorSpaceTo, typename ChannelT>
void ColorConvertImpl(
    const Color<ColorSpaceFrom, ChannelT> *from_data,
    Color<ColorSpaceTo, ChannelT> *to_data, int count,
    std::false_type from_color_space_is_linear,
    std::false_type dest_color_space_is_linear);

template<typename ChannelT, typename WorkingChannelTypeT>
void MatrixTimesVector(const ColorTransformationMat &mat,
                       const ChannelT *vector,
                       ChannelT *out_vector);

}  // namespace implementation_details


// Color conversion functions.  These functions convert a buffer of colors from
// one colorspace to another.
template<typename ColorSpaceFrom, typename ColorSpaceTo, typename ChannelT>
void ConvertColorSpace(
    const Color<ColorSpaceFrom, ChannelT> *from_data,
    Color<ColorSpaceTo, ChannelT> *to_data,
    int count) {
  assert(count >= 0);

  // If the color spaces are the same, this is just a simple memcpy.
  if (std::is_same<ColorSpaceFrom, ColorSpaceTo>::value) {
    memcpy(static_cast<void *>(to_data), static_cast<const void *>(from_data),
           count * sizeof(ChannelT));
    return;
  }

  // Here we dispatch to an implementation function using tag dispatching.
  // Which function we call depends on the linearity of each color space.  For
  // example, if both color spaces are linear, we can do this conversion via
  // a simple matrix multiply.
  typename ColorSpaceFrom::is_linear::type from_cs_is_linear;
  typename ColorSpaceFrom::is_linear::type to_cs_is_linear;
  implementation_details::ColorConvertImpl<ColorSpaceFrom,
                                           ColorSpaceTo,
                                           ChannelT>(
    from_data, to_data, count,
    from_cs_is_linear, to_cs_is_linear);
}

namespace implementation_details {

// For linear -> linear.
template<typename ColorSpaceFrom, typename ColorSpaceTo, typename ChannelT>
void ColorConvertImpl(
    const Color<ColorSpaceFrom, ChannelT> *from_data,
    Color<ColorSpaceTo, ChannelT> *to_data, int count,
    std::true_type, std::true_type) {

  // Since both color spaces are linear, we can use a single matrix multiply
  // for every single value.  We simply pre-multiply the two color space's
  // CS->XYZ matrices.
  // For example, consider if the matrices M1 and M2 convert color spaces
  // CS1 and CS2 to XYZ color.  If we form the matrix:
  // Mconv = M1 * M2, then we can use Mconv to convert between CS1 and CS2.
  typedef typename Color<ColorSpaceFrom, ChannelT>::WorkingChannelTypeT WorkingT;
  std::array<std::array<WorkingT, 3>, 3> color_conv_matrix;
  // TODO(cbraley): Both of these matrices are known at compile time, so we
  // could do this matrix multiplication at compile time.
  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 3; ++c) {
      double accum = 0.0;
      for (int m = 0; m < 3; ++m) {
        accum += ColorSpaceFrom::MATRIX_TO_XYZ[r][m] *
                 ColorSpaceTo::MATRIX_TO_XYZ[m][c];
      }
      color_conv_matrix[r][c] = WorkingT(accum);
    }
  }

  // Perform N=count matrix multiplies.
  // TODO(cbraley): Speed this up with SEE intrinsics.
  for (int i = 0; i < count; ++i) {
    const ChannelT *from_data_ptr = from_data[i].values;
    ChannelT* to_data_ptr = to_data[i].values;

    // Perform a matrix-vector multiply.
    for (int r = 0; r < 3; ++r) {
      WorkingT accum(0.0);
      for (int c = 0; c < 3; ++c) {
        accum += color_conv_matrix[r][c] * from_data_ptr[c];
      }
      to_data_ptr[r] = accum;
    }
  }
}

// For linear -> nonlinear.
template<typename ColorSpaceFrom, typename ColorSpaceTo, typename ChannelT>
void ColorConvertImpl(
    const Color<ColorSpaceFrom, ChannelT> *from_data,
    Color<ColorSpaceTo, ChannelT> *to_data, int count,
    std::true_type, std::false_type) {
  for (int i = 0; i < count; ++i) {
    // Convert to XYZ using a matrix, then apply a non-linear transformation.
    Color<ColorSpaceXYZ, ChannelT> xyz;
    MatrixTimesVector<ChannelT,
                      Color<ColorSpaceFrom, ChannelT>::WorkingChannelTypeT>(
        Color<ColorSpaceFrom, ChannelT>::MATRIX_TO_XYZ,
        from_data[i].values,
        xyz.values);
    // Apply nonlinear function to convert to ColorSpaceTo.
    to_data[i].template FromXYZ<ChannelT>(xyz);
  }
}

// For nonlinear -> nonlinear.
template<typename ColorSpaceFrom, typename ColorSpaceTo, typename ChannelT>
void ColorConvertImpl(
    const Color<ColorSpaceFrom, ChannelT> *from_data,
    Color<ColorSpaceTo, ChannelT> *to_data, int count,
    std::false_type, std::true_type) {
  std::cout << "nonlinear to linear!" << std::endl;
  for (int i = 0; i < count; ++i) {
    // Apply nonlinear function to convert to XYZ.
    Color<ColorSpaceXYZ, ChannelT> xyz;
    to_data[i].template ToXYZ<ChannelT>(xyz);

    // Perform a matrix multiplication to convert to the destination color
    MatrixTimesVector<ChannelT,
                      Color<ColorSpaceTo, ChannelT>::WorkingChannelTypeT>(
        Color<ColorSpaceTo, ChannelT>::MATRIX_TO_XYZ,  // TODO(cbraley): Invert!
        xyz.values,
        to_data[i].values);
  }
}

// FOr nonlinear -> nonlinear.
template<typename ColorSpaceFrom, typename ColorSpaceTo, typename ChannelT>
void ColorConvertImpl(
    const Color<ColorSpaceFrom, ChannelT> *from_data,
    Color<ColorSpaceTo, ChannelT> *to_data, int count,
    std::false_type, std::false_type) {
  std::cout << "nonlinear to nonlinear!" << std::endl;
}

template<typename ChannelT, typename WorkingChannelTypeT>
void MatrixTimesVector(const ColorTransformationMat &mat,
                       const ChannelT *vector,
                       ChannelT *out_vector) {
  for (int r = 0; r < 3; ++r) {
    WorkingChannelTypeT accum(0.0);
    for (int c = 0; c < 3; ++c) {
      accum += mat[r][c] * vector[c];
    }
    // TODO(cbraley): Saturate!
    out_vector[r] = ChannelT(accum);
  }
}


}  // namespace implementation_details


}  // namespace jr

#endif  // JRIMAGE_COLOR_H_
