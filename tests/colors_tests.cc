#include <string>
#include <iostream>
#include <random>
#include <cstdint>

#include "gtest/gtest.h"

#include "jrimage_color.h"

namespace jr {

// Ensure that all Color types are trivially copyable.  This is a requirement
// for use as pixel types.  It is probably overkill to do this test for all
// different template argument widths.
#define ENSURE_TRIVIALLY_COPYABLE_FOR_ALL_TYPES(CS) \
  static_assert(std::is_trivially_copyable<Color<CS, char>>::value, \
               "Color< #CS, char> wasn't trivially copyable."); \
  static_assert(std::is_trivially_copyable<Color<CS, short int>>::value, \
               "Color< #CS, short> wasn't trivially copyable."); \
  static_assert(std::is_trivially_copyable<Color<CS, int>>::value, \
               "Color< #CS, short> wasn't trivially copyable."); \
  static_assert(std::is_trivially_copyable<Color<CS, long int>>::value, \
               "Color< #CS, short> wasn't trivially copyable."); \
  static_assert(std::is_trivially_copyable<Color<CS, float>>::value, \
               "Color< #CS, short> wasn't trivially copyable."); \
  static_assert(std::is_trivially_copyable<Color<CS, double>>::value, \
               "Color< #CS, short> wasn't trivially copyable.")

ENSURE_TRIVIALLY_COPYABLE_FOR_ALL_TYPES(ColorSpaceXYZ);



TEST(JRImageColor, ColorBasics) {
  Color<ColorSpaceXYZ, float> hdr_xyz;
  Color<ColorSpaceSRGB, float> hdr_srgb;
  Color<ColorSpaceSRGB, uint8_t> ldr_srgb;
}


TEST(JRImageColor, LinearConversions) {
  std::unique_ptr<Color<ColorSpaceXYZ, float>> hdr_xyz_1(
      new Color<ColorSpaceXYZ, float>[100]);
  std::unique_ptr<Color<ColorSpaceXYZ, float>> hdr_xyz_2(
      new Color<ColorSpaceXYZ, float>[100]);
  
  ConvertColorSpace(hdr_xyz_1.get(), hdr_xyz_2.get(), 100);

}

}  // namespace jr

