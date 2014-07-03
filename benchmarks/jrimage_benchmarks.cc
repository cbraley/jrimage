#include <string>
#include <iostream>
#include <random>

#include "benchmark/benchmark.h"

#include "jrimage.h"

namespace {

// Benchmark for the CopyInto function.
void BM_JRImage_CopyInto(benchmark::State& state) {
  jr::Image<float> image(100, 200, 4);
  while (state.KeepRunning()) {
    jr::Image<float> copy;
    image.CopyInto(copy);
  }
}
BENCHMARK(BM_JRImage_CopyInto);

// Benchmark for the SetAll function.
void BM_JRImage_SetAllFloat(benchmark::State& state) {
  jr::Image<float> image(1000, 1000, 3);
  float val = 1.0f;
  while (state.KeepRunning()) {
    image.SetAll(val);
    val += 1.0f;
  }
}
BENCHMARK(BM_JRImage_SetAllFloat);

}  // anonymous namespace
