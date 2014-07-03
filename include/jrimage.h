#ifndef JRIMAGE_JRIMAGE_H_
#define JRIMAGE_JRIMAGE_H_

#include <string>
#include <iostream>
#include <cstring>
#include <thread>
#include <algorithm>
#include <cassert>

// TODO(cbraley): Option for no SSE.
#include <xmmintrin.h>
#include <emmintrin.h>

#include "mem_utils.h"
#include "math_utils.h"

//#define SLOW_AND_STEADY

namespace jr {

/// Constant used to create an image with a dynamic channel count.
const constexpr int DYNAMIC_CHANNELS = -1;

// Byte alignment of start of image buffers.
const constexpr std::size_t BUF_BYTE_ALIGNMENT = 128;

// Byte alignment of each individual row.
const constexpr std::size_t ROW_BYTE_ALIGNMENT = 4;

static_assert(BUF_BYTE_ALIGNMENT % ROW_BYTE_ALIGNMENT == 0,
              "Buffer alignment must be a multiple of row alignment.");

template <typename T, int CHANNELS>
class Image;

template <typename T_LHS, int CHANNELS_LHS, typename T_RHS, int CHANNELS_RHS>
bool DimensionsMatch(const Image<T_LHS, CHANNELS_LHS>& lhs,
                     const Image<T_RHS, CHANNELS_RHS>& rhs);


/// Core templated image class.
template <typename T, int CHANNELS = DYNAMIC_CHANNELS>
class Image {
 public:
  // TODO(cbraley): Static assertion that the padding is compatible with sizeof(T).

  Image(int width, int height, int num_channels = 1);

  Image()
      : w_(-1),
        h_(-1),
        c_(-1),
        buf_(nullptr),
        owns_data_(true),
        row_stride_(0),
        contiguous_(true) {}

  ~Image() {
    if (owns_data_) {
      jr::mem_utils::AlignedDelete<T>(buf_);
    }
  }

  inline int Width() const { return w_; }
  inline int Height() const { return h_; }
  inline int Channels() const {
    return CHANNELS == DYNAMIC_CHANNELS ? c_ : CHANNELS;
  }
  constexpr bool IsChannelCountDynamic() const {
    return CHANNELS == DYNAMIC_CHANNELS;
  }

  inline int NumPixels() const { return Width() * Height(); }
  inline int NumValues() const { return NumPixels() * Channels(); }

  inline bool IsMemoryContiguous() const { return contiguous_; }

  void SetAll(const T& new_value) {
#ifndef SLOW_AND_STEADY
    for (int y = 0; y < Height(); ++y) {
      for (int x = 0; x < Width(); ++x) {
        for (int c = 0; c < Channels(); ++c) {
          Set(x, y, c, new_value);
        }
      }
    }
#else
    for (int y = 0; y < Height(); ++y) {
      T* row = GetRow(y);
      jr::mem_utils::SetMemory(row, new_value, Width());
    }
#endif
  }

  inline bool InBounds(int x, int y, int c) const {
    return InBounds(x, y) && c >= 0 && c < Channels();
  }
  // TODO(cbraley): See if Channels() is unrollable at compile time.

  inline bool InBounds(int x, int y) const {
    return x >= 0 && y >= 0 && x < Width() && y < Height();
  }

  inline int ClampX(int x) { return std::min(std::max<int>(0, x), Width() - 1); }
  inline int ClampY(int y) { return std::min(std::max<int>(0, y), Height() - 1); }


  inline T Get(int x, int y, int c) const { return *GetPointer(x, y, c); }

  inline T* GetPointer(int x, int y, int c) const {
    return buf_ + (row_stride_ * y) + (x * Channels()) + c;
  }

  void GetAllChannelsClamped(int x, int y, T* out) const {
    GetAllChannels(ClampX(x), ClampY(y), out);
  }

  void GetAllChannels(int x, int y, T* out) const {
#ifdef SLOW_AND_STEADY
    for (int c = 0; c < Channels(); ++c) {
      out[c] = Get(x, y, c);
    }
#else
    memcpy(static_cast<void*>(out),
           static_cast<const void*>(GetPointer(x, y, 0)),
           Channels() * sizeof(T));
#endif
  }

  void Set(int x, int y, int c, const T& val) {
    *(buf_ + (row_stride_ * y) + (x * Channels()) + c) = val;
  }

  void SetAllChannels(int x, int y, const T* values) {
#ifdef SLOW_AND_STEADY
    for (int c = 0; c < Channels(); ++c) {
      Set(x, y, c, values[c]);
    }
#else
    memcpy(static_cast<void*>(GetPointer(x, y, 0)),
           static_cast<const void*>(values),
           Channels() * sizeof(T));
#endif
  }

