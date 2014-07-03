#include <string>
#include <iostream>
#include <random>
#include <cstdint>

#include "gtest/gtest.h"

#include "jrimage.h"

namespace {

// Set random pixel values many times and make sure we can read the
// same values back.
TEST(JRImage_Stress, PixelSetters) {
  jr::Image<float> image(100, 200, 4);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> channel_dist(1, 5);
  std::uniform_int_distribution<> width_dist(1, 10);
  std::uniform_int_distribution<> height_dist(1, 10);

  const int NUM_TRIALS = 1000;
  const int NUM_PIXELS = 1000;
  for (int i = 0; i < NUM_TRIALS; ++i) {
    image.Allocate(width_dist(gen), height_dist(gen), channel_dist(gen));

    std::uniform_int_distribution<> x_dist(0, image.Width() - 1);
    std::uniform_int_distribution<> y_dist(0, image.Height() - 1);
    std::uniform_int_distribution<> c_dist(0, image.Channels() - 1);
    std::uniform_real_distribution<float> values(-9999.0f, 9999.0f);
    float buf[100];
    float buf2[100];
    for (int j = 0; j < NUM_PIXELS; ++j) {
      const int x = x_dist(gen);
      const int y = y_dist(gen);
      const int c = c_dist(gen);
      EXPECT_TRUE(image.InBounds(x, y, c));

      // First, set a single channel.
      const float value = values(gen);
      image.Set(x, y, c, value);

      jr::Image<float> le_copy;
      image.CopyInto(le_copy);

      EXPECT_EQ(value, le_copy.Get(x, y, c));
      le_copy.GetAllChannels(x, y, buf);
      EXPECT_EQ(value, buf[c]);

      // Now, set all channgels.
      for (int chan = 0; chan < image.Channels(); ++chan) {
        buf[chan] = values(gen);
      }
      image.SetAllChannels(x, y, buf);
      for (int chan = 0; chan < image.Channels(); ++chan) {
        EXPECT_EQ(buf[chan], image.Get(x, y, chan));
      }
      image.GetAllChannels(x, y, buf2);
      EXPECT_EQ(0, memcmp(static_cast<const void*>(buf),
                          static_cast<const void*>(buf2),
                          image.Channels() * sizeof(float)));
    }
  }
}

// Test constructing an image with a dynamic number of channels.
TEST(JRImage, DynamicImageConstructors) {
  jr::Image<unsigned char> im_dynamic(100, 200, 3);
  EXPECT_TRUE(im_dynamic.IsChannelCountDynamic());
  EXPECT_EQ(3, im_dynamic.Channels());
  EXPECT_EQ(100, im_dynamic.Width());
  EXPECT_EQ(200, im_dynamic.Height());
}

// Test constructing an image with a static number of channels.
TEST(JRImage, StaticImageConstructors) {
  jr::Image<float, 5> im_static(100, 200);
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
int SampleFuncIntoImage(const std::function<T(int, int, int)>& func,
                        jr::Image<T, CHAN>& image) {
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

TEST(JRImage, ImageResizing) {
  jr::Image<uint32_t> gold(100, 200, 4);
  SampleFuncIntoImage<uint32_t, jr::DYNAMIC_CHANNELS>(F, gold);

  jr::Image<uint32_t> resized_gold(11, 30, 4);
  SampleFuncIntoImage<uint32_t, jr::DYNAMIC_CHANNELS>(F, resized_gold);

  jr::Image<uint32_t> resized_two_chan_gold(5, 13, 2);
  SampleFuncIntoImage<uint32_t, jr::DYNAMIC_CHANNELS>(F, resized_two_chan_gold);

  jr::Image<uint32_t, 4> dest_static;
  gold.CopyInto(dest_static);
  EXPECT_EQ(gold, dest_static);

  jr::Image<uint32_t> dest_dynamic;
  gold.CopyInto(dest_dynamic);
  EXPECT_EQ(gold, dest_static);

  /*
  dest_static.Resize(11, 30, dest_static.Channels());
  EXPECT_EQ(resized_gold, dest_static);

  dest_dynamic.Resize(11, 30, dest_dynamic.Channels());
  EXPECT_EQ(resized_gold, dest_dynamic);

  dest_dynamic.Resize(5, 13, 2);
  EXPECT_EQ(resized_two_chan_gold, dest_dynamic);
  */
}

TEST(JRImage, ImageCopying) {
  {
    // Simple small cases.
    jr::Image<int, 2> a(1, 1);
    a.Set(0, 0, 0, 22);
    a.Set(0, 0, 1, 44);

    jr::Image<int, 2> b(4, 4);
    a.CopyInto(b);

    EXPECT_TRUE(jr::DimensionsMatch(a, b));
    EXPECT_EQ(22, a.Get(0, 0, 0));
    EXPECT_EQ(22, b.Get(0, 0, 0));

    EXPECT_EQ(44, a.Get(0, 0, 1));
    EXPECT_EQ(44, b.Get(0, 0, 1));
  }

  jr::Image<float, 6> static_gold(312, 453);
  EXPECT_EQ(6, static_gold.Channels());
  jr::Image<float> dynamic_gold(312, 453, 6);
  EXPECT_EQ(6, dynamic_gold.Channels());

  SampleFuncIntoImage<float, 6>(F, static_gold);
  SampleFuncIntoImage<float, jr::DYNAMIC_CHANNELS>(F, dynamic_gold);
  EXPECT_EQ(static_gold, dynamic_gold);

  // Static to static.
  {
    jr::Image<float, 2> gold(1, 2);
    EXPECT_EQ(2, gold.Channels());
    SampleFuncIntoImage<float, 2>(F, gold);

    jr::Image<float, 2> copy(333, 1);
    EXPECT_TRUE(gold.CopyInto(copy));
    EXPECT_TRUE(jr::DimensionsMatch(copy, gold));
    EXPECT_EQ(copy, gold);

    jr::Image<float, 5> tmp1(312, 453);
    jr::Image<float, 6> tmp2(333, 1);
    EXPECT_TRUE(!static_gold.CopyInto(tmp1));
    EXPECT_TRUE(static_gold.CopyInto(tmp2));
    EXPECT_EQ(tmp2, static_gold);
  }
}

}  // anonymous namespace

