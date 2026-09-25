/*
* GAMS - General Algebraic Modeling System GDX API
 *
 * Copyright (c) 2017-2026 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2026 GAMS Development Corp. <support@gams.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "math_p3.hpp"

#include <cfenv>
#include <cstdlib>// for abs
#include <cmath>  // for log1p
#include <stdexcept>
#include <cfloat>

#if defined( __linux__ ) && !defined( __GLIBC__ )
#if defined( __x86_64__ )
#include <xmmintrin.h>
#elif !defined( __aarch64__ )
#error "FPE mask handling without glibc is only implemented on x86-64 and aarch64"
#endif
#endif

namespace GDX_NS rtl::math_p3
{

constexpr int64_t
        signMask { (int64_t) 0x80000000 << 32 },
        expoMask { (int64_t) 0x7ff00000 << 32 },
        mantMask { ~( signMask | expoMask ) };

union TI64Rec
{
   double x;
   int64_t i64;
};

bool IsNan( const double AValue )
{
   TI64Rec i64rec;
   i64rec.x = AValue;
   if( static_cast<uint64_t>( i64rec.i64 & expoMask ) >> 52 == 2047 )
   {
      int64_t mantissa = i64rec.i64 & mantMask;
      if( mantissa != 0 )
         return true;
   }
   return false;
}

bool IsInfinite( const double AValue )
{
   TI64Rec i64rec;
   i64rec.x = AValue;
   if( static_cast<uint64_t>( i64rec.i64 & expoMask ) >> 52 == 2047 )
   {
      int64_t mantissa = i64rec.i64 & mantMask;
      if( !mantissa )
         return true;
   }
   return false;
}

double IntPower( double X, const int I )
{
   double res { 1.0 };
   for( int Y { std::abs( I ) }; Y > 0; Y-- )
   {
      while( !( Y % 2 ) )
      {
         Y >>= 1;
         X *= X;
      }
      res *= X;
   }
   if( I < 0 )
      res = 1.0 / res;
   return res;
}

#if defined( __linux__ ) && !defined( __GLIBC__ ) && defined( __x86_64__ ) // LLM-generated; reviewed by OH
// MXCSR exception mask bits (a set bit masks the exception)
constexpr unsigned int mxcsrInvalid = 1u << 7, mxcsrDenormal = 1u << 8, mxcsrZeroDivide = 1u << 9,
                       mxcsrOverflow = 1u << 10, mxcsrUnderflow = 1u << 11, mxcsrPrecision = 1u << 12;
constexpr unsigned int mxcsrManaged = mxcsrInvalid | mxcsrDenormal | mxcsrZeroDivide | mxcsrOverflow | mxcsrUnderflow | mxcsrPrecision;
#elif defined( __linux__ ) && !defined( __GLIBC__ ) && defined( __aarch64__ )
// FPCR exception trap enable bits (a set bit makes the exception trap)
constexpr uint64_t fpcrInvalid = 1u << 8, fpcrZeroDivide = 1u << 9, fpcrOverflow = 1u << 10,
                   fpcrUnderflow = 1u << 11, fpcrPrecision = 1u << 12, fpcrDenormal = 1u << 15;
constexpr uint64_t fpcrManaged = fpcrInvalid | fpcrDenormal | fpcrZeroDivide | fpcrOverflow | fpcrUnderflow | fpcrPrecision;

static uint64_t readFPCR()
{
   uint64_t fpcr;
   __asm__ __volatile__( "mrs %0, fpcr" : "=r"( fpcr ) );
   return fpcr;
}

static void writeFPCR( uint64_t fpcr )
{
   __asm__ __volatile__( "msr fpcr, %0" : : "r"( fpcr ) );
}
#endif

double LnXP1( double x )
{
   return log1p( x );
}

/**
 * @brief Get current IEEE-754 exception mask (FPE mask) 
 * 
 * Note that a bit set to 1 means that no FPE is raised for the corresponding invalid operation
 *
 * @return       the current FPE mask 
 */