  // TODO(cbraley): Type conversion here!


  template <int DEST_CHANNELS>
  bool CopyInto(jr::Image<T, DEST_CHANNELS>& dest) const {
    // If the destination image does not have a dynamic channel count, we
    // must have the exact same number of channels.
    if ((!dest.IsChannelCountDynamic()) && Channels() != dest.Channels()) {
      return false;
    }

    // Resize the output buffer.
    dest.Allocate(Width(), Height(), Channels());
    assert(jr::DimensionsMatch(*this, dest));

    // Copy the data over.
#ifdef SLOW_AND_STEADY
    for (int y = 0; y < dest.Height(); ++y) {
      for (int x = 0; x < dest.Width(); ++x) {
        for (int c = 0; c < dest.Channels(); ++c) {
          dest.Set(x, y, c, Get(x, y, c));
        }
      }
    }
#else
    if (IsMemoryContiguous()) {
      memcpy(static_cast<void*>(dest.buf_),
             static_cast<const void*>(buf_),
             TotalByteCount());
    } else {
      const std::size_t row_bytes = RowByteCount();
      for (int y = 0; y < h_; ++y) {
        memcpy(static_cast<void*>(dest.GetRow(y)),
               static_cast<const void*>(GetRow(y)),
               row_bytes);
      }
    }
#endif

    return true;
  }

  template <typename RHS_T, int RHS_CHANNELS>
  bool operator==(const Image<RHS_T, RHS_CHANNELS>& rhs) const {
    // First, check cases that can allow us to avoid an expensive memcmp.

    // Images point to the same thing is definite equality.
    if (static_cast<const void*>(this) == static_cast<const void*>(&rhs)) {
      return true;
    }
    // Channel sizes or dimension mismatch means the images can't be the same.
    if (!jr::DimensionsMatch(*this, rhs)) {
      return false;
    }


#ifdef SLOW_AND_STEADY
    for (int y = 0; y < h_; ++y) {
      for (int x = 0; x < w_; ++x) {
        for (int c = 0; c < c_; ++c) {
          if (Get(x,y,c) != rhs.Get(x,y,c)) {
            return false;
          }
        }
      }
    }
    return true;
#else
    // At this point; we know we will have to do *some* memcmp.  However, if
    // both images are contiguous we can do this with one single large memcmp.
    if (IsMemoryContiguous() && rhs.IsMemoryContiguous()) {
      // Note that this call works because padding bytes are guaranteed to be
      // all 0's.
      return memcmp(static_cast<const void*>(buf_),
                    static_cast<const void*>(rhs.buf_),
                    TotalByteCount()) == 0;
    } else {
      const size_t row_bytes = RowByteCount();
      for (int y = 0; y < Height(); ++y) {
        if (memcmp(GetRow(y), rhs.GetRow(y), row_bytes) != 0) {
          return false;
        }
      }
      return true;
    }
#endif
  }

  template <typename RHS_T, int RHS_CHANNELS>
  bool operator!=(const Image<RHS_T, RHS_CHANNELS>& rhs) const {
    return !(*this == rhs);
  }

  inline void* GetRow(int y) { return buf_ + y * row_stride_; }
  inline const void* GetRow(int y) const { return buf_ + y * row_stride_; }

