#include <string>
#include <iostream>
#include <random>

#include "benchmark/benchmark.h"

#include "mem_utils.h"

namespace {

// Benchmarks for the templates SetMemory function.
template<typename T>
inline void SetMemTestHelper(benchmark::State& state) {
  T* buffer = new T[state.range_x()];
  T value = static_cast<T>(0);
  while (state.KeepRunning()) {
    jr::mem_utils::SetMemory(buffer, value++, state.range_x());
  }
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
      static_cast<int64_t>(state.range_x()));
  delete[] buffer;
}

#define MAKE_SET_MEMORY_BENCHMARK(TYPE) \
    static void BM_memutils_SetMemory_ ## TYPE(benchmark::State& state) { \
      SetMemTestHelper<TYPE>(state); \
    } \
    BENCHMARK(BM_memutils_SetMemory_ ## TYPE)->Arg(8)->Arg(64)->Arg(512)->Arg(1<<10)->Arg(8<<10)->Arg(256<<10);


MAKE_SET_MEMORY_BENCHMARK(uint8_t);
MAKE_SET_MEMORY_BENCHMARK(uint32_t);
MAKE_SET_MEMORY_BENCHMARK(uint64_t);
MAKE_SET_MEMORY_BENCHMARK(float);
MAKE_SET_MEMORY_BENCHMARK(double);

#undef MAKE_SET_MEMORY_BENCHMARK

// Benchmarks for the non-templated MemFill function.

#define MAKE_MEM_FILL_BENCHMARK(FILL_FUNC_NAME) \
static void BM_memutils_ ## FILL_FUNC_NAME(benchmark::State& state) { \
  uint8_t* buffer = new uint8_t[state.range_x()]; \
  uint8_t* pattern = new uint8_t[state.range_y()]; \
  while (state.KeepRunning()) { \
    jr::mem_utils::FILL_FUNC_NAME(buffer, state.range_x(), \
                                  pattern, state.range_y()); \
  } \
  state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * \
      static_cast<int64_t>(state.range_x())); \
  delete[] buffer; \
  delete[] pattern; \
} \
BENCHMARK(BM_memutils_ ## FILL_FUNC_NAME)->RangePair(1<<10, 20<<10, 1, 128);

MAKE_MEM_FILL_BENCHMARK(MemFill);
MAKE_MEM_FILL_BENCHMARK(MemFillSimple);
MAKE_MEM_FILL_BENCHMARK(MemFillChunks);

#undef MAKE_MEM_FILL_BENCHMARK

}  // anonymous namespace
