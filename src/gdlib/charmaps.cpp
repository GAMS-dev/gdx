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


#include "charmaps.h"
#include "utils.h"
#include <limits>

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace gdlib::charmaps
{

utils::charset digit, letter, alphanum, capletter, lowletter, identchar, labelchar, textquote, setcomch;

char quotecharx {};

std::array<char, numCharVals> mapcharBuf;

void InitChars( const bool AllChars )
{
   static_assert( mapcharBuf.size() == 256 );

   if( AllChars )
   {
      for( unsigned char c = std::numeric_limits<unsigned char>::min(); c < std::numeric_limits<unsigned char>::max(); c++ )
         mapcharBuf[c] = static_cast<char>(c);
      return;
   }

   // everything not explicitly whitelisted afterward kills the compilation (or is it ignored?)?
   for( unsigned char c = std::numeric_limits<unsigned char>::min(); c <= std::numeric_limits<unsigned char>::max(); c++ )
      mapcharBuf[c] = std::char_traits<char>::eof();
}

void InitCharacterMaps()
{
   // set other characeter arrays useful in compiler and execution
   // make sets empty first
   const utils::charset Empty;
   capletter = textquote = digit = lowletter = letter = identchar = Empty;
   utils::charRangeInsert( digit, '0', '9' );

   utils::charRangeInsert( letter, 'A', 'Z' );
   utils::charRangeInsert( capletter, 'A', 'Z' );

   utils::charRangeInsert( letter, 'a', 'z' );
   utils::charRangeInsert( lowletter, 'a', 'z' );

   identchar = utils::unionOp( letter, digit );
   alphanum = identchar;
   // set ident character array
   identchar.insert( '_' );

   // set label character array - unquoted characters
   labelchar = identchar;

   utils::insertAllChars( labelchar, "+-" );
   utils::insertAllChars( textquote, "\"\'" );
}

// Brief:
//  Determine the quote for a string
// Arguments:
//  S: Input string
// Returns:
//  The quote character for this string
char DetermineQuote( const char *s )
{
   if( s[0] == '\0' ) return quotecharx;

   bool quoted { !utils::in( s[0], alphanum ) };
   char quote { ' ' };

   for( int i {}; s[i] != '\0'; i++ )
   {
      char sk = s[i];
      // don't need quotes
      if( !utils::in( sk, labelchar ) )
      {
         quoted = true;
         if( sk == '\'' )
         {
            if( quote != '\'' ) quote = '\"';
         }
         else
         {
            if( sk == '\"' && quote != '\"' )
               quote = '\'';
         }
      }
   }

   return quoted && quote == ' ' ? quotecharx : quote;
}

}// namespace gdlib::charmaps