TFPUExceptionMask GetExceptionMask()
{
   TFPUExceptionMask result {};
   auto ADD2MASK = [&result]( TFPUException e ) {
      result.set( e );
   };

#if defined( _WIN32 )
   {
      unsigned int cw = 0;
      _controlfp_s( &cw, 0, 0 );
      if( cw & _EM_INVALID ) ADD2MASK( exInvalidOp );
      if( cw & _EM_DENORMAL ) ADD2MASK( exDenormalized );
      if( cw & _EM_ZERODIVIDE ) ADD2MASK( exZeroDivide );
      if( cw & _EM_OVERFLOW ) ADD2MASK( exOverflow );
      if( cw & _EM_UNDERFLOW ) ADD2MASK( exUnderflow );
      if( cw & _EM_INEXACT ) ADD2MASK( exPrecision );
   }
#elif defined( __APPLE__ ) && defined( __arm64__ )
   {
      fenv_t fenv;
      unsigned long long cw;
      // on ARM64, fenv.__fpcr seems to specify which floating-point exceptions should raise an exception (SIGILL (not even SIGFPE))
      // while on all other systems the floating-point control register says which exceptions should be masked (=not raise an exception)
      // that's why we negated the condition in the following if's
      (void) fegetenv( &fenv );
      cw = fenv.__fpcr & ( __fpcr_trap_invalid | __fpcr_trap_denormal | __fpcr_trap_divbyzero | __fpcr_trap_overflow | __fpcr_trap_underflow | __fpcr_trap_inexact );
      if( !( cw & __fpcr_trap_invalid ) ) ADD2MASK( exInvalidOp );
      if( !( cw & __fpcr_trap_denormal ) ) ADD2MASK( exDenormalized );
      if( !( cw & __fpcr_trap_divbyzero ) ) ADD2MASK( exZeroDivide );
      if( !( cw & __fpcr_trap_overflow ) ) ADD2MASK( exOverflow );
      if( !( cw & __fpcr_trap_underflow ) ) ADD2MASK( exUnderflow );
      if( !( cw & __fpcr_trap_inexact ) ) ADD2MASK( exPrecision );
   }
#elif defined( __APPLE__ )
   {
      fenv_t fenv;
      unsigned short cw;
      (void) fegetenv( &fenv );
      // SSE arithmetic on doubles is governed by MXCSR, whose mask bits are the x87 ones shifted by 7
      cw = ( fenv.__mxcsr >> 7 ) & FE_ALL_EXCEPT;
      if( cw & FE_INVALID ) ADD2MASK( exInvalidOp );
#if defined( FE_DENORMAL )
      if( cw & FE_DENORMAL ) ADD2MASK( exDenormalized );
#elif defined( FE_DENORMALOPERAND )// name used by macOS on Intel
      if( cw & FE_DENORMALOPERAND ) ADD2MASK( exDenormalized );
#else// assume always on if FE_DENORMAL not defined
      ADD2MASK( exDenormalized );
#endif
      if( cw & FE_DIVBYZERO ) ADD2MASK( exZeroDivide );
      if( cw & FE_OVERFLOW ) ADD2MASK( exOverflow );
      if( cw & FE_UNDERFLOW ) ADD2MASK( exUnderflow );
      if( cw & FE_INEXACT ) ADD2MASK( exPrecision );
   }
#elif defined( __linux__ ) && defined( __GLIBC__ ) // LLM-generated; OH checked
   {
      // fegetexcept returns the exceptions that are enabled, i.e. that trap (not masked)
      // On x86-64, it reflects both the x87 control word and MXCSR, on aarch64 the FPCR
      const int enabled = fegetexcept();
      if( !( enabled & FE_INVALID ) )   ADD2MASK( exInvalidOp );
#if defined( FE_DENORMAL )
      if( !( enabled & FE_DENORMAL ) )  ADD2MASK( exDenormalized );
#else// assume always on if FE_DENORMAL not defined
                                        ADD2MASK( exDenormalized );
#endif
      if( !( enabled & FE_DIVBYZERO ) ) ADD2MASK( exZeroDivide );
      if( !( enabled & FE_OVERFLOW ) )  ADD2MASK( exOverflow );
      if( !( enabled & FE_UNDERFLOW ) ) ADD2MASK( exUnderflow );
      if( !( enabled & FE_INEXACT ) )   ADD2MASK( exPrecision );
   }
#elif defined( __linux__ ) && defined( __x86_64__ )  // LLM-generated; OH checked
   {
      // non-glibc libc (e.g. musl) without feenableexcept: read MXCSR, which governs SSE arithmetic on doubles
      // a bit set in MXCSR means that the exception is masked
      const unsigned int csr = _mm_getcsr();
      if( csr & mxcsrInvalid )    ADD2MASK( exInvalidOp );
      if( csr & mxcsrDenormal )   ADD2MASK( exDenormalized );
      if( csr & mxcsrZeroDivide ) ADD2MASK( exZeroDivide );
      if( csr & mxcsrOverflow )   ADD2MASK( exOverflow );
      if( csr & mxcsrUnderflow )  ADD2MASK( exUnderflow );
      if( csr & mxcsrPrecision )  ADD2MASK( exPrecision );
   }
#elif defined( __linux__ ) && defined( __aarch64__ ) // LLM-generated; OH checked
   {
      // non-glibc libc (e.g. musl) without feenableexcept: read FPCR
      // a bit set in FPCR means that the exception traps (not masked)
      const uint64_t fpcr = readFPCR();
      if( !( fpcr & fpcrInvalid ) )    ADD2MASK( exInvalidOp );
      if( !( fpcr & fpcrDenormal ) )   ADD2MASK( exDenormalized );
      if( !( fpcr & fpcrZeroDivide ) ) ADD2MASK( exZeroDivide );
      if( !( fpcr & fpcrOverflow ) )   ADD2MASK( exOverflow );
      if( !( fpcr & fpcrUnderflow ) )  ADD2MASK( exUnderflow );
      if( !( fpcr & fpcrPrecision ) )  ADD2MASK( exPrecision );
   }
#else
// ...
#error "Function GetExceptionMask not implemented for this OS or compiler" is_not_implemented;
#endif
   return result;
}

