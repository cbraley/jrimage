#include <string>
#include <iostream>
#include <random>

#include "benchmark/benchmark.h"

#include "jrimage.h"

namespace {

// Benchmark for the CopyInto function.
void BM_JRImageBuf_CopyInto(benchmark::State& state) {
  jr::ImageBuf<float> image(100, 200, 4);
  while (state.KeepRunning()) {
    jr::ImageBuf<float> copy;
    image.CopyInto(copy);
  }
}
BENCHMARK(BM_JRImageBuf_CopyInto);

// Benchmark for the SetAll function.
void BM_JRImageBuf_SetAllFloat(benchmark::State& state) {
  jr::ImageBuf<float> image(1000, 1000, 3);
  float val = 1.0f;
  while (state.KeepRunning()) {
    image.SetAll(val);
    val += 1.0f;
  }
}
BENCHMARK(BM_JRImageBuf_SetAllFloat);

}  // anonymous namespace
