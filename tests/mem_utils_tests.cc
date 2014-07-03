#include <string>
#include <iostream>
#include <random>
#include <cstdint>
#include <random>
#include <functional>

#include "mem_utils.h"
#include "gtest/gtest.h"

namespace {

template <typename T> struct MemTestBlock {
  MemTestBlock(T* pointer, size_t numel) : ptr(pointer), n(numel) {}
  T* ptr;
  size_t n;
};

template <typename T>
void AlignedAllocTest(size_t align, const std::function<T(int)>& func,
                      int max_array_size, int num_tests) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, max_array_size);

  std::vector<MemTestBlock<T>> mem_blocks;

  size_t size_sum = 0;
  for (int i = 0; i < num_tests; ++i) {
    const size_t numel = dis(gen);
    size_sum += numel;

    T* mem = jr::mem_utils::AlignedNew<T>(numel, align);
    EXPECT_TRUE(jr::mem_utils::IsPointerAligned(mem, align));

    for (int i = 0; i < numel; ++i) {
      mem[i] = func(i);
    }

    mem_blocks.push_back(MemTestBlock<T>(mem, numel));
  }

  EXPECT_EQ(num_tests, mem_blocks.size());
  size_t size_sum_check = 0;
  for (size_t j = 0; j < mem_blocks.size(); ++j) {
    T* mem = mem_blocks[j].ptr;
    const size_t numel = mem_blocks[j].n;
    size_sum_check += numel;

    for (int i = numel - 1; i >= 0; --i) {
      const T expected = func(i);
      EXPECT_EQ(expected, mem[i]);
    }
    jr::mem_utils::AlignedDelete(mem);
  }
  EXPECT_EQ(size_sum, size_sum_check);
}

template <typename T> T TestFunc(int index) {
  if (index == 0) {
    return static_cast<T>(54634646.0);
  }
  return static_cast<T>(sin(static_cast<double>(index)) + 22.30 +
                        static_cast<double>(index * 3.0));
}

TEST(MemUtils, AlignedAllocStressTests) {
  const int num_tests = 500;
  const int ARRAY_LIMITS[] = { 0, 1, 10, 1012};
  for (int i = 0; i < sizeof(ARRAY_LIMITS) / sizeof(ARRAY_LIMITS[0]); ++i) {
    const int arr_size = ARRAY_LIMITS[i];

    AlignedAllocTest<uint8_t>(0, TestFunc<uint8_t>, arr_size, num_tests);
    AlignedAllocTest<uint8_t>(2, TestFunc<uint8_t>, arr_size, num_tests);
    AlignedAllocTest<uint8_t>(4, TestFunc<uint8_t>, arr_size, num_tests);
    AlignedAllocTest<uint8_t>(16, TestFunc<uint8_t>, arr_size, num_tests);
    AlignedAllocTest<uint8_t>(128, TestFunc<uint8_t>, arr_size, num_tests);
    AlignedAllocTest<uint8_t>(256, TestFunc<uint8_t>, arr_size, num_tests);
    AlignedAllocTest<uint8_t>(1024, TestFunc<uint8_t>, arr_size, num_tests);
    AlignedAllocTest<uint8_t>(2048, TestFunc<uint8_t>, arr_size, num_tests);

    AlignedAllocTest<float>(0, TestFunc<float>, arr_size, num_tests);
    AlignedAllocTest<float>(2, TestFunc<float>, arr_size, num_tests);
    AlignedAllocTest<float>(4, TestFunc<float>, arr_size, num_tests);
    AlignedAllocTest<float>(16, TestFunc<float>, arr_size, num_tests);
    AlignedAllocTest<float>(128, TestFunc<float>, arr_size, num_tests);
    AlignedAllocTest<float>(256, TestFunc<float>, arr_size, num_tests);
    AlignedAllocTest<float>(1024, TestFunc<float>, arr_size, num_tests);
    AlignedAllocTest<float>(2048, TestFunc<float>, arr_size, num_tests);

    AlignedAllocTest<double>(0, TestFunc<double>, arr_size, num_tests);
    AlignedAllocTest<double>(2, TestFunc<double>, arr_size, num_tests);
    AlignedAllocTest<double>(4, TestFunc<double>, arr_size, num_tests);
    AlignedAllocTest<double>(16, TestFunc<double>, arr_size, num_tests);
    AlignedAllocTest<double>(128, TestFunc<double>, arr_size, num_tests);
    AlignedAllocTest<double>(256, TestFunc<double>, arr_size, num_tests);
    AlignedAllocTest<double>(1024, TestFunc<double>, arr_size, num_tests);
    AlignedAllocTest<double>(2048, TestFunc<double>, arr_size, num_tests);
  }
}

