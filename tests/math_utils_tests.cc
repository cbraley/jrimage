#include <string>
#include <iostream>
#include <random>
#include <limits>
#include <cstdint>

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

TEST(MathUtils, ConvertWithSaturationWideToNarrow) {
  // Test uint -> uint8_t.
  uint8_t v = ConvertWithSaturation<uint32_t, uint8_t>(256);
  EXPECT_EQ(255, v) << "Expected 255 but got " << +v;
  v = ConvertWithSaturation<uint32_t, uint8_t>(356);
  EXPECT_EQ(255, v);
  v = ConvertWithSaturation<uint32_t, uint8_t>(226);
  EXPECT_EQ(226, v);
  v = ConvertWithSaturation<uint8_t, uint8_t>(216);
  EXPECT_EQ(216, v);

  // Test int->int8_t.
  int8_t v2 = 0;
  v2 = ConvertWithSaturation<int, int8_t>(-1);
  EXPECT_EQ(-1, v2) << "Expected -1 but got " << int(v2);
  v2 = ConvertWithSaturation<int, int8_t>(-1000);
  EXPECT_EQ(-128, v2);

  // Test int->short.
  short v3 = ConvertWithSaturation<int32_t, short>(100);
  EXPECT_EQ(100, v3);
  v3 = ConvertWithSaturation<int32_t, short>(0);
  EXPECT_EQ(0, v3);
  v3 = ConvertWithSaturation<int32_t, short>(-101);
  EXPECT_EQ(-101, v3);
  v3 = ConvertWithSaturation<int32_t, short>(32767);
  EXPECT_EQ(32767, v3);
  v3 = ConvertWithSaturation<int32_t, short>(32768);
  EXPECT_EQ(32767, v3);
  v3 = ConvertWithSaturation<int32_t, short>(-32768);
  EXPECT_EQ(-32768, v3);
  v3 = ConvertWithSaturation<int32_t, short>(-32769);
  EXPECT_EQ(-32768, v3);
}

TEST(MathUtils, ConvertWithSaturationNarrowToWide) {
  // Test uint8_t -> uint.
  for (uint8_t i = std::numeric_limits<uint8_t>::lowest();
       i < std::numeric_limits<uint8_t>::max(); ++i) {
    const uint32_t sat_uint = ConvertWithSaturation<uint8_t, uint32_t>(i);
    EXPECT_EQ(i, sat_uint);
  }

  // Test int8_t -> int32_t.
  for (int8_t i = std::numeric_limits<int8_t>::lowest();
       i < std::numeric_limits<int8_t>::max(); ++i) {
    const int32_t sat_int = ConvertWithSaturation<int8_t, int32_t>(i);
    EXPECT_EQ(i, sat_int);
  }
}

TEST(MathUtils, ConvertWithSaturationFloatsAndIntegers) {
  // int8_t->float and int8_t->double.
  for (int8_t i = std::numeric_limits<int8_t>::lowest();
       i < std::numeric_limits<int8_t>::max(); ++i) {
    const float sat_float = ConvertWithSaturation<int8_t, float>(i);
    EXPECT_EQ(float(i), sat_float);
    const double sat_double = ConvertWithSaturation<int8_t, double>(i);
    EXPECT_EQ(double(i), sat_double);
  }

  // float->int8_t.
  int8_t c = ConvertWithSaturation<float, int8_t>(254.0f);
  EXPECT_EQ(127, c);
  c = ConvertWithSaturation<float, int8_t>(217.0f);
  EXPECT_EQ(127, c);
  c = ConvertWithSaturation<float, int8_t>(100.0f);
  EXPECT_EQ(100, c);
  c = ConvertWithSaturation<float, int8_t>(0.0f);
  EXPECT_EQ(0, c);
  c = ConvertWithSaturation<float, int8_t>(-22.0f);
  EXPECT_EQ(-22, c);
  c = ConvertWithSaturation<float, int8_t>(-9999.0f);
  EXPECT_EQ(-128, c);
}

}  // anonymous namespace

