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


#include "dblutil.h"
#include "../rtl/math_p3.h"
#include <cmath>

namespace gdlib::dblutil
{

double gdRoundTo( const double x, const int i )
{
   if( !i )
      return static_cast<int>( x + 0.5 * ( x > 0 ? 1 : -1 ) );
   // use positive power of 10 to avoid roundoff error in z
   const double z { rtl::math_p3::IntPower( 10, std::abs( i ) ) };
   const double zReciprocal = i > 0 ? z : 1.0 / z;
   return std::trunc( x * zReciprocal + 0.5 * ( x > 0.0 ? 1.0 : -1.0 ) ) / zReciprocal;
}

}// namespace gdlib::dblutil