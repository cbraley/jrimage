# JRImage Goals

1. Image channel count can be specified at compile time or runtime.
2. Image colorspaces can be specified at compile time or runtime.
  1. Colorspace conversion code is automatically generated via profile connection spaces.
3. Image color conversions are type-checked at compile time when possible.
4. Compiler error messages are simple to understand.
5. Memory layout chosen at compile time.
  1. Row major vs column major
  2. Interleaved vs planar
6. Images can support any bit depth (8 bit, 16 bit, float, etc.)
  1. Bit depth conversion is simple.
7. Images can share memory with other images.
7. Buffer ownership semantics are clear.
8. Allocator aware.
9. Easy to write functions operating on jrimage objects.
10. Eventual support of alpha channels.



TODOs

  General cleanup
  Point class for sampling
