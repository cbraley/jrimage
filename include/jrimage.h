#ifndef JRIMAGE_JRIMAGE_H_
#define JRIMAGE_JRIMAGE_H_

#include <string>
#include <iostream>
#include <cstring>
#include <thread>
#include <algorithm>
#include <cassert>
#include <memory>

// TODO(cbraley): Option for no SSE.
#include <xmmintrin.h>
#include <emmintrin.h>

#include "mem_utils.h"
#include "math_utils.h"

//#define SLOW_AND_STEADY
namespace jr {

// Forward declarations of all classes.
template<typename ImageImplT> class ImageBase;  // CRTP base class.
template<typename T, int NumChannels, typename Allocator> class ImageBuf;  // Raw 2D buffer without color info.
template<typename T, typename ColorSpace, typename Allocator> class Image;  // Image class with color info.

/// Constant used to create an image with a dynamic channel count.
const constexpr int DYNAMIC_CHANNELS = -1;


/// CRTP base class for image classes.
/// The template ImageImplT should be an Image class that meets the following requirements:
///   The following typedefs are provided:
///     PixelT
///   The following functions are provided:
///     int Channels() const
///   TODO(cbraley): Complete this doc.
template<typename ImageImplT>
class ImageBase {
 public:
  inline int Width() const { return Impl().Width(); }
  inline int Height() const { return Impl().Height(); }
  inline int Channels() const { return Impl().Channels(); }

  // Testing if X and Y coordinates are in bounds.
  inline bool InBounds(int x, int y, int c) const {
    return InBounds(x, y) && c >= 0 && c < Channels();
  }
  inline bool InBounds(int x, int y) const {
    return x >= 0 && y >= 0 && x < Width() && y < Height();
  }

  // Clamping X and Y coordinates to be within bounds.
  inline int ClampX(int x) const { return std::min(std::max(0, x), Width() - 1); }
  inline int ClampY(int y) const { return std::min(std::max(0, y), Height() - 1); }

 private:
  // The helper functions Impl(...) cast the "this" pointer to an instance
  // of the ImageImplT type (the concrete image implementation).
  inline const ImageImplT& Impl() const {
    return *static_cast<const ImageImplT*>(this);
  }
  inline ImageImplT& Impl() {
    return *static_cast<ImageImplT*>(this);
  }

  // No copy construction or assignment allowed.
  ImageBase(const ImageBase&) = delete;
  ImageBase& operator=(const ImageBase&) = delete;

 protected:
  // No default construction (or construction of any kind) is allowed, since
  // this class is the base of a CRTP hierarchy.
  ImageBase() {}
  ~ImageBase() {}
};

// Return true if the dimensions of two images match.
template<typename ImageImplTA, typename ImageImplTB>
bool DimensionsMatch(const ImageBase<ImageImplTA>& a, const ImageBase<ImageImplTB>& b);


/// Core templated image class.
template <typename T,  // Primitive type stored in the array.
          int NumChannels = DYNAMIC_CHANNELS,  // Channel count, or DYNAMIC_CHANNELS.
          typename Allocator = std::allocator<T>>  // Allocator used for memory allocation.
class ImageBuf : public ImageBase<ImageBuf<T, NumChannels, Allocator>> {
 public:
  // Static assertions that the template arguments are reasonable.
  static_assert(std::is_trivial<T>::value,
                "ImageBuf template type T must be trivially copyable "
                "and trivially constructable.");
  static_assert(std::is_same<T, typename Allocator::value_type>::value,
                "ImageBuf allocator template argument needs to be an allocator "
                "for the ImageBuf's pixel type T.");
  static_assert(
      NumChannels == DYNAMIC_CHANNELS || NumChannels > 0,
      "NumChannels must either be a positive integer, or be the special "
      "DYNAMIC value.");

  // Typedefs.

  // ChannelCountKnownAtCompileTime is true_type if we know the channel count
  // at compile time, or false_type otherwise.
  typedef typename std::conditional<NumChannels != DYNAMIC_CHANNELS,
                                    std::true_type, std::false_type>::type
                                    ChannelCountKnownAtCompileTime;
  // Constructors.

  // Construct an image with a dynamic number of channels.
  // This constructor can only be called if NumChannels == DYNAMIC_CHANNELS.
  ImageBuf(int width, int height, int num_channels);

  // Construct an ImageBuf with a static number of channels.
  // This constructor can only be called if NumChannels != DYNAMIC_CHANNELS.
  ImageBuf(int width, int height);

  // Construct an empty ImageBuf.
  ImageBuf();

  // Destructor.
  ~ImageBuf();

