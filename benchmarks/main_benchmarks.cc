#include <string>
#include <iostream>

#include "benchmark/benchmark.h"

int main(int argc, const char** argv) {
#ifdef SLOW_AND_STEADY
  std::cout << "Compiled with SLOW_AND_STEADY!" << std::endl;
#else 
  std::cout << "Normal compilation." << std::endl;
#endif
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  return 0;
}


