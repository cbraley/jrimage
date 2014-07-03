#include "mem_utils.h"

namespace jr {
namespace mem_utils {
namespace implementation_details {

template<>
void SetMem<1>(uint8_t* ptr,
               const uint8_t* const value_to_set,
               std::size_t num) {
  memset(static_cast<void*>(ptr),
         *(reinterpret_cast<const int*>(value_to_set)),
         num);
}

}  // namespace implementation_details

void* MemFill(void* buffer, std::size_t buffer_size_bytes,
              const void* pattern, std::size_t pattern_size_bytes) {
  assert(buffer != nullptr);
  assert(pattern != nullptr);

  if (pattern_size_bytes == 1) {
    return memset(buffer,
                  *reinterpret_cast<const unsigned char*>(pattern),
                  buffer_size_bytes);
  } else if (pattern_size_bytes == 0 || buffer_size_bytes == 0) {
    return buffer;  // This is required to prevent division by 0.
  } else {
    // TODO(cbraley): For small parameter sizes; MemFillSimple is faster.
    return MemFillChunks(buffer, buffer_size_bytes, pattern, pattern_size_bytes);
  }
}

void* MemFillSimple(void* buffer, std::size_t buffer_size_bytes,
                    const void* pattern, std::size_t pattern_size_bytes) {
  assert(buffer != nullptr);
  assert(pattern != nullptr);

  if (pattern_size_bytes == 0 || buffer_size_bytes == 0) {
    // This is required to prevent division by 0.
    return buffer;
  }

  // Fill buffer in chunks of pattern_size_bytes at a time.
  uint8_t* buf = static_cast<uint8_t*>(buffer);
  for (std::size_t i = 0; i < buffer_size_bytes / pattern_size_bytes; ++i) {
    memcpy(static_cast<void*>(buf + i * pattern_size_bytes),
           pattern, pattern_size_bytes);
  }

  // Fill the remaining byte left over (if any).
  const std::size_t remaining_bytes = buffer_size_bytes % pattern_size_bytes;
  if (remaining_bytes != 0) {
    memcpy(static_cast<void*>(buf + (buffer_size_bytes - remaining_bytes)),
           pattern,
           remaining_bytes);
  }
  return buffer;
}

void* MemFillChunks(void* buffer, std::size_t buffer_size_bytes,
                    const void* pattern, std::size_t pattern_size_bytes) {
  assert(buffer != nullptr);
  assert(pattern != nullptr);

  if (pattern_size_bytes == 0 || buffer_size_bytes == 0) {
    // This is required to prevent division by 0.
    return buffer;
  }

  // Handle special case with patterns larger than buffer.
  // TODO(cbraley): This could be handled more elegantly and maybe faster if
  // we did:
  // pattern_size_bytes = std::min(pattern_size_bytes, buffer_size_bytes)
  if (pattern_size_bytes >= buffer_size_bytes) {
    memcpy(buffer, pattern, buffer_size_bytes);
    return buffer;
  }

  // Copy the first pattern_size_bytes.
  memcpy(static_cast<void*>(buffer),
         static_cast<const void*>(pattern),
         pattern_size_bytes);

  uint8_t* out = static_cast<uint8_t*>(buffer) + pattern_size_bytes;
  std::size_t copy_from_buf_size = pattern_size_bytes;

  while (out + copy_from_buf_size <= static_cast<uint8_t*>(buffer) + buffer_size_bytes) {
    memcpy(static_cast<void*>(out),
           static_cast<const void*>(buffer),
           copy_from_buf_size);
    out += copy_from_buf_size;
    copy_from_buf_size *= 2;  // TODO(cbraley): pow to remove dependency?
  }

  // Copy over the slack.
  if (buffer_size_bytes > copy_from_buf_size) {
    memcpy(static_cast<void*>(out),
           static_cast<const void*>(buffer),
           buffer_size_bytes - copy_from_buf_size);
  }

  return buffer;
}

}  // namespace mem_utils
}  // namespace jr

