#include <string>
#include <iostream>
#include <cassert>
#include <random>

#undef NDEBUG

#include "jrimage.h"
#include "mem_utils.h"
#include "math_utils.h"

void rounding() {
  while (true) {
    std::cout << "Enter x and y: ";
    int x,y;
    std::cin >> x;
    std::cin >> y;
    std::cout << std::endl;
    std::cout << "RoundUpToMuliple(" << x << ", " << y << ") = "
              << jr::math_utils::UpToNearestMultiple(x, y) << std::endl;

  }
}


void stress_setters() {
  jr::Image<float> image(100, 200, 4);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> channel_dist(1, 5);
  std::uniform_int_distribution<> width_dist(1, 10);
  std::uniform_int_distribution<> height_dist(1, 10);

  const int NUM_TRIALS = 1000;
  const int NUM_PIXELS = 1000;
  for (int i = 0; i < NUM_TRIALS; ++i) {
    image.Resize(width_dist(gen), height_dist(gen), channel_dist(gen));
    
    std::uniform_int_distribution<> x_dist(0, image.Width() - 1);
    std::uniform_int_distribution<> y_dist(0, image.Height() - 1);
    std::uniform_int_distribution<> c_dist(0, image.Channels() - 1);
    std::uniform_real_distribution<float> values(-9999.0f, 9999.0f);
    float buf[100];
    for (int j = 0; j < NUM_PIXELS; ++j) {
      int x = x_dist(gen);
      int y = y_dist(gen);
      int c = c_dist(gen);
      assert(image.InBounds(x, y, c));
      
      const float value = values(gen);
      image.Set(x, y, c, value);

      jr::Image<float> le_copy;
      image.CopyInto(le_copy);

      assert(le_copy.Get(x, y, c) == value);
      le_copy.GetAllChannels(x, y, buf);
      assert(value == buf[c]);
    }
    
  }


}

template<typename T>
struct MemTestBlock {
  MemTestBlock(T* pointer, size_t numel) :
      ptr(pointer), n(numel) {}
  T* ptr;
  size_t n;
};

template<typename T>
void AlignedAllocTest(size_t align,
                      const std::function<T(int)>& func,
                      int max_array_size, int num_tests) {
  std::cout << "AlignedAllocTest with byte alignment of " 
            << align << ".  Doing " << num_tests << " allocations "
            << "with array sizes in the range [0, " << max_array_size << ")" 
            << std::endl;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, max_array_size);

  std::vector<MemTestBlock<T>> mem_blocks;

  size_t size_sum = 0;
  for (int i = 0; i < num_tests; ++i) {
    const size_t numel = dis(gen);
    size_sum += numel;

    T* mem = jr::mem_utils::AlignedNew<T>(numel, align);
    assert(jr::mem_utils::IsPointerAligned(mem, align));
    //std::cout << "\t\tAllocated block at " << (void*)(mem) << std::endl;

    for (int i = 0; i < numel; ++i) {
      mem[i] = func(i);
    }

    mem_blocks.push_back(MemTestBlock<T>(mem, numel));
  }

  assert(mem_blocks.size() == num_tests);
  size_t size_sum_check = 0;
  for (size_t j = 0; j < mem_blocks.size(); ++j) {
    T* mem = mem_blocks[j].ptr;
    const size_t numel = mem_blocks[j].n;
    size_sum_check += numel;

    for (int i = numel - 1; i >= 0; --i) {
      const T expected = func(i);
      assert(mem[i] == expected);
    }
    //std::cout << "\t\tDeleting  " << (void*)(mem) << std::endl;
    jr::mem_utils::AlignedDelete(mem);
  }
  assert(size_sum == size_sum_check);
  std::cout << "Test for alignment " << align << " complete." << std::endl;
}


template<typename T>
T TestFunc(int index) {
  if (index == 0) {
    return static_cast<T>(54634646.0);
  }
  return static_cast<T>(sin(static_cast<double>(index)) +
         22.30 + 
         static_cast<double>(index * 3.0));
}

