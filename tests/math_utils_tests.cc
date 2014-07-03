#include <string>
#include <iostream>
#include <random>

#include "gtest/gtest.h"

#include "math_utils.h"

namespace {

TEST(MathUtils, Clamp) {
  EXPECT_EQ(12, jr::math_utils::Clamp<int>(-10, 12, 44));
  EXPECT_EQ(12, jr::math_utils::Clamp<int>(11, 12, 44));
  EXPECT_EQ(12, jr::math_utils::Clamp<int>(12, 12, 44));
  EXPECT_EQ(44, jr::math_utils::Clamp<int>(44, 12, 44));
  EXPECT_EQ(44, jr::math_utils::Clamp<int>(45, 12, 44));
  EXPECT_EQ(25, jr::math_utils::Clamp<int>(25, 12, 44));
}

}  // anonymous namespace

