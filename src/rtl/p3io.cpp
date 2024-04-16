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

#include <cstring>
#include "p3io.h"
#include "dtoaLoc.h"
#include <cassert>
#include <cstdio>
#include <array>

namespace rtl::p3io
{

/* convert base-10 digits and implied decimal position into E-format string
 * we assume buf is long enough for the requested width
 * Possible output formats for width=23,decimals=15:
 *    12345678901234567890123
 *   '   d.ddddddddddddddEsdd'
 *   '  d.ddddddddddddddEsddd'
 *   '  -d.ddddddddddddddEsdd'
 *   ' -d.ddddddddddddddEsddd'
 * But Delphi always does this for width=23,decimals=15:
 *   ' d.ddddddddddddddEs0ddd'
 *   '-d.ddddddddddddddEs0ddd'
 */
void dig2Exp( const char *dig, size_t digLen, int decPos, int isNeg, int width, int decimals, char *buf, size_t *bufLen )
{
   assert( digLen >= 1 );
   assert( digLen <= 18 );

   int e = decPos - 1;

   char *d = buf;
   // any width > 26 is just more blanks
   for( int k = 26; k < width; k++ )
      *d++ = ' ';

   if( isNeg )
      *d++ = '-';
   else
      *d++ = ' ';
   const char *s = dig;
   *d++ = *s++;
   *d++ = '.';
   while( *s )
      *d++ = *s++;
   // zero-fill as necessary
   for( int k {}; k < decimals - (int) digLen; k++ )
      *d++ = '0';
   *d++ = 'E';

   if( e < 0 )
   {
      e *= -1;
      *d++ = '-';
   }
   else
      *d++ = '+';
   *bufLen = d - buf;

   std::snprintf( d, 255, "%04d", e );
   *bufLen += 4;
}

void padLeftC2P( const char *eBuf, size_t eLen, int width, char *s, uint8_t sMax )
{
   size_t nPad { static_cast<size_t>(width) - eLen };
   if( nPad >= sMax )
      static_cast<void>(std::memset( s, ' ', sMax ));
   else
   {
      char *dst;
      if( nPad > 0 )
      {
         (void) std::memset( s, ' ', nPad );
         dst = s + nPad;
      }
      else
      {
         nPad = 0;
         dst = s;
      }
      size_t k { sMax - nPad };
      if( k > eLen )
         k = eLen;
      std::memcpy( dst, eBuf, k );
   }
}

void P3_Str_dd0( const double x, char *s, const uint8_t sMax, size_t *eLen )
{
   int decPos, isNeg;
   std::array<char, 32> dBuf;
   char *pEnd;
   const char *p { dtoaLoc( x, 2, 15, dBuf.data(), sizeof( char ) * 32, &decPos, &isNeg, &pEnd ) };
   if( decPos < 999 )
      dig2Exp( p, pEnd - p, decPos, isNeg, 23, 15, s, eLen );
   else
   {
      // inf or NaN: take string as returned
      dBuf[10] = '\0';// just in case, but should never be necessary
      padLeftC2P( dBuf.data(), std::strlen( dBuf.data() ), 23, s, sMax );
   }
}

}// namespace rtl::p3io
