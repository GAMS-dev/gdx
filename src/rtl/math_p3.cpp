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

namespace rtl::math_p3
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

double LnXP1( double x )
{
   return log1p( x );
}

TFPUExceptionMask GetExceptionMask()
{
   std::set<TFPUException> result {};
   auto ADD2MASK = [&result]( TFPUException e ) {
      result.insert( e );
   };
#if defined( _WIN32 )
   {
      unsigned int cw = _control87( 0, 0 );
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
      cw = fenv.__control & FE_ALL_EXCEPT;
      if( cw & FE_INVALID ) ADD2MASK( exInvalidOp );
#if defined( FE_DENORMAL )
      if( cw & FE_DENORMAL ) ADD2MASK( exDenormalized );
#else// assume always on if FE_DENORMAL not defined
      ADD2MASK( exDenormalized );
#endif
      if( cw & FE_DIVBYZERO ) ADD2MASK( exZeroDivide );
      if( cw & FE_OVERFLOW ) ADD2MASK( exOverflow );
      if( cw & FE_UNDERFLOW ) ADD2MASK( exUnderflow );
      if( cw & FE_INEXACT ) ADD2MASK( exPrecision );
   }
#elif defined( __linux__ ) && defined( __aarch64__ )
   {
      fenv_t fenv;
      unsigned long long cw;

      /* on AARCH64, __control_world is replaced by __fpcr and bits are shifted by FE_EXCEPT_SHIFT */
      (void) fegetenv( &fenv );
      cw = ( fenv.__fpcr >> FE_EXCEPT_SHIFT ) & FE_ALL_EXCEPT;
      if( ( cw & FE_INVALID ) ) ADD2MASK( exInvalidOp );
#if defined( FE_DENORMAL ) /* not present on almalinux8/aarch64 */
      if( ( cw & FE_DENORMAL ) ) ADD2MASK( exDenormalized );
#else                      // assume always on if FE_DENORMAL not defined
      ADD2MASK( exDenormalized );
#endif
      if( ( cw & FE_DIVBYZERO ) ) ADD2MASK( exZeroDivide );
      if( ( cw & FE_OVERFLOW ) ) ADD2MASK( exOverflow );
      if( ( cw & FE_UNDERFLOW ) ) ADD2MASK( exUnderflow );
      if( ( cw & FE_INEXACT ) ) ADD2MASK( exPrecision );
   }
#elif defined( __linux )
   {
      std::fenv_t fenv;
      (void) fegetenv( &fenv );
      fesetenv( &fenv );
      unsigned short ex = fenv.__control_word & FE_ALL_EXCEPT;
      if( ex & FE_INVALID ) ADD2MASK( exInvalidOp );
#if defined( FE_DENORMAL )
      if( ex & FE_DENORMAL ) ADD2MASK( exDenormalized );
#else
      // assume always on if FE_DENORMAL not defined
      ADD2MASK( exDenormalized );
#endif
      if( ex & FE_DIVBYZERO ) ADD2MASK( exZeroDivide );
      if( ex & FE_OVERFLOW ) ADD2MASK( exOverflow );
      if( ex & FE_UNDERFLOW ) ADD2MASK( exUnderflow );
      if( ex & FE_INEXACT ) ADD2MASK( exPrecision );
   }
#else
// ...
#error "Function GetExceptionMask not implemented for this OS or compiler" is_not_implemented;
#endif
   return result;
}

