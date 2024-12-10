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

#include "gmsspecs.hpp"

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace global::gmsspecs
{
std::array<int, styequb + 1> equstypInfo;

tgmsvalue mapval( double x )
{
   if( x < valund ) return xvreal;
   if( x >= valacr ) return xvacr;
   x /= valund;
   const int k = utils::round<int>( x );
   if( std::abs( k - x ) > 1.0e-5 )
      return xvund;

   constexpr std::array<tgmsvalue, 5> kToRetMapping = {
           xvund, xvna, xvpin, xvmin, xveps };
   return k >= 1 && k <= (int)kToRetMapping.size() ? kToRetMapping[k - 1] : xvacr;
}

txgmsvalue xmapval( double x )
{
   if( x < valund )
   {
      if( x < 0 ) return vneg;
      else if( x == 0.0 )
         return vzero;// epsilon check?
      return vpos;
   }
   return txgmsvalue( (int) mapval( x ) + 2 );
}
}// namespace global::gmsspecs
