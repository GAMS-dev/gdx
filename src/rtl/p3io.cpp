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
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <array>

#include "p3io.h"
#include "dtoaLoc.h"

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

static inline char tolower( const char c )
{
   return c >= 'A' && c <= 'Z' ? static_cast<char>( c ^ 32 ) : c;
}

void P3_Val_dd(const char *s, double *d, int *code)
{
   const auto len {std::strlen(s)};
   P3_Val_dd(s, len, d, code);
}

void P3_Val_dd(const char *s, size_t slen, double *d, int *code)
{
   std::array<char, 256> buffer;
   std::memcpy(buffer.data(), s, sizeof(char)*(slen+1));

   // skip over blanks
   // - Kylix 3 does not treat any other chars as whitespace
   char *s2;
   for (s2 = buffer.data(); ' ' == *s2;  s2++);
   char *sd;
   int sign {1};
   if ('+' == *s2)
      sd = s2+1;
   else if ('-' == *s2) {
      sign = -1;
      sd = s2+1;
   }
   else
      sd = s2;

   /* guard against some special cases where strtod
   * doesn't do the right thing for val():
   *   the decimal string in front of the decimal exponent
   *      must be nonempty for strtod, not so for val
   *   hex input, starts with 0x or 0X
   *   nan or inf (case insensitive)
   */
   char *end;
   int locErrno;
   if (*sd >= '0' && *sd <= '9') {
      if ('x' == tolower(sd[1])) {
         end = sd + 1;
         *code = (int)(end - buffer.data() + 1);
         *d = (*sd - '0');
         return;
      }
      *d = strtodLoc (s2, &end, &locErrno);
      if ('\0' == *end) // reached the end, things went OK
         *code = 0;
      else
         *code = (int)(end - buffer.data() + 1);
   } // if digit after space and sign char
   else if ('.' == *sd) {
      if ('\0' == sd[1]) {
         // corner case of valid input not handled by strtod
         *code = 0;
         *d = 0;
      }
      else {
         if ('e' == tolower(sd[1]))
            *sd = '0';
         *d = strtodLoc (sd, &end, &locErrno);
         *d *= sign;
         // reached the end, things went OK
         if ('\0' == *end)
            *code = 0;
         else {
            if (end <= sd)
               end = sd + 1;
            *code = (int)(end - buffer.data() + 1);
         }
      }
   }
   else { // not a digit, not a '.'
      *d = 0;
      *code = (int)(sd - buffer.data() + 1);
   }
}

void P3_Val_i(const char *s, int *i, int *code)
{
   const auto len {std::strlen(s)};
   P3_Val_i(s, len, i, code);
}

/* valid strings look like: [+|-][0x|$]d+,
 * where d is a decimal digit unless preceded by the 0x or $,
 * in which case it is a hex digit
 */
void P3_Val_i(const char *s, size_t slen, int *i, int *code)
{
   std::array<char, 256> buffer;
   char *end, *s2, *sd;
   long int li;
   int sign = 1;

   std::memcpy(buffer.data(), s, sizeof(char)*(slen+1));

   /* skip over blanks
   * - Kylix 3 does not treat any other chars as whitespace
   */
   for (s2 = (char *)buffer.data();  ' ' == *s2;  s2++);
   if ('+' == *s2) {
      sd = s2+1;
   }
   else if ('-' == *s2) {
      sign = -1;
      sd = s2+1;
   }
   else
      sd = s2;

   /* first check for the usual case - decimal digits */
   if (((*sd > '0') && (*sd <= '9')) ||
       (('0' == *sd) && ('\0' == sd[1] || (sd[1] >= '0' && sd[1] <= '9') )) ) {
      li = strtol((char *)s2, (char **)&end, 10);
      *i = li;
      if ('\0' == *end) {         /* reached the end, things went OK */
         *code = 0;
      }
      else
         *code = (int)(end - (char *)buffer.data() + 1);
      return;
   }

   /* if not a decimal string,
   * must be either $ffff or 0xffff or an error */
   if ('$' == *sd) {
      if ((sd[1] >= '0' && sd[1] <= '9') || (sd[1] >= 'A' && sd[1] <= 'F')) { // isxdigit
         if (-1 == sign)
            *sd = '-';
         else
            sd++;
         li = strtol((char *)sd, (char **)&end, 16);
         *i = li;
         if ('\0' == *end) {               /* reached the end, things went OK */
            *code = 0;
         }
         else
            *code = (int)(end - (char *)buffer.data() + 1);
      }
      else {
         *i = 0;
         sd++;
         *code = (int)(sd - (char *)buffer.data() + 1);
      }
      return;
   }
   else if (('0' == *sd) &&
            (('x' == sd[1]) || ('X' == sd[1]))
   ) {
      li = strtol((char *)s2, (char **)&end, 16);
      *i = li;
      if ('\0' == *end) {         /* reached the end, things went OK */
         *code = 0;
      }
      else {
         /* we alread read the 0x, that is not an error for val */
         if (end < sd+2)
            end = sd+2;
         *code = (int)(end - (char *)buffer.data() + 1);
      }
      return;
   }
   else {
      *i = 0;
      *code = (int)(sd - (char *)buffer.data() + 1);
   }
}

}// namespace rtl::p3io