/**
 * @brief Set IEEE-754 exception mask (FPE mask) from argument and return current values
 * 
 * Note that a bit set to 1 means that no FPE is raised for the corresponding invalid operation
 *
 * @param mask   the desired FPE mask
 *
 * @return       the FPE mask before the call
 */
TFPUExceptionMask SetExceptionMask( const TFPUExceptionMask &mask )
{
   TFPUExceptionMask curMask {};
   [[maybe_unused]] auto ADD2MASK = [&curMask]( TFPUException e ) {
      curMask.set( e );
   };

   auto ISINMASK = [&mask]( TFPUException e ) {
      return mask.test( e );
   };

#if defined( _WIN32 )
   {
      unsigned int cw = 0;
      _controlfp_s(&cw, 0, 0);

      if (cw & _EM_INVALID)   ADD2MASK(exInvalidOp);
      if (cw & _EM_DENORMAL)  ADD2MASK(exDenormalized);
      if (cw & _EM_ZERODIVIDE) ADD2MASK(exZeroDivide);
      if (cw & _EM_OVERFLOW)  ADD2MASK(exOverflow);
      if (cw & _EM_UNDERFLOW) ADD2MASK(exUnderflow);
      if (cw & _EM_INEXACT)   ADD2MASK(exPrecision);

      unsigned int tcw = 0;
      if (ISINMASK(exInvalidOp))   tcw |= _EM_INVALID;
      if (ISINMASK(exDenormalized)) tcw |= _EM_DENORMAL;
      if (ISINMASK(exZeroDivide))  tcw |= _EM_ZERODIVIDE;
      if (ISINMASK(exOverflow))    tcw |= _EM_OVERFLOW;
      if (ISINMASK(exUnderflow))   tcw |= _EM_UNDERFLOW;
      if (ISINMASK(exPrecision))   tcw |= _EM_INEXACT;

      unsigned int current_cw = 0;
      _controlfp_s(&current_cw, tcw, _MCW_EM);
   }
#elif defined( __APPLE__ ) && defined( __arm64__ )
   {
      fenv_t fenv;
      unsigned long long oldcw, newcw;

      /* on ARM64, fenv.__fpcr seems to specify which floating-point exceptions should raise an exception (SIGILL (not even SIGFPE))
* while on all other systems the floating-point control register says which exceptions should be masked (=not raise an exception)
* that's why we negated the condition in the following if's
*/
      (void) fegetenv( &fenv );
      oldcw = fenv.__fpcr & ( __fpcr_trap_invalid | __fpcr_trap_denormal | __fpcr_trap_divbyzero | __fpcr_trap_overflow | __fpcr_trap_underflow | __fpcr_trap_inexact );
      if( !( oldcw & __fpcr_trap_invalid ) ) ADD2MASK( exInvalidOp );
      if( !( oldcw & __fpcr_trap_denormal ) ) ADD2MASK( exDenormalized );
      if( !( oldcw & __fpcr_trap_divbyzero ) ) ADD2MASK( exZeroDivide );
      if( !( oldcw & __fpcr_trap_overflow ) ) ADD2MASK( exOverflow );
      if( !( oldcw & __fpcr_trap_underflow ) ) ADD2MASK( exUnderflow );
      if( !( oldcw & __fpcr_trap_inexact ) ) ADD2MASK( exPrecision );

      newcw = 0;
      if( !ISINMASK( exInvalidOp ) ) newcw |= __fpcr_trap_invalid;
      if( !ISINMASK( exDenormalized ) ) newcw |= __fpcr_trap_denormal;
      if( !ISINMASK( exZeroDivide ) ) newcw |= __fpcr_trap_divbyzero;
      if( !ISINMASK( exOverflow ) ) newcw |= __fpcr_trap_overflow;
      if( !ISINMASK( exUnderflow ) ) newcw |= __fpcr_trap_underflow;
      if( !ISINMASK( exPrecision ) ) newcw |= __fpcr_trap_inexact;
      fenv.__fpcr &= ~( __fpcr_trap_invalid | __fpcr_trap_denormal | __fpcr_trap_divbyzero | __fpcr_trap_overflow | __fpcr_trap_underflow | __fpcr_trap_inexact );
      fenv.__fpcr |= newcw;
      (void) fesetenv( &fenv );
   } /* all macOS */
#elif defined( __APPLE__ ) /* Mac on Intel, all compilers */
   {
      fenv_t fenv;
      unsigned short oldcw, newcw;

      (void) fegetenv( &fenv );
      // SSE arithmetic on doubles is governed by MXCSR, whose mask bits are the x87 ones shifted by 7
      oldcw = ( fenv.__mxcsr >> 7 ) & FE_ALL_EXCEPT;
      if( oldcw & FE_INVALID ) ADD2MASK( exInvalidOp );
#if defined( FE_DENORMAL )
      if( oldcw & FE_DENORMAL ) ADD2MASK( exDenormalized );
#elif defined( FE_DENORMALOPERAND )// name used by macOS on Intel
      if( oldcw & FE_DENORMALOPERAND ) ADD2MASK( exDenormalized );
#else// assume always on if FE_DENORMAL not defined
      ADD2MASK( exDenormalized );
#endif
      if( oldcw & FE_DIVBYZERO ) ADD2MASK( exZeroDivide );
      if( oldcw & FE_OVERFLOW ) ADD2MASK( exOverflow );
      if( oldcw & FE_UNDERFLOW ) ADD2MASK( exUnderflow );
      if( oldcw & FE_INEXACT ) ADD2MASK( exPrecision );

      newcw = FE_ALL_EXCEPT; /* start with all exceptions masked */
      /* next unmask only what we can mask */
      newcw -= FE_INVALID + FE_DIVBYZERO + FE_OVERFLOW + FE_UNDERFLOW + FE_INEXACT;
#if defined( FE_DENORMAL )
      newcw -= FE_DENORMAL;
#elif defined( FE_DENORMALOPERAND )
      newcw -= FE_DENORMALOPERAND;
#endif
      // all of this respects bits that must stay set

      if( ISINMASK( exInvalidOp ) ) newcw |= FE_INVALID;
#if defined( FE_DENORMAL )
      if( ISINMASK( exDenormalized ) ) newcw |= FE_DENORMAL;
#elif defined( FE_DENORMALOPERAND )// name used by macOS on Intel
      if( ISINMASK( exDenormalized ) ) newcw |= FE_DENORMALOPERAND;
#endif
      if( ISINMASK( exZeroDivide ) ) newcw |= FE_DIVBYZERO;
      if( ISINMASK( exOverflow ) ) newcw |= FE_OVERFLOW;
      if( ISINMASK( exUnderflow ) ) newcw |= FE_UNDERFLOW;
      if( ISINMASK( exPrecision ) ) newcw |= FE_INEXACT;
      fenv.__control &= ~FE_ALL_EXCEPT;
      fenv.__control |= newcw;
      fenv.__mxcsr &= ~( FE_ALL_EXCEPT << 7 );
      fenv.__mxcsr |= newcw << 7;
      (void) fesetenv( &fenv );
   } /* macOS on Intel */
#elif defined( __linux__ ) && defined( __GLIBC__ ) // LLM-generated; OH checked
   {
      curMask = GetExceptionMask();

      // feenableexcept/fedisableexcept update both the x87 control word and MXCSR on x86-64, the FPCR on aarch64
      // Note that trapping is optional on aarch64: on CPUs without support, feenableexcept fails and has no effect
      int enabled = 0;
      if( !ISINMASK( exInvalidOp ) )  enabled |= FE_INVALID;
#if defined( FE_DENORMAL )
      if( !ISINMASK( exDenormalized ) ) enabled |= FE_DENORMAL;
#endif
      if( !ISINMASK( exZeroDivide ) ) enabled |= FE_DIVBYZERO;
      if( !ISINMASK( exOverflow ) )   enabled |= FE_OVERFLOW;
      if( !ISINMASK( exUnderflow ) )  enabled |= FE_UNDERFLOW;
      if( !ISINMASK( exPrecision ) )  enabled |= FE_INEXACT;
      (void) fedisableexcept( FE_ALL_EXCEPT & ~enabled );
      (void) feenableexcept( enabled );
   }
#elif defined( __linux__ ) && defined( __x86_64__ ) // LLM-generated; OH checked
   {
      curMask = GetExceptionMask();

      // non-glibc libc (e.g. musl) without feenableexcept: update MXCSR for SSE and the x87 control word
      // for long double. A bit set means that the exception is masked
      unsigned int masked = 0;
      if( ISINMASK( exInvalidOp ) )    masked |= mxcsrInvalid;
      if( ISINMASK( exDenormalized ) ) masked |= mxcsrDenormal;
      if( ISINMASK( exZeroDivide ) )   masked |= mxcsrZeroDivide;
      if( ISINMASK( exOverflow ) )     masked |= mxcsrOverflow;
      if( ISINMASK( exUnderflow ) )    masked |= mxcsrUnderflow;
      if( ISINMASK( exPrecision ) )    masked |= mxcsrPrecision;
      _mm_setcsr( ( _mm_getcsr() & ~mxcsrManaged ) | masked );

      // the x87 control word has the same layout, shifted by 7 bits
      unsigned short cw;
      __asm__ __volatile__( "fnstcw %0" : "=m"( cw ) );
      cw = static_cast<unsigned short>( ( cw & ~( mxcsrManaged >> 7 ) ) | ( masked >> 7 ) );
      __asm__ __volatile__( "fldcw %0" : : "m"( cw ) );
   }
#elif defined( __linux__ ) && defined( __aarch64__ ) // LLM-generated; OH checked;
   {
      curMask = GetExceptionMask();

      // non-glibc libc (e.g. musl) without feenableexcept: update FPCR. A bit set means that the exception traps
      // Note that trapping is optional on aarch64: on CPUs without support, these bits are ignored (read as zero)
      uint64_t enabled = 0;
      if( !ISINMASK( exInvalidOp ) )    enabled |= fpcrInvalid;
      if( !ISINMASK( exDenormalized ) ) enabled |= fpcrDenormal;
      if( !ISINMASK( exZeroDivide ) )   enabled |= fpcrZeroDivide;
      if( !ISINMASK( exOverflow ) )     enabled |= fpcrOverflow;
      if( !ISINMASK( exUnderflow ) )    enabled |= fpcrUnderflow;
      if( !ISINMASK( exPrecision ) )    enabled |= fpcrPrecision;
      writeFPCR( ( readFPCR() & ~fpcrManaged ) | enabled );
   }
#else
#error "function SetExceptionMask not implemented for this OS or compiler" is_not_implemented;
   // ...
#endif

   return curMask;
}

void ClearExceptions()
{
   std::feclearexcept( FE_ALL_EXCEPT );
}

}// namespace rtl::math_p3
