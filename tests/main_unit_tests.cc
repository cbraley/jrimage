#include <string>
#include <iostream>

#include "gtest/gtest.h"

void PrintDebugInfo() {}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  PrintDebugInfo();
  return RUN_ALL_TESTS();
}

