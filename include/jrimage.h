#ifndef JRIMAGE_JRIMAGE_H_
#define JRIMAGE_JRIMAGE_H_

#include <string>
#include <iostream>
#include <cstring>
#include <thread>
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>

#include "mem_utils.h"
#include "math_utils.h"
#include "template_utils.h"

// TODO(cbraley): Make all the pointer methods return void* instead of uchar*.

namespace jr {

// Forward declarations of all classes.
template<typename ImageImplT> class ImageBase;  // CRTP base class.
template<typename T, int NumChannels, typename Allocator> class ImageBuf;  // Raw 2D buffer without color info.
template<typename T, typename ColorSpace, typename Allocator> class Image;  // Image class with color info.

/// Constant used to create an image with a dynamic channel count.
const constexpr int DYNAMIC_CHANNELS = -1;

// Traits class.  Each CTRP leaf node class must specialize ImageTraits.
template<typename ImageT> struct ImageTraits;

// Return true if the dimensions of two images match.
template<typename ImageImplTA, typename ImageImplTB>
bool DimensionsMatch(const ImageBase<ImageImplTA>& a,
                     const ImageBase<ImageImplTB>& b);

// Deep comparison of images, pixel-for-pixel.
template<typename ImageImplLhsT, typename ImageImplRhsT>
bool operator!=(const ImageBase<ImageImplLhsT> &lhs,
                const ImageBase<ImageImplRhsT> &rhs);
template<typename ImageImplLhsT, typename ImageImplRhsT>
bool operator==(const ImageBase<ImageImplLhsT> &lhs,
                const ImageBase<ImageImplRhsT> &rhs);

// Print information about an image to an ostream, and print it's full raster if
// it is small.
template<typename ImageImplT>
std::ostream& operator<<(std::ostream& os, const ImageBase<ImageImplT>& image);


/// CRTP base class for image classes.
/// The template ImageImplT should be an Image class that meets the following
/// requirements:
///   The following typedefs are provided:
///     TODO(cbraley): This is all now via ImageTraits.
///     ChannelT
///     ChannelCountKnownAtCompileTime
///   The following functions are provided:
///     int Width() const;
///     int Height() const;
///     int Channels() const
///     bool IsMemoryContiguous() const;
///     std::size_t PixelSizeBytes() const
///     std::size_t TotalByteCount() const
///
///     uint8_t* GetRow(int y)
///     const uint8_t* GetRow(int y) const
///     uint8_t* GetPointer(int x, int y, int c) const
///
///     bool Resize(int new_w, int new_h, int new_c);
///
///   TODO(cbraley): Complete this doc.
template<typename ImageImplT>
class ImageBase {
 public:
  // Fundamental type used to represent a single channel value.
  typedef typename ImageTraits<ImageImplT>::ChannelT ChannelT;

  /// Get the width of the image in pixels.
  inline int Width() const { return Impl().Width(); }
  /// Get the height of the image in pixels.
  inline int Height() const { return Impl().Height(); }
  /// Get the number of channels in the image.
  inline int Channels() const { return Impl().Channels(); }

  // Number of pixels in the image.
  inline int NumPixels() const { return Width() * Height(); }

  // Size of each row in bytes (not including padding).
  inline std::size_t RowSizeBytes() const { return Width() * PixelSizeBytes(); }

  // Static constexpr helper function that returns true if the channel count is
  // known at compile time.
  static constexpr bool IsChannelCountStatic() {
    return ImageTraits<ImageImplT>::ChannelCountKnownAtCompileTime::value;
  }
  // Static constexpr helper function that returns true if the channel count is
  // NOT known at compile time.
  static constexpr bool IsChannelCountDynamic() {
    return !IsChannelCountStatic();
  }

  inline bool IsMemoryContiguous() const { return Impl().IsMemoryContiguous(); }
  inline std::size_t PixelSizeBytes() const { return Impl().PixelSizeBytes(); }
  inline std::size_t TotalByteCount() const { return Impl().TotalByteCount(); }
  inline ChannelT* GetRow(int y) { return Impl().GetRow(y); }
  inline const ChannelT* GetRow(int y) const { return Impl().GetRow(y); }
  inline ChannelT* GetPointer(int x, int y, int c) const { return Impl().GetPointer(x, y, c); }
  inline ChannelT Get(int x, int y, int c) const { return Impl().Get(x, y, c); }
  inline bool Resize(int new_w, int new_h, int new_c) { return Impl().Resize(new_w, new_h, new_c); }



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

