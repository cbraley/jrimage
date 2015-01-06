#include <string>
#include <iostream>
#include <random>
#include <cstdint>

#include "gtest/gtest.h"

#include "jrimage.h"
#include "jrimage_allocators.h"

namespace {

TEST(JRImageBuf_Allocators, PixelSetters) {
  jr::ImageBuf<float, 3, jr::AlignedAllocator<float>> a;
}

TEST(JRImageBuf, MemSafetyAndCopying) {
  jr::ImageBuf<int> image(100, 100, 3);
  EXPECT_EQ(image.Width(), 100);
  EXPECT_EQ(image.Height(), 100);
  EXPECT_EQ(image.Channels(), 3);
}

TEST(JRImageBuf, Subwindows) {
  jr::ImageBuf<int, 2> image(3, 2);
  image.Set(0, 0, 0, 12);
  image.Set(1, 0, 0, 13);
  image.Set(2, 0, 0, 14);
  image.Set(0, 1, 0, 2);
  image.Set(1, 1, 0, 3);
  image.Set(2, 1, 0, 4);

  image.Set(0, 0, 1, 5);
  image.Set(1, 0, 1, 6);
  image.Set(2, 0, 1, 7);
  image.Set(0, 1, 1, 8);
  image.Set(1, 1, 1, 9);
  image.Set(2, 1, 1, 10);

  // channel 0
  // [ 12  13   14 ]
  // [  2   3    4 ]

  // channel 1
  // [ 5  6   7 ]
  // [ 8  9  10 ]

  EXPECT_EQ(image.Width(), 3);
  EXPECT_EQ(image.Height(), 2);
  EXPECT_EQ(image.Channels(), 2);
  EXPECT_EQ(image, image);

  // Take a subwindow of full size.  This subwindow should contain
  // the same pixel data.
  {
    jr::ImageBuf<int, 2> win;
    bool success = image.GetWindow(0, 0, 3, 2, win);
    EXPECT_TRUE(success);
    EXPECT_EQ(image, win);
  }

  // Take single pixel subwindows.
  {
    bool success = false;
    for (int y = 0; y < image.Height(); ++y) {
      for (int x = 0; x < image.Width(); ++x) {
        jr::ImageBuf<int, 2> win, win_2;
        success = image.GetWindow(x, y, 1, 1, win);
        image.GetWindow(x, y, 1, 1, win_2);
        EXPECT_EQ(win, win_2);
        EXPECT_TRUE(success);
        EXPECT_EQ(1, win.Width());
        EXPECT_EQ(1, win.Height());
        for (int c = 0; c < 2; ++c) {
          EXPECT_EQ(image.Get(x, y, c), win.Get(0, 0, c));
        }
      }
    }
  }

  // Make sure invalid subwindows return false.
  {
    bool success = true;
    jr::ImageBuf<int, 2> win;

    success = true;
    success = image.GetWindow(0, -1, 1, 1, win);
    EXPECT_FALSE(success);

    success = true;
    success = image.GetWindow(-1, 0, 1, 1, win);
    EXPECT_FALSE(success);

    success = true;
    success = image.GetWindow(image.Width(), 0, 1, 1, win);
    EXPECT_FALSE(success);

    success = true;
    success = image.GetWindow(0, image.Height(), 1, 1, win);
    EXPECT_FALSE(success);

    success = true;
    success = image.GetWindow(0, 0, image.Width() + 1, 1, win);
    EXPECT_FALSE(success);

    success = true;
    success = image.GetWindow(0, 0, 1, image.Height() + 1, win);
    EXPECT_FALSE(success);
  }
}

TEST(JRImageBuf, DeepComparison) {
  // Two images that point to the same thing are the same thing are the same.
  jr::ImageBuf<unsigned char, 4> a, *b;
  b = &a;
  EXPECT_TRUE(a.Resize(100, 200, 4));
  EXPECT_EQ(a, *b);

  // Two images that are deep copies of each other should compare the same.
  jr::ImageBuf<unsigned char, 4> c, d;
  a.CopyInto(c);
  a.CopyInto(d);
  EXPECT_EQ(a, c);
  EXPECT_EQ(*b, c);
  EXPECT_EQ(a, d);
  EXPECT_EQ(*b, d);
}


TEST(JRImageBuf, AllocateReallocation) {
  // Construct an image.
  jr::ImageBuf<double> image(100, 200, 4);
  EXPECT_EQ(100, image.Width());
  EXPECT_EQ(200, image.Height());
  EXPECT_EQ(4, image.Channels());
  EXPECT_TRUE(image.IsChannelCountDynamic());
  EXPECT_FALSE(image.IsChannelCountStatic());

  // Reallocate it to a new size.
  image.Allocate(10, 400, 5);
  EXPECT_EQ(10, image.Width());
  EXPECT_EQ(400, image.Height());
  EXPECT_EQ(5, image.Channels());
  EXPECT_TRUE(image.IsChannelCountDynamic());
  EXPECT_FALSE(image.IsChannelCountStatic());

  // Reallocate it to *the same* size.
  image.Allocate(10, 400, 5);
  EXPECT_EQ(10, image.Width());
  EXPECT_EQ(400, image.Height());
  EXPECT_EQ(5, image.Channels());
  EXPECT_TRUE(image.IsChannelCountDynamic());
  EXPECT_FALSE(image.IsChannelCountStatic());
}


TEST(JRImageBuf_Stress, MemoryBugRepro) {
  jr::ImageBuf<float> image(100, 200, 4);
  image.Allocate(2, 4, 4);
  EXPECT_EQ(image.Width(), 2);
  EXPECT_EQ(image.Height(), 4);
  EXPECT_EQ(image.Channels(), 4);
  EXPECT_TRUE(image.InBounds(0, 2, 2));
  EXPECT_EQ(image.Width(), 2);
  EXPECT_EQ(image.Height(), 4);
  EXPECT_EQ(image.Channels(), 4);
}


TEST(JRImageBuf, SimpleSetAllChannels) {
  jr::ImageBuf<float> image(100, 200, 4);
  const float TEST_VAL = 12.0f;
  image.Set(12, 44, 2, TEST_VAL);
  image.Set(12, 44, 0, 13);

  float buf[4];
  image.GetAllChannels(12, 44, buf);
  EXPECT_EQ(TEST_VAL, buf[2]);
  EXPECT_EQ(13, buf[0]);
}

// Set random pixel values many times and make sure we can read the
// same values back.
TEST(JRImageBuf_Stress, PixelSetters) {
  jr::ImageBuf<float> image(100, 200, 4);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> channel_dist(1, 5);
  std::uniform_int_distribution<> width_dist(1, 10);
  std::uniform_int_distribution<> height_dist(1, 10);

  const int NUM_TRIALS = 100;
  const int NUM_PIXELS = 1000;
  for (int i = 0; i < NUM_TRIALS; ++i) {
    const int new_w = width_dist(gen);
    const int new_h = height_dist(gen);
    const int new_c = channel_dist(gen);
    image.Allocate(new_w, new_h, new_c);
    ASSERT_EQ(new_w, image.Width());
    ASSERT_EQ(new_h, image.Height());
    ASSERT_EQ(new_c, image.Channels());

    std::uniform_int_distribution<> x_dist(0, image.Width() - 1);
    std::uniform_int_distribution<> y_dist(0, image.Height() - 1);
    std::uniform_int_distribution<> c_dist(0, image.Channels() - 1);
    std::uniform_real_distribution<float> values(-9999.0f, 9999.0f);
    float buf[100];
    float buf2[100];
    for (int j = 0; j < NUM_PIXELS; ++j) {
      ASSERT_EQ(new_w, image.Width());
      ASSERT_EQ(new_h, image.Height());
      ASSERT_EQ(new_c, image.Channels());

      const int x = x_dist(gen);
      const int y = y_dist(gen);
      const int c = c_dist(gen);
      ASSERT_GE(x, 0);
      ASSERT_GE(y, 0);
      ASSERT_GE(c, 0);
      ASSERT_LE(x, image.Width() - 1);
      ASSERT_LE(y, image.Height() - 1);
      ASSERT_LE(c, image.Channels() - 1);

      ASSERT_TRUE(image.InBounds(x, y, c))
          << "(" << x << ", " << y << ", " << c << ") out of bounds "
          << "for an image with dims " << image.Width() << ", "
          << image.Height() << ", " << image.Channels();

      // First, set a single channel.
      const float value = values(gen);
      image.Set(x, y, c, value);

      jr::ImageBuf<float> le_copy;
      image.CopyInto(le_copy);

      ASSERT_EQ(value, le_copy.Get(x, y, c));
      le_copy.GetAllChannels(x, y, buf);
      ASSERT_EQ(value, buf[c]);

      // Now, set all channels.
      for (int chan = 0; chan < image.Channels(); ++chan) {
        buf[chan] = values(gen);
      }
      image.SetAllChannels(x, y, buf);
      for (int chan = 0; chan < image.Channels(); ++chan) {
        ASSERT_EQ(buf[chan], image.Get(x, y, chan))
            << "Expected channel " << chan << " location ("
            << x << ", " << y << ")";
      }
      image.GetAllChannels(x, y, buf2);
      ASSERT_EQ(0, memcmp(static_cast<const void*>(buf),
                          static_cast<const void*>(buf2),
                          image.Channels() * sizeof(float)));
    }
  }
}

// Test constructing an image with a dynamic number of channels.
TEST(JRImageBuf, DynamicImageBufConstructors) {
  jr::ImageBuf<unsigned char> im_dynamic(100, 200, 3);
  EXPECT_TRUE(im_dynamic.IsChannelCountDynamic());
  EXPECT_EQ(3, im_dynamic.Channels());
  EXPECT_EQ(100, im_dynamic.Width());
  EXPECT_EQ(200, im_dynamic.Height());
}

// Test constructing an image with a static number of channels.
TEST(JRImageBuf, StaticImageBufConstructors) {
  jr::ImageBuf<float, 5> im_static(100, 200);
  EXPECT_FALSE(im_static.IsChannelCountDynamic());
  EXPECT_EQ(5, im_static.Channels());
  EXPECT_EQ(100, im_static.Width());
  EXPECT_EQ(200, im_static.Height());
}

double Func(int x, int y, int chan) {
  const double dx = static_cast<double>(x);
  const double dy = static_cast<double>(y);
  const double dc = static_cast<double>(chan);
  return dx * dy * dc + dx * 23.0 + dy * dy * 14.0 + sin(dc);
}

template <typename T, int CHAN>
int SampleFuncIntoImageBuf(const std::function<T(int, int, int)>& func,
                        jr::ImageBuf<T, CHAN>& image) {
  int num_evals = 0;
  for (int y = 0; y < image.Height(); ++y) {
    for (int x = 0; x < image.Width(); ++x) {
      for (int c = 0; c < image.Channels(); ++c) {
        image.Set(x, y, c, func(x, y, c));
        ++num_evals;
      }
    }
  }
  return num_evals;
}

float F(int x, int y, int c) {
  return static_cast<float>(x + y + c) + 12.0f +
         static_cast<float>(x * x * -1.f) + static_cast<float>(y * x * 3.3f) +
         static_cast<float>(c * y * 2.0f);
}

/*
TEST(JRImageBuf, ImageBufResizing) {
  jr::ImageBuf<uint32_t> gold(100, 200, 4);
  SampleFuncIntoImageBuf<uint32_t, jr::DYNAMIC_CHANNELS>(F, gold);

  jr::ImageBuf<uint32_t> resized_gold(11, 30, 4);
  SampleFuncIntoImageBuf<uint32_t, jr::DYNAMIC_CHANNELS>(F, resized_gold);

  jr::ImageBuf<uint32_t> resized_two_chan_gold(5, 13, 2);
  SampleFuncIntoImageBuf<uint32_t, jr::DYNAMIC_CHANNELS>(F, resized_two_chan_gold);

  jr::ImageBuf<uint32_t, 4> dest_static;
  gold.CopyInto(dest_static);
  EXPECT_EQ(gold, dest_static);

  jr::ImageBuf<uint32_t> dest_dynamic;
  gold.CopyInto(dest_dynamic);
  EXPECT_EQ(gold, dest_static);

  dest_static.Resize(11, 30, dest_static.Channels());
  EXPECT_EQ(resized_gold, dest_static);

  dest_dynamic.Resize(11, 30, dest_dynamic.Channels());
  EXPECT_EQ(resized_gold, dest_dynamic);

  dest_dynamic.Resize(5, 13, 2);
  EXPECT_EQ(resized_two_chan_gold, dest_dynamic);
}
*/

TEST(JRImageBuf, ImageBufCopying) {
  {
    // Simple small cases.
    jr::ImageBuf<int, 2> a(1, 1);
    a.Set(0, 0, 0, 22);
    a.Set(0, 0, 1, 44);

    jr::ImageBuf<int, 2> b(4, 4);
    a.CopyInto(b);

    EXPECT_TRUE(jr::DimensionsMatch(a, b));
    EXPECT_EQ(22, a.Get(0, 0, 0));
    EXPECT_EQ(22, b.Get(0, 0, 0));

    EXPECT_EQ(44, a.Get(0, 0, 1));
    EXPECT_EQ(44, b.Get(0, 0, 1));
  }

  jr::ImageBuf<float, 6> static_gold(312, 453);
  EXPECT_EQ(6, static_gold.Channels());
  jr::ImageBuf<float> dynamic_gold(312, 453, 6);
  EXPECT_EQ(6, dynamic_gold.Channels());

  SampleFuncIntoImageBuf<float, 6>(F, static_gold);
  SampleFuncIntoImageBuf<float, jr::DYNAMIC_CHANNELS>(F, dynamic_gold);
  EXPECT_EQ(static_gold, dynamic_gold);

  // Static to static.
  {
    jr::ImageBuf<float, 2> gold(1, 2);
    EXPECT_EQ(2, gold.Channels());
    SampleFuncIntoImageBuf<float, 2>(F, gold);

    jr::ImageBuf<float, 2> copy(333, 1);
    EXPECT_TRUE(gold.CopyInto(copy));
    EXPECT_TRUE(jr::DimensionsMatch(copy, gold));
    EXPECT_EQ(copy, gold);

    jr::ImageBuf<float, 5> tmp1(312, 453);
    jr::ImageBuf<float, 6> tmp2(333, 1);
    EXPECT_TRUE(!static_gold.CopyInto(tmp1));
    EXPECT_TRUE(static_gold.CopyInto(tmp2));
    EXPECT_EQ(tmp2, static_gold);
  }
}


}  // anonymous namespace

