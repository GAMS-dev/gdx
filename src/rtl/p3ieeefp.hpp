#pragma once
#include <cstdint>

#if defined(__APPLE__) // hide cr*p from macOS math.h
#undef FP_NAN
#undef FP_SNAN
#undef FP_QNAN
#endif

namespace rtl::p3ieeefp {

enum class TFPClass : uint8_t
{
   FP_SNAN,   // signaling NaN
   FP_QNAN,   // quiet NaN
   FP_NINF,   // negative infinity
   FP_PINF,   // positive infinity
   FP_NDENORM,// negative denormalized non-zero
   FP_PDENORM,// positive denormalized non-zero
   FP_NZERO,  // -0.0
   FP_PZERO,  // +0.0
   FP_NNORM,  // negative normalized non-zero
   FP_PNORM   // positive normalized non-zero
};

bool p3IsFinite( double x );

TFPClass FPClass(double x );

}// namespace rtl::p3ieeefp