  // Implementation of the interface required by the CRTP base class ImageBase.
  inline int Width() const { return w_; }
  inline int Height() const { return h_; }
  inline int Channels() const {
    return NumChannels == DYNAMIC_CHANNELS ? c_ : NumChannels;
  }
  constexpr bool IsChannelCountDynamic() const {
    return NumChannels == DYNAMIC_CHANNELS;
  }
  constexpr bool IsChannelCountStatic() const {
    return !IsChannelCountDynamic();
  }

  inline bool IsMemoryContiguous() const { return contiguous_; }

  void SetAll(const T& new_value) {
    if (IsMemoryContiguous()) {
      jr::mem_utils::SetMemory(buf_, new_value, data_numel_);
    } else {
      for (int y = 0; y < Height(); ++y) {
        T* row = GetRow(y);
        jr::mem_utils::SetMemory(row, new_value, Width());
      }
    }
  }

  inline T Get(int x, int y, int c) const { return *GetPointer(x, y, c); }

  inline T* GetPointer(int x, int y, int c) const {
    return buf_ + (row_stride_ * y) + (x * Channels()) + c;
  }

  void GetAllChannelsClamped(int x, int y, T* out) const {
    GetAllChannels(this->ClampX(x), this->ClampY(y), out);
  }


  // Function definitions.

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
    *(buf_ + (row_stride_* y) + (x* Channels()) + c) = val;
  }

  void SetAllChannels(int x, int y, const T* values) {
#ifdef SLOW_AND_STEADY
    for (int c = 0; c < Channels(); ++c) {
      Set(x, y, c, values[c]);
    }
#else
    memcpy(static_cast<void*>(GetPointer(x, y, 0)),
           static_cast<const void*>(values), Channels() * sizeof(T));
#endif
  }

  // TODO(cbraley): Type conversion here!

  template <int DestNumChannels, class DestAllocator>
  bool CopyInto(jr::ImageBuf<T, DestNumChannels, DestAllocator>& dest) const {
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
      memcpy(static_cast<void*>(dest.buf_), static_cast<const void*>(buf_),
             TotalByteCount());
    } else {
      const std::size_t row_bytes = RowByteCount();
      for (int y = 0; y < h_; ++y) {
        memcpy(static_cast<void*>(dest.GetRow(y)),
               static_cast<const void*>(GetRow(y)), row_bytes);
      }
    }
#endif

