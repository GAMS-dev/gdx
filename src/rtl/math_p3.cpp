/*
* GAMS - General Algebraic Modeling System GDX API
 *
 * Copyright (c) 2017-2024 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2024 GAMS Development Corp. <support@gams.com>
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

#include "math_p3.h"
#include <cstdlib>              // for abs
#include <cmath>                // for log1p
#include "../global/modhead.h"  // for STUBWARN

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
   // ...
   STUBWARN();
   return {};
}

TFPUExceptionMask SetExceptionMask( const TFPUExceptionMask &Mask )
{
   STUBWARN();
   // ...
   return {};
}

void SetExceptionMask2P3()
{
   SetExceptionMask( { exDenormalized, exUnderflow, exPrecision, exInvalidOp, exZeroDivide, exOverflow } );
}

void ClearExceptions()
{
   STUBWARN();
   // ...
}

}// namespace rtl::math_p3