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
#include <cstdlib>              // for abs
#include <cmath>                // for log1p
#include <stdexcept>

namespace rtl::math_p3
{

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
#if defined(_WIN32)
   std::set<rtl::math_p3::TFPUException> result {};
   unsigned int cw = _control87( 0, 0 );
   if( cw & _EM_INVALID ) result.insert( exInvalidOp );
   if( cw & _EM_DENORMAL ) result.insert( exDenormalized );
   if( cw & _EM_ZERODIVIDE ) result.insert( exZeroDivide );
   if( cw & _EM_OVERFLOW ) result.insert( exOverflow );
   if( cw & _EM_UNDERFLOW ) result.insert( exUnderflow );
   if( cw & _EM_INEXACT ) result.insert( exPrecision );
   return result;
#else
   // ...
   throw std::runtime_error("Not implemented yet!");
#endif
}

TFPUExceptionMask SetExceptionMask( const TFPUExceptionMask &Mask )
{
#if defined(_WIN32)
   unsigned int cw = _control87( 0, 0 );
   std::set<rtl::math_p3::TFPUException> result {};
   if( cw & _EM_INVALID ) result.insert( exInvalidOp );
   if( cw & _EM_DENORMAL ) result.insert( exDenormalized );
   if( cw & _EM_ZERODIVIDE ) result.insert( exZeroDivide );
   if( cw & _EM_OVERFLOW ) result.insert( exOverflow );
   if( cw & _EM_UNDERFLOW ) result.insert( exUnderflow );
   if( cw & _EM_INEXACT ) result.insert( exPrecision );
   unsigned int tcw {};
   if( result.count(exInvalidOp) ) tcw |= _EM_INVALID;
   if( result.count( exDenormalized ) ) tcw |= _EM_DENORMAL;
   if( result.count( exZeroDivide ) ) tcw |= _EM_ZERODIVIDE;
   if( result.count( exOverflow ) ) tcw |= _EM_OVERFLOW;
   if( result.count( exUnderflow ) ) tcw |= _EM_UNDERFLOW;
   if( result.count( exPrecision ) ) tcw |= _EM_INEXACT;
   _control87( tcw, _MCW_EM );
   return result;
#else
   throw std::runtime_error("Not implemented yet!");
   // ...
#endif
}

void SetExceptionMask2P3()
{
   SetExceptionMask( { exDenormalized, exUnderflow, exPrecision, exInvalidOp, exZeroDivide, exOverflow } );
}

void ClearExceptions()
{
   throw std::runtime_error("Not implemented yet!");
   // ...
}

}// namespace rtl::math_p3