    return true;
  }

  template <typename RHS_T, int RhsChannels, typename Allocator_RHS>
  bool operator==(const ImageBuf<RHS_T, RhsChannels, Allocator_RHS>& rhs) const {
    // First, check cases that can allow us to avoid an expensive memcmp.

    // ImageBufs point to the same thing is definite equality.
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
          if (Get(x, y, c) != rhs.Get(x, y, c)) {
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
                    static_cast<const void*>(rhs.buf_), TotalByteCount()) == 0;
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

  template <typename RHS_T, int RhsChannels, typename Allocator_RHS>
  bool operator!=(const ImageBuf<RHS_T, RhsChannels, Allocator_RHS>& rhs) const {
    return !(*this == rhs);
  }

  inline T* GetRow(int y) { return buf_ + y * row_stride_; }
  inline const T* GetRow(int y) const { return buf_ + y * row_stride_; }

  void BilinearSample(float x, float y, T* out) {
    float xy_buf[4] = {x, y, 0.0f, 0.0f};
    float size_buf[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    size_buf[0] = static_cast<float>(Width());
    size_buf[1] = static_cast<float>(Height());

    __m128 pos =
        _mm_loadu_ps(xy_buf);  // TODO(cbraley): This unaligned load is slow!
    __m128 dims =
        _mm_load_ps(size_buf);  // TODO(cbraley): This unaligned load is slow!

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
    assert(new_w >= 0);
    assert(new_h >= 0);
    AssertInvariants();
    if (IsChannelCountStatic()) {
      assert(new_c == NumChannels);
      new_c = NumChannels;
    } else {
      assert(IsChannelCountDynamic());
      assert(new_c > 0);
    }

    // Store size of old data.
    const std::size_t old_data_numel = data_numel_ ;

    // Compute size needed for new data.
    const std::size_t row_data_bytes = new_w * new_c * sizeof(T);
    const std::size_t total_bytes = row_data_bytes * new_h;
    assert(total_bytes % sizeof(T) == 0);

    data_numel_= new_w * new_h * new_c;
    assert(data_numel_ == total_bytes / sizeof(T));
    const std::size_t new_row_stride = new_w * new_c;

    // Allocate new memory.
    T* new_buf = allocator_.allocate(data_numel_);

    // We are *required* to memset all the newly allocated memory to 0.  This
    // is necessary since we memcmp buffers to compare images.  If we don't
    // memset things to 0 here; the padding bytes end up affecting the
    // comparison results.
    memset(static_cast<void*>(new_buf), 0, total_bytes);

    if (owns_data_) {
      allocator_.deallocate(buf_, old_data_numel);
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
  std::size_t data_numel_;  // Width() * Height() * Channels()
  Allocator allocator_;

  // Stride between rows in terms of T's.
  std::size_t row_stride_;

  // True if the image's memory is contiguous.
  bool contiguous_;

  inline void AssertInvariants() const {
    if (Width() >= 0 && Height() >= 0 && Channels() >= 0) {
      assert(data_numel_ == Width() * Height() * Channels());
    } else {
      assert(buf_ == nullptr);
    }
  }

  // Exclusive of padding.
  inline std::size_t RowByteCount() const {
    return Width() * Channels() * sizeof(T);
  }

  // Inclusive of padding.
  inline std::size_t TotalByteCount() const {
    assert(IsMemoryContiguous());
    return row_stride_ * sizeof(T) * Height();
  }

  // No default construction or copying of jr::ImageBuf objects.
  ImageBuf(const ImageBuf& other) = delete;
  ImageBuf& operator=(const ImageBuf& other) = delete;

  inline int Index(int x, int y, int c) const {
    return Index(x, y, c, Width(), Height(), Channels());
  }

  static inline int Index(int x, int y, int c, int width, int height,
                          int chans) {
    assert(width > 0);
    assert(height > 0);
    assert(chans > 0);
    return y * (width * chans) + x * chans + c;
  }

  // We need to be friends with other template variants.
  template <typename T_FRIEND, int ChannelsFriend, typename AllocatorFriend>
  friend class ImageBuf;
  // The stream insertion operator needs to be friends.
  template <typename T_FRIEND, int ChannelsFriend, typename AllocatorFriend>
  friend std::ostream& operator<<(
      std::ostream& os, const ImageBuf<T_FRIEND, ChannelsFriend, AllocatorFriend>& image);
};

template <typename T, int NumChannels, typename Allocator>
std::ostream& operator<<(std::ostream& os, const ImageBuf<T, NumChannels, Allocator>& image) {
  os << "ImageBuf with width=" << image.Width() << ", height=" << image.Height()
     << " and " << image.Channels()
     << (image.IsChannelCountDynamic() ? " dynamic " : " ") << "channels."
     << " row stride = " << image.row_stride_ << std::endl << "Pixel buffer: {"
     << std::endl;

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


template<typename ImageImplTA, typename ImageImplTB>
bool DimensionsMatch(const ImageBase<ImageImplTA>& a,
                     const ImageBase<ImageImplTB>& b) {
  return a.Width() == b.Width() && a.Height() == b.Height() &&
         a.Channels() == b.Channels();
}

// Inline member function definitions. ----------------------------------------

template <typename T, int NumChannels, typename Allocator>
ImageBuf<T, NumChannels, Allocator>::ImageBuf()
    : w_(-1),
      h_(-1),
      c_(-1),
      buf_(nullptr),
      owns_data_(true),
      data_numel_(0),
      row_stride_(0),
      contiguous_(true) {}

template <typename T, int NumChannels, typename Allocator>
ImageBuf<T, NumChannels, Allocator>::~ImageBuf() {
  if (owns_data_) {
    allocator_.deallocate(buf_, data_numel_);
  }
}

template <typename T, int NumChannels, typename Allocator>
ImageBuf<T, NumChannels, Allocator>::ImageBuf(int width, int height,
                                              int num_channels)
    : w_(width),
      h_(height),
      c_(num_channels),
      buf_(nullptr),
      owns_data_(true),
      data_numel_(width * height * num_channels),
      row_stride_(0),
      contiguous_(true) {
  static_assert(NumChannels == DYNAMIC_CHANNELS,
                "The ImageBuf(width, height, num_channels) constructor can "
                "only be called when the ImageBuf has a \"dynamic\" number "
                "of channels.  ImageBufs with a dynamic number of "
                "channels have their channel count chosen at runtime ("
                "as opposed to at compile time).");
  Allocate(w_, h_, c_);
}

template <typename T, int NumChannels, typename Allocator>
ImageBuf<T, NumChannels, Allocator>::ImageBuf(int width, int height)
    : w_(width),
      h_(height),
      c_(NumChannels),
      buf_(nullptr),
      owns_data_(true),
      data_numel_(width * height * NumChannels),
      row_stride_(0),
      contiguous_(true) {
  static_assert(NumChannels != DYNAMIC_CHANNELS,
                "The ImageBuf(width, height) constructor can "
                "only be called when the ImageBuf has a \"static\" number "
                "of channels.  ImageBufs with a static number of "
                "channels have their channel count chosen at compile time ("
                "as opposed to at runtime).");
  static_assert(NumChannels > 0, "Invalid channel count.");
  Allocate(w_, h_, NumChannels);
}


}  // namespace jr

#endif  // JRIMAGE_JRIMAGE_H_