TEST(MemUtils, IsPointerAligned) {
  uint8_t* pointer = reinterpret_cast<uint8_t*>(0x0F);

  EXPECT_TRUE(jr::mem_utils::IsPointerAligned<uint8_t>(pointer, 0));
  EXPECT_TRUE(jr::mem_utils::IsPointerAligned<uint8_t>(pointer, 1));
  EXPECT_TRUE(jr::mem_utils::IsPointerAligned<uint8_t>(pointer, 3));
  EXPECT_TRUE(jr::mem_utils::IsPointerAligned<uint8_t>(pointer, 5));
  EXPECT_TRUE(jr::mem_utils::IsPointerAligned<uint8_t>(pointer, 15));

  EXPECT_FALSE(jr::mem_utils::IsPointerAligned<uint8_t>(pointer, 2));
  EXPECT_FALSE(jr::mem_utils::IsPointerAligned<uint8_t>(pointer, 4));
  EXPECT_FALSE(jr::mem_utils::IsPointerAligned<uint8_t>(pointer, 6));
  EXPECT_FALSE(jr::mem_utils::IsPointerAligned<uint8_t>(pointer, 7));
  EXPECT_FALSE(jr::mem_utils::IsPointerAligned<uint8_t>(pointer, 8));
  EXPECT_FALSE(jr::mem_utils::IsPointerAligned<uint8_t>(pointer, 9));
  EXPECT_FALSE(jr::mem_utils::IsPointerAligned<uint8_t>(pointer, 10));
  EXPECT_FALSE(jr::mem_utils::IsPointerAligned<uint8_t>(pointer, 11));
  EXPECT_FALSE(jr::mem_utils::IsPointerAligned<uint8_t>(pointer, 12));
  EXPECT_FALSE(jr::mem_utils::IsPointerAligned<uint8_t>(pointer, 13));
  EXPECT_FALSE(jr::mem_utils::IsPointerAligned<uint8_t>(pointer, 14));

  for (std::size_t i = 16; i < 2048; ++i) {
    EXPECT_FALSE(jr::mem_utils::IsPointerAligned<uint8_t>(pointer, i));
  }
}

uint8_t* AllocateRandomBuffer(std::size_t num_bytes) {
  std::random_device rd;
  std::default_random_engine rng(rd());
  std::uniform_int_distribution<uint8_t> dist(0, std::numeric_limits<uint8_t>::max());
  uint8_t* buf = new uint8_t[num_bytes];
  for (std::size_t i = 0; i < num_bytes; ++i) {
    buf[i] = dist(rng);
  }
  return buf;
}

TEST(MemUtils, MemFillVariantStressTest) {
  const int MAX_BUF_SIZE = 100;
  const int MAX_FILL_SIZE = 200;
  for (int i = 1; i < MAX_BUF_SIZE; ++i) {
    for (int j = 1; j < MAX_FILL_SIZE; ++j) {
      uint8_t* buf_basic = new uint8_t[i];
      uint8_t* buf_simple = new uint8_t[i];
      uint8_t* buf_chunks = new uint8_t[i];

      uint8_t* pattern = AllocateRandomBuffer(j);

      jr::mem_utils::MemFill(buf_basic, i, pattern, j);
      jr::mem_utils::MemFillSimple(buf_simple, i, pattern, j);
      jr::mem_utils::MemFillChunks(buf_chunks, i, pattern, j);

      ASSERT_EQ(0, memcmp(buf_basic, buf_simple, i))
        << "basic buffer and simple buffer disagree for a buffer size of "
        << i << " and a pattern size of " << j;
      ASSERT_EQ(0, memcmp(buf_basic, buf_chunks, i))
        << "basic buffer and chunk buffer disagree for a buffer size of "
        << i << " and a pattern size of " << j;
      ASSERT_EQ(0, memcmp(buf_simple, buf_chunks, i))
        << "simple buffer and simple buffer disagree for a buffer size of "
        << i << " and a pattern size of " << j;

      delete[] pattern;
      delete[] buf_basic;
      delete[] buf_simple;
      delete[] buf_chunks;
    }
  }
}

TEST(MemUtils, MemFillEdgeCases) {
  // Array of the various MemFill.* functions to test with.
  std::function<void*(void*, std::size_t, const void*, std::size_t)> funcs[3] =
      { jr::mem_utils::MemFillSimple,
        jr::mem_utils::MemFill,
        jr::mem_utils::MemFillChunks };

  for (int i = 0; i < 3; ++i) {
    // Fresh data each iteration.
    float A[3] = {1.0, 2.0, 3.0};
    float B[1] = {2.0};
    uint8_t C[1] = {12};
    uint8_t D[4] = {255, 12, 0, 123};

    auto func = funcs[i];

    const float gold_float = 5.5f;
    const uint8_t gold_uchar = 243;

    func(A, sizeof(A), reinterpret_cast<const void*>(&gold_float), sizeof(float));
    EXPECT_EQ(gold_float, A[0]);
    EXPECT_EQ(gold_float, A[1]);
    EXPECT_EQ(gold_float, A[2]);

    func(B, sizeof(B), reinterpret_cast<const void*>(&gold_float), sizeof(float));
    EXPECT_EQ(gold_float, B[0]);

    func(C, sizeof(C), reinterpret_cast<const void*>(&gold_uchar), sizeof(uint8_t));
    EXPECT_EQ(gold_uchar, C[0]);

    func(D, sizeof(D), reinterpret_cast<const void*>(&gold_uchar), sizeof(uint8_t));
    EXPECT_EQ(gold_float, A[0]);
    EXPECT_EQ(gold_float, A[1]);
    EXPECT_EQ(gold_float, A[2]);

    // Now try using a bit-pattern larger than the buffer to write to.
    const uint8_t gold_long_pattern[7] = {0, 1, 2, 5, 55, 77, 125};
    func(D, sizeof(D), reinterpret_cast<const void*>(gold_long_pattern), sizeof(gold_long_pattern));
    for (int j = 0; j < 4; ++j) {
      EXPECT_EQ(gold_long_pattern[j], D[j]);
    }

  }
}



}  // anonymous namespace