void mem_test() {
  const int num_tests = 1000;

  const int ARRAY_LIMITS[] = {0, 1, 10, 1000, 2012};

  for (int i = 0; i < sizeof(ARRAY_LIMITS) / sizeof(ARRAY_LIMITS[0]); ++i) {

    const int arr_size = ARRAY_LIMITS[i];

    AlignedAllocTest<unsigned char>(0,    TestFunc<unsigned char>, arr_size, num_tests);
    AlignedAllocTest<unsigned char>(4,    TestFunc<unsigned char>, arr_size, num_tests);
    AlignedAllocTest<unsigned char>(16,   TestFunc<unsigned char>, arr_size, num_tests);
    AlignedAllocTest<unsigned char>(128,  TestFunc<unsigned char>, arr_size, num_tests);
    AlignedAllocTest<unsigned char>(256,  TestFunc<unsigned char>, arr_size, num_tests);
    AlignedAllocTest<unsigned char>(1024, TestFunc<unsigned char>, arr_size, num_tests);
    AlignedAllocTest<unsigned char>(2048, TestFunc<unsigned char>, arr_size, num_tests);

    AlignedAllocTest<float>(0,    TestFunc<float>, arr_size, num_tests);
    AlignedAllocTest<float>(4,    TestFunc<float>, arr_size, num_tests);
    AlignedAllocTest<float>(16,   TestFunc<float>, arr_size, num_tests);
    AlignedAllocTest<float>(128,  TestFunc<float>, arr_size, num_tests);
    AlignedAllocTest<float>(256,  TestFunc<float>, arr_size, num_tests);
    AlignedAllocTest<float>(1024, TestFunc<float>, arr_size, num_tests);
    AlignedAllocTest<float>(2048, TestFunc<float>, arr_size, num_tests);
  }
}

void dynamic_image_construction() {
  std::cout << "dynamic_image_construction" << std::endl;
  jr::Image<unsigned char> im_dynamic(100, 200, 3);
  assert(im_dynamic.IsChannelCountDynamic());
  assert(im_dynamic.Channels() == 3);
  assert(im_dynamic.Width() == 100);
  assert(im_dynamic.Height() == 200);
}

void static_image_construction() {
  std::cout << "static_image_construction" << std::endl;
  jr::Image<float, 5> im_static(100, 200);
  assert(!im_static.IsChannelCountDynamic());
  assert(im_static.Channels() == 5);
  assert(im_static.Width() == 100);
  assert(im_static.Height() == 200);
}

double Func(int x, int y, int chan) {
  const double dx = static_cast<double>(x);
  const double dy = static_cast<double>(y);
  const double dc = static_cast<double>(chan);
  return dx*dy*dc + 
         dx * 23.0 + 
         dy * dy * 14.0 + 
         sin(dc);
}

void image_resizing() {
  std::cout << "image_resizing" << std::endl;
  
  
  jr::Image<double, 4> A(1, 1);
  A.Set(0, 0, 0, 11.0);
  A.Set(0, 0, 1, 22.0);
  A.Set(0, 0, 2, 33.0);
  A.Set(0, 0, 3, 44.0);

  jr::Image<double, 4> copy(22,44);
  A.CopyInto(copy);
  assert(A == copy);

  return;
 
  /*
  jr::Image<double, 4> im_static(111, 223);
  double buf[4];
  for (int y = 0; y < im_static.Height(); ++y) {
    for (int x = 0; x < im_static.Width(); ++x) {
      
      for (int c = 0; c < im_static.Channels(); ++c) {
        buf[c] = Func(x, y, c);
      }
      
      im_static.SetAllChannels(x, y, buf);
    }
  }

  jr::Image<double, 4> copy1, copy2, copy3, copy4;
  im_static.CopyInto(copy1);
  im_static.CopyInto(copy2);
  im_static.CopyInto(copy3);
  im_static.CopyInto(copy4);

  copy1.Resize(1, 34);
  copy2.Resize(555, 1);
  copy3.Resize(22, 44);
  copy4.Resize(111, 223);

  std::vector<jr::Image<double, 4>*> images;
  images.push_back(&copy1);
  images.push_back(&copy2);
  images.push_back(&copy3);
  images.push_back(&copy4);
  
  for (size_t i = 0; i < images.size(); ++i) {
    std::cout << "Testing copied image " << i << std::endl;
    const jr::Image<double, 4>& im = *(images[i]);
    for (int y = 0; y < im.Height(); ++y) {
      for (int x = 0; x < im.Width(); ++x) {
        for (int c = 0; c < im.Channels(); ++c) {
          const double expected =
              im_static.InBounds(x, y, c) ? Func(x, y, c) : 0.0;
          const double val = im.Get(x, y, c);
          if (val != expected) {
            std::cout << "Val = " << val << " but expected = " << expected << std::endl;
            std::cout << "Indices = " << x << ", " << y << ", " << c << std::endl;
          }
          assert(val == expected);
        }
      }
    }
  }
  */
}