  void BilinearSample(float x, float y, T* out) {
    float xy_buf[4] = {x, y, 0.0f, 0.0f};
    float size_buf[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    size_buf[0] = static_cast<float>(Width());
    size_buf[1] = static_cast<float>(Height());

    __m128 pos  = _mm_loadu_ps(xy_buf);  // TODO(cbraley): This unaligned load is slow!
    __m128 dims = _mm_load_ps(size_buf);  // TODO(cbraley): This unaligned load is slow!

  /*
    pos = _mm_mul_ps(pos, dims);  // Multiply (x,y) .* (width, height).

    // Round up and round down.
    __m128 pos_rup   = _mm_ceil_ps(pos);
    __m128 pos_rdown = _mm_ceil_ps(pos);

    // Convert to integer types.
    __m128i upper_indices = _mm_castps_si128(pos_rup);
    __m128i lower_indices = _mm_castps_si128(pos_rdown);
    // TODO(cbraley): Clamp to array bounds.

    // Grab data from the array.
    float buf[10];  // TODO(cbraley): Dynamic channels!
    */
  }

  void Allocate(int new_w, int new_h, int new_c) {
    new_c = CHANNELS == DYNAMIC_CHANNELS ? new_c : CHANNELS;

    // Compute size needed for new data.
    const std::size_t row_data_bytes = new_w * new_c * sizeof(T);
    const std::size_t row_data_with_padding =
        jr::math_utils::UpToNearestMultiple(row_data_bytes, ROW_BYTE_ALIGNMENT);
    const std::size_t total_bytes = row_data_with_padding * new_h;
    assert(total_bytes % sizeof(T) == 0);

    const std::size_t new_size = total_bytes / sizeof(T);
    const std::size_t new_row_stride = row_data_with_padding / sizeof(T);

    // Allocate new memory.
    T* new_buf = jr::mem_utils::AlignedNew<T>(new_size, BUF_BYTE_ALIGNMENT);

    // We are *required* to memset all the newly allocated memory to 0.  This
    // is necessary since we memcmp buffers to compare images.  If we don't
    // memset things to 0 here; the padding bytes end up affecting the
    // comparison results.
    memset(static_cast<void*>(new_buf), 0, total_bytes);

    if (owns_data_) {
      jr::mem_utils::AlignedDelete<T>(buf_);
    }
    buf_ = new_buf;

    w_ = new_w;
    h_ = new_h;
    c_ = new_c;
    row_stride_ = new_row_stride;
    owns_data_ = true;
  }

 private:
  int w_, h_, c_;
  T* buf_;
  bool owns_data_;

  // Stride between rows in terms of T's.
  std::size_t row_stride_;

  // True if the image's memory is contiguous.
  bool contiguous_;

  // Exclusive of padding.
  inline std::size_t RowByteCount() const {
    return Width() * Channels() * sizeof(T);
  }

  // Inclusive of padding.
  inline std::size_t TotalByteCount() const {
    assert(IsMemoryContiguous());
    return row_stride_ * sizeof(T) * Height();
  }

  // No default construction or copying of jr::Image objects.
  Image(const Image& other) = delete;
  Image& operator=(const Image& other) = delete;

  inline int Index(int x, int y, int c) const {
    return Index(x, y, c, Width(), Height(), Channels());
  }

  static inline int Index(int x, int y, int c,
                          int width, int height, int chans) {
    assert(width > 0);
    assert(height > 0);
    assert(chans > 0);
    return y * (width * chans) + x * chans + c;
  }

  // We need to be friends with other template variants.
  template <typename T_FRIEND, int CHANNELS_FRIEND> friend class Image;
  // The stream insertion operator needs to be friends.
  template <typename T_FRIEND, int CHANNELS_FRIEND>
  friend std::ostream& operator<<(
      std::ostream& os, const Image<T_FRIEND, CHANNELS_FRIEND>& image);
};

template <typename T, int CHANNELS>
std::ostream& operator<<(std::ostream& os, const Image<T, CHANNELS>& image) {
  os << "Image with width=" << image.Width() << ", height=" << image.Height()
     << " and " << image.Channels()
     << (image.IsChannelCountDynamic() ? " dynamic " : " ") << "channels."
     << " row stride = " << image.row_stride_
     << std::endl
     << "Pixel buffer: {" << std::endl;

  for (int y = 0; y < image.Height(); ++y) {
    os << "row " << y << ": ";
    for (int x = 0; x < image.Width(); ++x) {
      os << "[";
      for (int c = 0; c < image.Channels(); ++c) {
        os << image.Get(x, y, c) << (c != image.Channels() - 1 ? ", " : "");
      }
      os << "] ";
    }
    os << std::endl;
  }
  os << "}.  End of pixel bufer." << std::endl;
  return os;
}

template <typename T_LHS, int CHANNELS_LHS, typename T_RHS, int CHANNELS_RHS>
bool DimensionsMatch(const Image<T_LHS, CHANNELS_LHS>& lhs,
                     const Image<T_RHS, CHANNELS_RHS>& rhs) {
  return lhs.Width() == rhs.Width() &&
         lhs.Height() == rhs.Height() &&
         lhs.Channels() == rhs.Channels();
}


// Inline member function definitions. ----------------------------------------
template <typename T, int CHANNELS>
Image<T, CHANNELS>::Image(int width, int height, int num_channels)
    : w_(width),
      h_(height),
      c_(CHANNELS == DYNAMIC_CHANNELS ? num_channels : CHANNELS),
      buf_(nullptr),
      owns_data_(true),
      row_stride_(0),
      contiguous_(true) {
  Allocate(w_, h_, c_);
}

}  // namespace jr

#endif  // JRIMAGE_JRIMAGE_H_
