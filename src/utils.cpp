/*
 * GAMS - General Algebraic Modeling System GDX API
 *
 * Copyright (c) 2017-2023 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2023 GAMS Development Corp. <support@gams.com>
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

#include "utils.h"
#include <algorithm>// for transform
#include <cstdint>  // for uint8_t
#include <cstring>  // for memcpy, strlen, size_t
#include <limits>   // for numeric_limits

using namespace std::literals::string_literals;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace gdx::utils
{

std::string uppercase( const std::string_view s )
{
   std::string out{ s };
   std::transform( s.begin(), s.end(), out.begin(), utils::toupper );
   return out;
}

bool sameTextInvariant( const std::string_view a,
                        const std::string_view b )
{
   const auto l = a.length();
   if( b.length() != a.length() ) return false;
   for( size_t i{}; i < l; i++ )
   {
      if( utils::tolower( a[i] ) != utils::tolower( b[i] ) )
         return false;
   }
   return true;
}

std::string_view trim( const std::string_view s )
{
   if( s.empty() ) return {};
   int firstNonBlank{ -1 }, lastNonBlank{};
   for( int i{}; i < static_cast<int>( s.length() ); i++ )
   {
      if( (unsigned char) s[i] > 32 )
      {
         if( firstNonBlank == -1 ) firstNonBlank = i;
         lastNonBlank = i;
      }
   }
   if( firstNonBlank == -1 ) return {};
   return s.substr( firstNonBlank, ( lastNonBlank - firstNonBlank ) + 1 );
}

const char *trimRight( const char *s,
                       char *storage,
                       int &slen )
{
   int i;
   slen = -1;// all whitespace? => slen=0!
   for( i = 0; s[i] != '\0'; i++ )
      if( static_cast<unsigned char>( s[i] ) > 32 )
         slen = i;
   if( ++slen == i ) return s;
   std::memcpy( storage, s, slen );
   storage[slen] = '\0';
   return storage;
}

std::string quoteWhitespace( const std::string &s,
                             char quotechar )
{
   return s.find( ' ' ) != std::string::npos ? ""s + quotechar + s + quotechar : s;
}

int strCompare( const char *S1,
                const char *S2,
                bool caseInsensitive )
{
   if( S1[0] == '\0' || S2[0] == '\0' )
      return ( S1[0] != '\0' ? 1 : 0 ) - ( S2[0] != '\0' ? 1 : 0 );
   size_t K;
   for( K = 0; S1[K] != '\0' && S2[K] != '\0'; K++ )
   {
      int c1 = static_cast<unsigned char>( caseInsensitive ? utils::toupper( S1[K] ) : S1[K] );
      int c2 = static_cast<unsigned char>( caseInsensitive ? utils::toupper( S2[K] ) : S2[K] );
      int d = c1 - c2;
      if( d ) return d;
   }
   return static_cast<int>( std::strlen( S1 ) - std::strlen( S2 ) );
}

/**
     * PORTING NOTES FROM ANDRE
     * Pascal/Delphi convention: 1 byte is size/length/charcount, then character bytes, then quote byte END
     * C/C++ convention here: raw character bytes, null terminator \0, quote char after that
     * Doing the quote char after the terminator keeps strlen etc working
     **/

// Convert C++ standard library string to Delphi short string
int strConvCppToDelphi( const std::string_view s,
                        char *delphistr )
{
   if( s.length() > std::numeric_limits<uint8_t>::max() )
   {
      const std::string errorMessage{ "Error: Maximum short string length is 255 characters!"s };
      strConvCppToDelphi( errorMessage, delphistr );
      return static_cast<int>( errorMessage.length() );
   }
   const auto l = static_cast<uint8_t>( s.length() );
   delphistr[0] = static_cast<char>( l );
   std::memcpy( &delphistr[1], s.data(), l );
   return 0;
}
}// namespace gdx::utils