  // Total number of measurements.
  inline int Numel() const { return Width() * Height() * Channels(); }

  void SetAll(const ChannelT& new_value) {
    if (IsMemoryContiguous()) {
      jr::mem_utils::SetMemory(GetRow(0), new_value, Numel());
    } else {
      for (int y = 0; y < Height(); ++y) {
        ChannelT* row = GetRow(y);
        jr::mem_utils::SetMemory(row, new_value, Width());
      }
    }
  }

  void GetAllChannels(int x, int y, ChannelT* out) const {
    memcpy(static_cast<void*>(out),
           static_cast<const void*>(GetPointer(x, y, 0)),
           PixelSizeBytes());
  }

  void Set(int x, int y, int c, const ChannelT& val) {
    *GetPointer(x, y, c) = val;
  }

  void SetAllChannels(int x, int y, const ChannelT* values) {
    memcpy(static_cast<void*>(GetPointer(x, y, 0)),
           static_cast<const void*>(values),
           PixelSizeBytes());
  }

  bool GetWindow(int x, int y, int width, int height,
                 ImageImplT& result_window) const {
    // Ensure that the window location is valid.
    const bool window_loc_okay =
        InBounds(x, y) && InBounds(x + width - 1, y + height - 1);
    // If the location is valid, make the implementation class return a
    // window into the image.
    if (window_loc_okay) {
      return Impl().GetWin(x, y, width, height, result_window);
    } else {
      return false;
    }
  }

  // More complex functions.

  template <class ImageImplOtherT>
  bool CopyInto(jr::ImageBase<ImageImplOtherT> & dest) const {
    // If the destination image does not have a dynamic channel count, we
    // must have the exact same number of channels.
    if ((!dest.IsChannelCountDynamic()) && Channels() != dest.Channels()) {
      return false;
    }
    assert(dest.Resize(Width(), Height(), Channels()));
    assert(jr::DimensionsMatch(*this, dest));

    // TODO(cbraley): Remove this limitation and allow casting and conversions....
    // maybe do this via  a separate function with a diff name?
    static_assert(
        std::is_same<typename ImageTraits<ImageImplT>::ChannelT,
                     typename ImageTraits<ImageImplOtherT>::ChannelT>::value,
        "Channel types must match!");

    // Copy the data over.
    if (IsMemoryContiguous()) {
      memcpy(static_cast<void*>(dest.GetRow(0)),
             static_cast<const void*>(GetRow(0)),
             TotalByteCount());
    } else {
      const std::size_t row_bytes = Width() * PixelSizeBytes();
      for (int y = 0; y < Height(); ++y) {
        memcpy(static_cast<void*>(dest.GetRow(y)),
               static_cast<const void*>(GetRow(y)),
               row_bytes);
      }
    }
    return true;
  }

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
  // No construction of any kind is allowed, unless calling from the initializer
  // list of a derived class, since this class is the base of a CRTP hierarchy.
  ImageBase() {}
  ~ImageBase() {}
};


template<typename T, int NumChannels, typename Allocator>
struct ImageTraits<ImageBuf<T, NumChannels, Allocator>> {
  // Primitive type used for a single channel.
  typedef T ChannelT;

