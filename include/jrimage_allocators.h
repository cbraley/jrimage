#ifndef JRIMAGE_ALLOCATORS_H_
#define JRIMAGE_ALLOCATORS_H_

#include <memory>
#include <cassert>
#include <type_traits>

#include "mem_utils.h"

// Custom memory allocators for jrimage.

namespace jr {

// This allocator class can align memory to a "stronger" alignment requirement
// than alignof(Tp).
template <typename Tp, std::size_t AlignBytes = alignof(Tp)>
struct AlignedAllocator {
 public:
  // Public interface required by all C++ allocators.
  typedef Tp value_type;
  AlignedAllocator();
  template <typename TpOther, std::size_t AlignBytesOther>
  AlignedAllocator(const AlignedAllocator<TpOther, AlignBytesOther>& other);
  Tp* allocate(std::size_t n);
  void deallocate(Tp* p, std::size_t n);

  // Details specefic to AlignedAllocator.
  static_assert(AlignBytes % alignof(Tp) == 0,
                "AlignedAllocator alignment must be a multiple of the type "
                "Tp's native alignment.");
};

template <typename T, std::size_t AlignBytesLHS,
          typename U, std::size_t AlignBytesRHS>
bool operator==(const AlignedAllocator<T, AlignBytesLHS>& lhs,
                const AlignedAllocator<U, AlignBytesRHS>& rhs);
template <typename T, std::size_t AlignBytesLHS,
          typename U, std::size_t AlignBytesRHS>
bool operator!=(const AlignedAllocator<T, AlignBytesLHS>& lhs,
                const AlignedAllocator<U, AlignBytesRHS>& rhs);

// Aligned allocator implementation. ------------------------------------------

template <typename Tp, std::size_t AlignBytes>
AlignedAllocator<Tp, AlignBytes>::AlignedAllocator() {}

template <typename Tp, std::size_t AlignBytes>
template <typename TpOther, std::size_t AlignBytesOther>
AlignedAllocator<Tp, AlignBytes>::AlignedAllocator(
    const AlignedAllocator<TpOther, AlignBytesOther>& other)
    : AlignedAllocator<Tp, AlignBytes>() {}

template <typename Tp, std::size_t AlignBytes>
Tp* AlignedAllocator<Tp, AlignBytes>::allocate(std::size_t n) {
  return jr::mem_utils::AlignedNew<Tp>(n, AlignBytes);
}

template <typename Tp, std::size_t AlignBytes>
void AlignedAllocator<Tp, AlignBytes>::deallocate(Tp* p, std::size_t n) {
  assert(jr::mem_utils::IsPointerAligned(p, AlignBytes));
  jr::mem_utils::AlignedDelete(p);
}

template <typename T, std::size_t AlignBytesLHS, typename U, std::size_t AlignBytesRHS>
bool operator==(const AlignedAllocator<T, AlignBytesLHS>& lhs,
                const AlignedAllocator<U, AlignBytesRHS>& rhs) {
  return (&lhs == &rhs) ||
         (std::is_same<T, U>::value && AlignBytesLHS == AlignBytesRHS);
}

template <typename T, std::size_t AlignBytesLHS, typename U, std::size_t AlignBytesRHS>
bool operator!=(const AlignedAllocator<T, AlignBytesLHS>& lhs,
                const AlignedAllocator<U, AlignBytesRHS>& rhs) {
  return !(lhs == rhs);
}

}  // namespace jr

#endif  // JRIMAGE_ALLOCATORS_H_
