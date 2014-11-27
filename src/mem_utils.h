#ifndef JRIMAGE_MEMUTILS_H_
#define JRIMAGE_MEMUTILS_H_

#include <cassert>
#include <cstring>
#include <cstdint>
#include <memory>

#include <algorithm>
#include <iostream>
#include <memory>
#include <type_traits>

namespace jr {

/// Utility functions for working with memory.
namespace mem_utils {

/// Return true if the two arrays are different.  Optionally, we can return the
/// index of the first difference via the diff_index parameter.  If the arrays
/// differ, the output value of diff_index is undefined.
template<typename T>
bool ArraysAreDifferent(const T* const buffer_a, const T* const buffer_b,
                        std::size_t num_elements,
                        std::size_t* diff_index = nullptr);


// TODO(cbraley): Rename to memfill and make similar to memset?

/// Set the num values starting at ptr to value.  This is equivalent to:
/// ptr[0] = ptr[1] = ... = ptr[num-1] = value;
///
/// This function is similar to the SetMemory(...) functions, but this form
/// can be more convienient when writing template code.  Furthermore, this
/// function can be marginally more effecient than MemFill variants since
/// the size of the "pattern" is known at compile-time.
template<typename T>
void SetMemory(T* ptr, const T& value, std::size_t num);

void* MemFill(void* buffer, std::size_t buffer_size_bytes,
              const void* pattern, std::size_t pattern_size_bytes);

void* MemFillSimple(void* buffer, std::size_t buffer_size_bytes,
                    const void* pattern, std::size_t pattern_size_bytes);

void* MemFillChunks(void* buffer, std::size_t buffer_size_bytes,
                    const void* pattern, std::size_t pattern_size_bytes);


/// Return true if the pointer is aligned to a byte_alignment boundary.
template<typename T>
bool IsPointerAligned(const T* const pointer, std::size_t byte_alignment);


struct AlignmentMetadata {
  // - base_mem_addr: the memory location of the base address that came directly
  //   from the new[] operator.
  // - aligned_addr: the aligned address to be returned to the caller.
  // - alignment_req:  the byte alignment requirement passed into the allocator.
  inline AlignmentMetadata(const void* const base_mem_addr,
                           const void* const aligned_addr,
                           std::size_t alignment_req)
      : base_addr(reinterpret_cast<uintptr_t>(base_mem_addr)) {
  }

  inline uint8_t* BaseAddress(uint8_t* client_address) const {
    return reinterpret_cast<uint8_t*>(base_addr);
  }

 private:
  uintptr_t base_addr;
};
static_assert(std::is_standard_layout<AlignmentMetadata>::value,
              "AlignmentMetadata objects must be have standard layout since "
              "they are used with reinterpret_cast.");

template<typename T>
T* AlignedNew(std::size_t num_Ts, std::size_t byte_alignment) {
  // byte_alignment must be a power of 2 or 0.
  const bool byte_alignmnt_ok = ((byte_alignment & (byte_alignment - 1)) == 0);
  assert(byte_alignmnt_ok);
  if (!byte_alignmnt_ok) {
    return nullptr;
  }

  // TODO(cbraley): Test this with larger AlignmentMetadata type; such as those we
  // might use when debugging.
  // We need to allocate the AlignmentMetadata properly as well.
  byte_alignment = std::max(byte_alignment, alignof(AlignmentMetadata));

  const std::size_t bytes_to_allocate =
      num_Ts * sizeof(T) +  // Base mem requested by caller.
      sizeof(AlignmentMetadata) +  // Alignment metadata.
      byte_alignment;  // Slack space.

  // Allocate chunk with extra size from the memory allocator.
  uint8_t* buffer = new uint8_t[bytes_to_allocate];
  if (buffer == nullptr) {
    return nullptr;
  }

  // Make room for the metadata.
  void* aligned_buffer = buffer + sizeof(AlignmentMetadata);
  std::size_t aligned_buffer_space = buffer - static_cast<uint8_t*>(aligned_buffer);

  // Compute the first aligned chunk.
  std::align(byte_alignment,
             num_Ts * sizeof(T),
             aligned_buffer,
             aligned_buffer_space);
  assert(aligned_buffer != nullptr);
  assert(IsPointerAligned<void>(aligned_buffer, byte_alignment));

  // Write the metadata.
  *reinterpret_cast<AlignmentMetadata*>(static_cast<uint8_t*>(aligned_buffer) -
                                    sizeof(AlignmentMetadata)) =
      AlignmentMetadata(buffer, aligned_buffer, byte_alignment);

  return reinterpret_cast<T*>(aligned_buffer);
}

template<typename T>
void AlignedDelete(T* to_delete) {
  if (to_delete != nullptr) {
    //  const AlignmentMetadata& alignment_info =
    //      *(reinterpret_cast<const AlignmentMetadata* const>(to_delete) - 1);
    delete[](reinterpret_cast<const AlignmentMetadata* const>(to_delete) - 1)
        ->BaseAddress(reinterpret_cast<uint8_t*>(to_delete));
  }
}


// Implementation details only below this line. -------------------------------

namespace implementation_details {
template<std::size_t TYPE_SIZE_BYTES>
void SetMem(uint8_t* ptr, const uint8_t* const value_to_set, std::size_t num);
}  // namespace implementation_details

template<typename T>
inline bool ArraysAreDifferent(const T* const buffer_a, const T* const buffer_b,
                        std::size_t num_elements, std::size_t* diff_index) {
  // If the caller does not care about the location of the first difference,
  // then we can just call memcmp.
  if (diff_index == nullptr) {
    return memcmp(static_cast<const void*>(buffer_a),
                  static_cast<const void*>(buffer_b),
                  num_elements * sizeof(T)) != 0;
  }

  for (std::size_t i = 0; i < num_elements; ++i) {
    if (buffer_a[i] != buffer_b[i]) {
      if (diff_index != nullptr) {
        *diff_index = i;
      }
      return true;
    }
  }
  return false;
}

template<typename T>
inline void SetMemory(T* ptr, const T& value, std::size_t num) {
  std::fill(ptr, ptr + num, value);
  /*
  implementation_details::SetMem<sizeof(T)>(
      reinterpret_cast<uint8_t*>(ptr),
      reinterpret_cast<const uint8_t*>(&value),
      num);
  */
  // TODO(cbraley): Why is std::fill so fast!
}

template<typename T>
inline bool IsPointerAligned(const T* const pointer, std::size_t byte_alignment) {
  return byte_alignment == 0 ||
      reinterpret_cast<uintptr_t>(pointer) % byte_alignment == 0;
}


namespace implementation_details {

template<std::size_t TYPE_SIZE_BYTES>
void SetMem(uint8_t* ptr, const uint8_t* const value_to_set, std::size_t num) {
  MemFill(static_cast<void*>(ptr),
          num * TYPE_SIZE_BYTES,
          static_cast<const void*>(value_to_set),
          TYPE_SIZE_BYTES);
}

// Template specialization of SetMem that calls memset for single-byte data.
template<>
void SetMem<1>(uint8_t* ptr,
               const uint8_t* const value_to_set,
               std::size_t num);

}  // namespace implementation_details


}  // namespace mem_utils
}  // namespace jr

#endif  // JRIMAGE_MEMUTILS_H_