template<typename T, int CHAN>
int SampleFuncIntoImage(const std::function<T(int,int,int)>& func,
                        jr::Image<T, CHAN>& image) {
  int num_evals = 0;
  for (int y = 0; y < image.Height(); ++y) {
    for (int x = 0; x < image.Width(); ++x) {
      for (int c = 0; c < image.Channels(); ++c) {
        image.Set(x,y,c, func(x,y,c));
        ++num_evals;
      }
    }
  }
  return num_evals;
}

float F(int x, int y, int c) {
  return static_cast<float>(x + y + c) + 12.0f + 
         static_cast<float>(x*x*-1.f) + 
         static_cast<float>(y*x*3.3f) + 
         static_cast<float>(c*y*2.0f); 
}

void image_copying() {
  {
    // Simple small cases.
    jr::Image<int, 2> a(1,1);
    a.Set(0,0,0, 22);
    a.Set(0,0,1, 44);

    jr::Image<int, 2> b(4,4);
    a.CopyInto(b);

    assert(jr::DimensionsMatch(a, b));
    assert(a.Get(0,0,0) == 22);
    assert(b.Get(0,0,0) == 22);
    
    assert(a.Get(0,0,1) == 44);
    assert(b.Get(0,0,1) == 44);
  }
 

  jr::Image<float, 6> static_gold(312, 453);
  assert(static_gold.Channels() == 6);
  jr::Image<float> dynamic_gold(312, 453, 6);
  assert(dynamic_gold.Channels() == 6);
    
  SampleFuncIntoImage<float, 6>(F, static_gold);
  SampleFuncIntoImage<float, jr::DYNAMIC_CHANNELS>(F, dynamic_gold);
  assert(static_gold == dynamic_gold);

  // Static to static.
  {
    jr::Image<float, 2> gold(1, 2);
    assert(gold.Channels() == 2);
    SampleFuncIntoImage<float, 2>(F, gold);
    std::cout << "Original gold = " << gold << std::endl;

    jr::Image<float, 2> copy(333, 1);
    assert(gold.CopyInto(copy));
    std::cout << "Copy  = " << copy << std::endl;
    std::cout << "Gold again = " << gold << std::endl;
    assert(jr::DimensionsMatch(copy, gold));
    
    
    copy.DebugPrintBuffer(std::cout);
    gold.DebugPrintBuffer(std::cout);
    
    assert(copy == gold);

    jr::Image<float, 5> tmp1(312, 453);
    jr::Image<float, 6> tmp2(333, 1);
    assert(!static_gold.CopyInto(tmp1));
    assert(static_gold.CopyInto(tmp2));
    assert(tmp2 == static_gold);
  }
}

void bilinear() {
  jr::Image<float, 3> tmp(200, 200);
  float out[3];
  tmp.BilinearSample(0.01, 0.02, out);
}


int main(int argc, char** argv) {
  std::cout << "Starting tests..." << std::endl;
  //rounding();
  bilinear();
  stress_setters();
  mem_test();
  dynamic_image_construction();
  static_image_construction(); 
  image_copying();
  image_resizing();
  std::cout << "Tests complete!" << std::endl;
  return 0;
}
