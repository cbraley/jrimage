#ifndef JRIMAGE_TEMPLATE_UTILS_H_
#define JRIMAGE_TEMPLATE_UTILS_H_

#include <cassert>
#include <type_traits>
#include <limits>

namespace jr {
namespace template_utils {

// TODO(cbraley): Merge in code from jrimage_old!!!!

template<typename IntegerT, IntegerT power>
constexpr IntegerT Power(IntegerT input) {
  return input * Power<IntegerT, power - 1>(input);
}

template<typename T>
void SetIfNonNull(const T& to_set_to, T* ptr) {
  if (ptr != nullptr) {
    *ptr = to_set_to;
  }
}

/*
// Using ArithmeticWorkingType<Foo>::WorkingTypeT
// you can get an apprioriate numeric type that can be
// used for intermedaite computation when using
// image data of type Foo.
template<typename T>
struct ArithmeticWorkingType {
  static_assert(
      std::is_arithmetic<T>::value,
      "You can only query the arithmetic working type for arithmetic types!");
};

// Specializations for each primitive type.
template<> struct ArithmeticWorkingType<unsigned char>  { typedef float WorkingTypeT;  };
template<> struct ArithmeticWorkingType<char>           { typedef float WorkingTypeT;  };
template<> struct ArithmeticWorkingType<unsigned short> { typedef float WorkingTypeT;  };
template<> struct ArithmeticWorkingType<short>          { typedef float WorkingTypeT;  };
template<> struct ArithmeticWorkingType<unsigned int>   { typedef float WorkingTypeT;  };
template<> struct ArithmeticWorkingType<int>            { typedef float WorkingTypeT;  };
template<> struct ArithmeticWorkingType<long>           { typedef float WorkingTypeT;  };
template<> struct ArithmeticWorkingType<unsigned long>  { typedef float WorkingTypeT;  };
template<> struct ArithmeticWorkingType<float>          { typedef float WorkingTypeT;  };
template<> struct ArithmeticWorkingType<double>         { typedef double WorkingTypeT; };
*/
// TODO(cbraley): The above is no longer needed since we use std::conditional.

// This function converts the primitive type FromT to the primitive
// type ToT.
// This operation is the same as OpenCV's saturate_cast function.
/*
template<typename FromT, typename ToT>
ToT ConvertWithSaturation(FromT from) {
  // TODO(cbraley): Code!
  return ToT(from);
}
*/

}  // namespace template_utils
} // namespace jr

#endif  // JRIMAGE_TEMPLATE_UTILS_H_