  // ChannelCountKnownAtCompileTime is true_type if we know the channel count
  // at compile time, or false_type otherwise.
  typedef typename std::conditional<NumChannels != DYNAMIC_CHANNELS,
                                    std::true_type, std::false_type>::type
                                    ChannelCountKnownAtCompileTime;
};

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
    return SelfT::IsChannelCountDynamic() ? c_ : NumChannels;
  }
  inline bool IsMemoryContiguous() const { return Width() == row_stride_; }

  inline T* GetRow(int y) { return buf_ + y * row_stride_; }
  inline const T* GetRow(int y) const { return buf_ + y * row_stride_; }
  inline T Get(int x, int y, int c) const { return *GetPointer(x, y, c); }
  inline T* GetPointer(int x, int y, int c) const { return buf_ + (row_stride_ * y) + (x * Channels()) + c; }
  inline std::size_t PixelSizeBytes() const { return sizeof(T) * Channels(); }

  // Inclusive of padding.
  inline std::size_t TotalByteCount() const {
    return row_stride_ * sizeof(T) * Height();
  }

  // TODO(cbraley): Diff allocators and channel counts!
  // TODO(cbraley): Rename? and make private.
  inline bool GetWin(int x, int y, int width, int height,
                     ImageBuf<T, NumChannels, Allocator>& window) const {
    // First, free any memory the image may own.  This invalidates all
    // windows into that data.
    window.FreeMemIfOwned();

    window.w_ = width;
    window.h_ = height;
    window.c_ = c_;
    window.buf_ = GetPointer(x, y, 0);
    window.owns_data_ = false;
    window.allocator_ = allocator_;
    window.row_stride_ = row_stride_ + ((Width() - width) * Channels());
    return true;
  }

  bool Resize(int new_w, int new_h, int new_c) {
    if (new_w < 0 || new_h < 0 || new_c < 0) {
      return false;
    } else if (!SelfT::IsChannelCountDynamic() && new_c != Channels()) {
      return false;
    } else if (!owns_data_) {
      return false;
    } else {
      AllocateHelper(new_w, new_h, new_c);
      return true;
    }
  }

  // TODO(cbraley): Move Allocate functions into base class!

  void Allocate(int new_w, int new_h) {
    return AllocateHelper(new_w, new_h, Channels());
  }

  void Allocate(int new_w, int new_h, int new_c) {
    static_assert(SelfT::IsChannelCountDynamic(),
                  "The 3 argument form of Allocate, "
                  "Allocate(width, height, channels), "
                  "can only be called using images that have a dynamic "
                  "channel count.  This image has a static channel count.  "
                  "Maybe you want to call Allocate(width, height) instead?");
    return AllocateHelper(new_w, new_h, new_c);
  }

 private:
  typedef ImageBuf<T, NumChannels, Allocator> SelfT;
  int w_, h_, c_;
  T* buf_;
  bool owns_data_;
  Allocator allocator_;

  // Stride between rows in terms of T's.
  std::size_t row_stride_;

  void FreeMemIfOwned();

  void AllocateHelper(int new_w, int new_h, int new_c) {
    assert(new_w >= 0);
    assert(new_h >= 0);
    AssertInvariants();
    if (ImageTraits<SelfT>::ChannelCountKnownAtCompileTime::value) {
      assert(new_c == Channels());
      new_c = Channels();
    } else {
      assert(!ImageTraits<SelfT>::ChannelCountKnownAtCompileTime::value);
      assert(new_c > 0);
    }

    // Store size of old data.
    const std::size_t old_data_numel = Width() * Height() * Channels();

    // Compute size needed for new data.
    const std::size_t row_data_bytes = new_w * new_c * sizeof(T);
    const std::size_t total_bytes = row_data_bytes * new_h;
    assert(total_bytes % sizeof(T) == 0);

    const std::size_t data_numel = new_w * new_h * new_c;
    assert(data_numel == total_bytes / sizeof(T));
    const std::size_t new_row_stride = new_w * new_c;

    // Allocate new memory.
    T* new_buf = allocator_.allocate(data_numel);

    // We are *required* to memset all the newly allocated memory to 0.  This
    // is necessary since we memcmp buffers to compare images.  If we don't
    // memset things to 0 here; the padding bytes end up affecting the
    // comparison results.
    //memset(static_cast<void*>(new_buf), 0, total_bytes);

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

  inline void AssertInvariants() const {
    if (buf_ != nullptr) {
      assert(Width() > 0);
      assert(Height() > 0);
      assert(Channels() > 0);
    }
  }

  // No default construction or copying of jr::ImageBuf objects.
  ImageBuf(const ImageBuf& other) = delete;
  ImageBuf& operator=(const ImageBuf& other) = delete;

  // We need to be friends with other template variants.
  template <typename T_FRIEND, int ChannelsFriend, typename AllocatorFriend>
  friend class ImageBuf;
};


