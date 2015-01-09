#include <string>
#include <iostream>
#include <random>
#include <limits>

#include "gtest/gtest.h"

#include "math_utils.h"

using namespace jr::math_utils;

namespace {

TEST(MathUtils, Clamp) {
  EXPECT_EQ(12, jr::math_utils::Clamp<int>(-10, 12, 44));
  EXPECT_EQ(12, jr::math_utils::Clamp<int>(11, 12, 44));
  EXPECT_EQ(12, jr::math_utils::Clamp<int>(12, 12, 44));
  EXPECT_EQ(44, jr::math_utils::Clamp<int>(44, 12, 44));
  EXPECT_EQ(44, jr::math_utils::Clamp<int>(45, 12, 44));
  EXPECT_EQ(25, jr::math_utils::Clamp<int>(25, 12, 44));
}

TEST(MathUtils, ConvertWithSaturation) {
  // Here we are converting from a wider type to a narrower type.
  unsigned char v = ConvertWithSaturation<int, unsigned char>(256);
  EXPECT_EQ(255, v) << "Expected 255 but got " << +v;
  v = ConvertWithSaturation<unsigned int, unsigned char>(356);
  EXPECT_EQ(255, v);
  v = ConvertWithSaturation<int, unsigned char>(226);
  EXPECT_EQ(226, v);
  v = ConvertWithSaturation<unsigned char, unsigned char>(216);
  EXPECT_EQ(216, v);
  v = ConvertWithSaturation<int, unsigned char>(-1);
  EXPECT_EQ(0, v);
  v = ConvertWithSaturation<int, unsigned char>(-1000);
  EXPECT_EQ(0, v);

  // Test a conversion from a narrow type to a wider type.
  for (unsigned char i = std::numeric_limits<unsigned char>::lowest();
       i < std::numeric_limits<unsigned char>::max(); ++i) {
    const int sat_int = ConvertWithSaturation<unsigned char, int>(i);
    EXPECT_EQ(i, sat_int);
    const unsigned int sat_uint =
        ConvertWithSaturation<unsigned char, unsigned int>(i);
    EXPECT_EQ(i, sat_uint);
  }
}

TEST(MathUtils, MixedSignConvertWithSaturation) {
  /*
  unsigned int zero_uint = 0;
  char neg_ten_char = -10;
  ASSERT_TRUE(neg_ten_char < zero_uint);
  */

  // Test mixed-sign conversion.
  const char in_val = -10;
  const unsigned int val = ConvertWithSaturation<char, unsigned int>(in_val);
  EXPECT_EQ(0, val);
}

}  // anonymous namespace

