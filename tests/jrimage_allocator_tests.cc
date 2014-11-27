#include <string>
#include <iostream>
#include <vector>

#include "gtest/gtest.h"

#include "jrimage_allocators.h"
#include  "mem_utils.h"

namespace {

// Ensure that our aligned allocator actually allocates properly memory.
TEST(Allocators, AlignedAlloc) {
  const int NUMEL = 100;
  const int NUM_RUNS = 100;

  // Allocate some uchar buffers and make sure the alignment is correct.
  for (int i = 0; i < NUM_RUNS; ++i) {
    std::vector<uint8_t, jr::AlignedAllocator<uint8_t, 0>> nothing_special_1(NUMEL);
    std::vector<uint8_t, jr::AlignedAllocator<uint8_t, 1>> nothing_special_2(NUMEL);

    std::vector<uint8_t, jr::AlignedAllocator<uint8_t, 16>> alignment_sse(NUMEL);
    EXPECT_TRUE(jr::mem_utils::IsPointerAligned(alignment_sse.data(), 16));
    std::vector<uint8_t, jr::AlignedAllocator<uint8_t, 128>> alignment_cacheline(NUMEL);
    EXPECT_TRUE(jr::mem_utils::IsPointerAligned(alignment_cacheline.data(), 128));
  }

  // Allocate some float buffers and make sure the alignment is correct.
  for (int i = 0; i < NUM_RUNS; ++i) {
    std::vector<float, jr::AlignedAllocator<float, 16>> alignment_sse(NUMEL);
    EXPECT_TRUE(jr::mem_utils::IsPointerAligned(alignment_sse.data(), 16));
    std::vector<float, jr::AlignedAllocator<float, 128>> alignment_cacheline(NUMEL);
    EXPECT_TRUE(jr::mem_utils::IsPointerAligned(alignment_cacheline.data(), 128));
  }
}

}
