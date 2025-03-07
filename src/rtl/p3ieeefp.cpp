#include "gdlib/dblutil.hpp"
#include "p3ieeefp.hpp"

namespace rtl::p3ieeefp {

constexpr int64_t
   expoMask { static_cast<int64_t>( 0x7ff00000 ) << 32},
   signMask { static_cast<int64_t>( 0x80000000 ) << 32},
   mantMask { ~(signMask | expoMask) },
   qnanMask { static_cast<int64_t>(0x00080000) << 32};

bool p3IsFinite( const double x )
{
   const gdlib::dblutil::TI64Rec I64Rec { x };
   return ( I64Rec.i64 & expoMask ) >> 52 != 2047;
}

void double2i64( const double x, int64_t &i)
{
   const gdlib::dblutil::TI64Rec I64Rec {x};
   i = I64Rec.i64;
}

TFPClass FPClass( const double x )
{
   int64_t ix;
   double2i64( x, ix );
   const bool isNeg { ( ix & signMask ) == signMask };
   const int64_t exponent { ( ix & expoMask ) >> 52 };
   const bool isQuiet { ( ix & qnanMask ) == qnanMask };
   const int64_t mantissa {ix & mantMask};

   if(!exponent)
   {
      if(mantissa)
         return isNeg ? TFPClass::FP_NDENORM : TFPClass::FP_PDENORM;
      return isNeg ? TFPClass::FP_NZERO : TFPClass::FP_PZERO;
   }
   if(exponent == 2047)
   {
      if(!mantissa)
         return isNeg ? TFPClass::FP_NINF : TFPClass::FP_PINF;
      return isQuiet ? TFPClass::FP_QNAN : TFPClass::FP_SNAN;
   }
   if(isNeg)
      return TFPClass::FP_NNORM;
   return TFPClass::FP_PNORM;
}

}// namespace rtl::p3ieeefp