TFPUExceptionMask SetExceptionMask( const TFPUExceptionMask &Mask )
{
   std::set<TFPUException> result {};
   auto ADD2MASK = [&result]( TFPUException e ) {
      result.insert( e );
   };
   auto ISINMASK = [&result]( TFPUException e ) {
      return result.count( e );
   };
#if defined( _WIN32 )
   {
      unsigned int cw = _control87( 0, 0 );
      if( cw & _EM_INVALID ) ADD2MASK( exInvalidOp );
      if( cw & _EM_DENORMAL ) ADD2MASK( exDenormalized );
      if( cw & _EM_ZERODIVIDE ) ADD2MASK( exZeroDivide );
      if( cw & _EM_OVERFLOW ) ADD2MASK( exOverflow );
      if( cw & _EM_UNDERFLOW ) ADD2MASK( exUnderflow );
      if( cw & _EM_INEXACT ) ADD2MASK( exPrecision );
      unsigned int tcw {};
      if( ISINMASK( exInvalidOp ) ) tcw |= _EM_INVALID;
      if( ISINMASK( exDenormalized ) ) tcw |= _EM_DENORMAL;
      if( ISINMASK( exZeroDivide ) ) tcw |= _EM_ZERODIVIDE;
      if( ISINMASK( exOverflow ) ) tcw |= _EM_OVERFLOW;
      if( ISINMASK( exUnderflow ) ) tcw |= _EM_UNDERFLOW;
      if( ISINMASK( exPrecision ) ) tcw |= _EM_INEXACT;
      _control87( tcw, _MCW_EM );
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
      oldcw = fenv.__control & FE_ALL_EXCEPT;
      if( oldcw & FE_INVALID ) ADD2MASK( exInvalidOp );
#if defined( FE_DENORMAL )
      if( oldcw & FE_DENORMAL ) ADD2MASK( exDenormalized );
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
#endif
      // all of this respects bits that must stay set

      if( ISINMASK( exInvalidOp ) ) newcw |= FE_INVALID;
#if defined( FE_DENORMAL )
      if( ISINMASK( exDenormalized ) ) newcw |= FE_DENORMAL;
#endif
      if( ISINMASK( exZeroDivide ) ) newcw |= FE_DIVBYZERO;
      if( ISINMASK( exOverflow ) ) newcw |= FE_OVERFLOW;
      if( ISINMASK( exUnderflow ) ) newcw |= FE_UNDERFLOW;
      if( ISINMASK( exPrecision ) ) newcw |= FE_INEXACT;
      fenv.__control &= ~FE_ALL_EXCEPT;
      fenv.__control |= newcw;
      (void) fesetenv( &fenv );
   } /* macOS on Intel */
#elif defined( __linux__ ) && defined( __aarch64__ )
   {
      fenv_t fenv;
      unsigned long long oldcw, newcw;

      /* on AARCH64, __control_world is replaced by __fpcr and bits are shifted by FE_EXCEPT_SHIFT
* however, trapping exceptions is optional, so that the below fesetenv() may have no effect
* on wally's CPU, this seems to be the case (or is it because it is a VM?)
*/
      (void) fegetenv( &fenv );
      oldcw = ( fenv.__fpcr >> FE_EXCEPT_SHIFT ) & FE_ALL_EXCEPT;
      if( ( oldcw & FE_INVALID ) ) ADD2MASK( exInvalidOp );
#if defined( FE_DENORMAL )
      if( ( oldcw & FE_DENORMAL ) ) ADD2MASK( exDenormalized );
#else// assume always on if FE_DENORMAL not defined
      ADD2MASK( exDenormalized );
#endif
      if( ( oldcw & FE_DIVBYZERO ) ) ADD2MASK( exZeroDivide );
      if( ( oldcw & FE_OVERFLOW ) ) ADD2MASK( exOverflow );
      if( ( oldcw & FE_UNDERFLOW ) ) ADD2MASK( exUnderflow );
      if( ( oldcw & FE_INEXACT ) ) ADD2MASK( exPrecision );

      newcw = 0;
      if( ISINMASK( exInvalidOp ) ) newcw |= FE_INVALID;
#if defined( FE_DENORMAL )
      if( ISINMASK( exDenormalized ) ) newcw |= FE_DENORMAL;
#endif
      if( ISINMASK( exZeroDivide ) ) newcw |= FE_DIVBYZERO;
      if( ISINMASK( exOverflow ) ) newcw |= FE_OVERFLOW;
      if( ISINMASK( exUnderflow ) ) newcw |= FE_UNDERFLOW;
      if( ISINMASK( exPrecision ) ) newcw |= FE_INEXACT;
      fenv.__fpcr &= ~( FE_ALL_EXCEPT << FE_EXCEPT_SHIFT );
      fenv.__fpcr |= newcw << FE_EXCEPT_SHIFT;
      (void) fesetenv( &fenv );
   }
#elif defined( __linux )
   {
      std::fenv_t fenv;
      (void) fegetenv( &fenv );
      unsigned short oldcw = fenv.__control_word & FE_ALL_EXCEPT;
      if( oldcw & FE_INVALID ) ADD2MASK( exInvalidOp );
#if defined( FE_DENORMAL )
      if( oldcw & FE_DENORMAL ) ADD2MASK( exDenormalized );
#else
      ADD2MASK( exDenormalized );
#endif
      if( oldcw & FE_DIVBYZERO ) ADD2MASK( exZeroDivide );
      if( oldcw & FE_OVERFLOW ) ADD2MASK( exOverflow );
      if( oldcw & FE_UNDERFLOW ) ADD2MASK( exUnderflow );
      if( oldcw & FE_INEXACT ) ADD2MASK( exPrecision );

      unsigned short newcw = 0;
      if( ISINMASK( exInvalidOp ) ) newcw |= FE_INVALID;
#if defined( FE_DENORMAL )
      if( ISINMASK( exdenormalized ) ) newcw |= FE_DENORMAL;
#endif
      if( ISINMASK( exZeroDivide ) ) newcw |= FE_DIVBYZERO;
      if( ISINMASK( exOverflow ) ) newcw |= FE_OVERFLOW;
      if( ISINMASK( exUnderflow ) ) newcw |= FE_UNDERFLOW;
      if( ISINMASK( exPrecision ) ) newcw |= FE_INEXACT;
      fenv.__control_word &= ~FE_ALL_EXCEPT;
      fenv.__control_word |= newcw;
      (void) fesetenv( &fenv );
   }
#else
#error "function SetExceptionMask not implemented for this OS or compiler" is_not_implemented;
   // ...
#endif
   return result;
}

void SetExceptionMask2P3()
{
   SetExceptionMask( { exDenormalized, exUnderflow, exPrecision, exInvalidOp, exZeroDivide, exOverflow } );
}

void ClearExceptions()
{
   throw std::runtime_error( "Not implemented yet!" );
   // ...
}

}// namespace rtl::math_p3