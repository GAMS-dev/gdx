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

#include "../rtl/math_p3.h"
#include "../rtl/sysutils_p3.h"

#include "dblutil.h"
#include "utils.h"

#include <cmath>

using namespace std::literals::string_literals;

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

constexpr TI64Rec t64 { 1 };
const bool bigEndian { t64.bytes.at( 7 ) == 1 };

constexpr int64_t signMask { static_cast<int64_t>( 0x80000000 ) << 32 },
        expoMask { static_cast<int64_t>( 0x7ff00000 ) << 32 },
        mantMask { ~( signMask | expoMask ) };


static void dblDecomp( const double x, bool &isNeg, uint32_t &expo, int64_t &mant )
{
   TI64Rec xi { x };
   isNeg = ( xi.i64 & signMask ) == signMask;
   expo = ( xi.i64 & expoMask ) >> 52;
   mant = xi.i64 & mantMask;
}

char hexDigit( const uint8_t b )
{
   return static_cast<char>( b < 10 ? utils::ord( '0' ) + b : utils::ord( 'a' ) + b - 10 );
}

std::string dblToStrHex( const double x )
{
   TI64Rec xi { x };
   uint8_t c;
   std::string result = "0x";

   if( bigEndian )
   {
      for( int i {}; i < 8; i++ )
      {
         c = xi.bytes.at( i );
         result += gdlib::dblutil::hexDigit( c / 16 );
         result += gdlib::dblutil::hexDigit( c & 0x0F );
      }
   }
   else
   {
      for( int i { 7 }; i >= 0; i-- )
      {
         c = xi.bytes.at( i );
         result += gdlib::dblutil::hexDigit( c / 16 );
         result += gdlib::dblutil::hexDigit( c & 0x0F );
      }
   }
   return result;
}

// format the bytes in the mantissa
static std::string mFormat( int64_t m )
{
   if( !m )
      return "0";
   // TI64Rec xi { m };
   int64_t mask { static_cast<int64_t>( 0x000f0000 ) << 32 };
   int shiftCount = 48;
   std::string res;
   while( m )
   {
      const int64_t m2 { ( m & mask ) >> shiftCount };
      const auto b { static_cast<uint8_t>( m2 ) };
      res += hexDigit( b );
      m &= ~mask;
      mask >>= 4;
      shiftCount -= 4;
   }
   return res;
}

std::string dblToStrHexponential( const double x )
{
   bool isNeg;
   uint32_t expo;
   int64_t mant;
   dblDecomp( x, isNeg, expo, mant );
   std::string result;
   // Consider all 10 cases: SNaN, QNaN, and +/-[INF,denormal,zero,normal]
   if( isNeg )
      result += '-';
   if( !expo )
   {
      if( !mant ) // zero
         result += "0x0.0p0"s;
      else // denorm
         result += "0x0."s + mFormat( mant ) + "p-1022"s;
   }
   // not all ones
   else if( expo < 2047 )
      // normalized double
         result += "0x1."s + mFormat( mant ) + 'p' + rtl::sysutils_p3::IntToStr( static_cast<int64_t>( expo ) - 1023 );
   // exponent all ones
   else
   {
      // infinity
      if( !mant )
         result += "Infinity"s;
      else // NaN
         result = "NaN"s;
   }
   return result;
}


}// namespace gdlib::dblutil