template<typename ImageImplT>
std::ostream& operator<<(std::ostream& os, const ImageBase<ImageImplT>& image) {
  os << "ImageBuf with width=" << image.Width() << ", height=" << image.Height()
     << " and " << image.Channels() << " "
     << (ImageBase<ImageImplT>::IsChannelCountDynamic() ? "dynamic" : "static") << " channels.";
  // TODO(cbraley): Print debug info here about the ImageImplT type.

  // If there are too many pixels to reasonably look at, don't flood
  // the console.
  static const int MAX_PIXELS_TO_PRINT = 50;
  if (image.NumPixels() > MAX_PIXELS_TO_PRINT) {
    os << "(pixel buffer too large to print)" << std::endl;
    return os;
  }
  os << std::endl << "Pixel buffer: {" << std::endl;
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


template<typename ImageImplLhsT, typename ImageImplRhsT>
bool operator!=(const ImageBase<ImageImplLhsT> &lhs,
                const ImageBase<ImageImplRhsT> &rhs) {
  return !(lhs == rhs);
}


template<typename ImageImplLhsT, typename ImageImplRhsT>
bool operator==(const ImageBase<ImageImplLhsT> &lhs,
                const ImageBase<ImageImplRhsT> &rhs) {
  // First, check cases that can allow us to avoid an expensive memcmp.

  // ImageBufs point to the same thing is definite equality.
  if (static_cast<const void*>(&lhs) == static_cast<const void*>(&rhs)) {
    return true;
  }
  // Channel sizes or dimension mismatch means the images can't be the same.
  if (!jr::DimensionsMatch(lhs, rhs)) {
    return false;
  }
  if (lhs.PixelSizeBytes() != rhs.PixelSizeBytes()) {
    return false;
  }
  if (!std::is_same<typename ImageTraits<ImageImplLhsT>::ChannelT,
                    typename ImageTraits<ImageImplRhsT>::ChannelT>::value) {
    return false;
  }


  // If both images are contiguous we can do a single memcmp, whereas if either
  // image is not we must compare rows individually.
  if (lhs.IsMemoryContiguous() && rhs.IsMemoryContiguous()) {
    assert(lhs.TotalByteCount() == rhs.TotalByteCount());
    return memcmp(lhs.GetRow(0), rhs.GetRow(0), lhs.TotalByteCount()) == 0;
  } else {
    assert(lhs.RowSizeBytes() == rhs.RowSizeBytes());
    const std::size_t row_bytes = lhs.RowSizeBytes();
    for (int y = 0; y < lhs.Height(); ++y) {
      if (memcmp(lhs.GetRow(y), rhs.GetRow(y), row_bytes) != 0) {
        return false;
      }
    }
  }
  return true;  // If we made it this far, they must be the same.
}


// Inline member function definitions. ----------------------------------------

template <typename T, int NumChannels, typename Allocator>
ImageBuf<T, NumChannels, Allocator>::ImageBuf()
    : w_(-1),
      h_(-1),
      c_(-1),
      buf_(nullptr),
      owns_data_(true),
      row_stride_(0) {}

template <typename T, int NumChannels, typename Allocator>
ImageBuf<T, NumChannels, Allocator>::~ImageBuf() {
  FreeMemIfOwned();
}

template<typename T, int NumChannels, typename Allocator>
void ImageBuf<T, NumChannels, Allocator>::FreeMemIfOwned() {
  // TODO(cbraley): Make a thread safe version of this class that cleans up here in all
  // subwindows.
  if (owns_data_) {
    allocator_.deallocate(buf_, Width() * Height() * Channels());
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
      row_stride_(0) {
  static_assert(NumChannels == DYNAMIC_CHANNELS,
                "The ImageBuf(width, height, num_channels) constructor can "
                "only be called when the ImageBuf has a \"dynamic\" number "
                "of channels.  ImageBufs with a dynamic number of "
                "channels have their channel count chosen at runtime ("
                "as opposed to at compile time).");
  AllocateHelper(w_, h_, c_);
}

template <typename T, int NumChannels, typename Allocator>
ImageBuf<T, NumChannels, Allocator>::ImageBuf(int width, int height)
    : w_(width),
      h_(height),
      c_(NumChannels),
      buf_(nullptr),
      owns_data_(true),
      row_stride_(0) {
  static_assert(NumChannels != DYNAMIC_CHANNELS,
                "The ImageBuf(width, height) constructor can "
                "only be called when the ImageBuf has a \"static\" number "
                "of channels.  ImageBufs with a static number of "
                "channels have their channel count chosen at compile time ("
                "as opposed to at runtime).");
  static_assert(NumChannels > 0, "Invalid channel count.");
  AllocateHelper(w_, h_, NumChannels);
}


}  // namespace jr

#endif  // JRIMAGE_JRIMAGE_H_
