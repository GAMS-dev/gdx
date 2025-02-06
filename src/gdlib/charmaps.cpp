/*
* GAMS - General Algebraic Modeling System GDX API
 *
 * Copyright (c) 2017-2025 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2025 GAMS Development Corp. <support@gams.com>
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


#include "charmaps.hpp"
#include "utils.hpp"
#include <limits>

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace gdlib::charmaps
{

utils::charset digit, letter, alphanum, capletter, lowletter, identchar, labelchar, textquote, setcomch;

char quotecharx {};

std::array<char, numCharVals> mapcharBuf;

constexpr std::array mapToSelf {
        ' ', // blank
        '!', // exclamation mark  !
        '"', // double quote      "
        '#', // number sign       #
        '$', // dollar            $
        '%', // per-cent          %
        '&', // ampersand         &
        '(', // left paren        (
        ')', // right paren       )
        '*', // asterisk          *
        '+', // plus              +
        ',', // comma             ,
        '-', // minus             -
        '.', // period            .
        '/', // slash             /
        ':', // colon             :
        ',', // semi-colon        ,
        '<', // less than         <
        '=', // equals            =
        '>', // greater than      >
        '?', // question mark     ?
        '@', // at sign           @
        '\\',// backslash
        '_', // underscore        _
        '\'',// single quote      '
        '[', // left sq bracket    [
        ']', // right sq bracket   ]
        '^', // circumflex         ^
        '|', // vert bar           |
        '`', // accent grave       `
        '}', // right brace
        '{', // left  brace
        '~', // tilde              ~
};

void InitChars( const bool AllChars )
{
   using uchar = unsigned char;
   static_assert( mapcharBuf.size() == 256 );

   if( AllChars )
   {
      for( unsigned char c = std::numeric_limits<uchar>::min(); c < std::numeric_limits<uchar>::max(); c++ )
         mapcharBuf[c] = static_cast<char>(c);
      return;
   }

   // everything not explicitly whitelisted afterward kills the compilation (or is it ignored?)?
   for( uchar c = std::numeric_limits<uchar>::min(); c <= std::numeric_limits<uchar>::max(); c++ )
      mapcharBuf[c] = chareof;

   for( char c { 'A' }; c <= 'Z'; c++ )
      if(letter[c])
         mapcharBuf[c] = c;

   for( char c { 'a' }; c <= 'z'; c++ )
      if(letter[c])
         mapcharBuf[c] = c;

   for( char c { '0' }; c <= '9'; c++ )
      if(digit[c])
         mapcharBuf[c] = c;

   for( const uchar c: mapToSelf )
   {
      assert( c <= std::numeric_limits<char>::max() );
      mapcharBuf[c] = static_cast<char>(c);
   }
}

void InitCharacterMaps()
{
   // set other character arrays useful in compiler and execution
   // make sets empty first
   const utils::charset Empty;
   capletter = textquote = digit = lowletter = letter = identchar = Empty;
   charRangeInsert( digit, '0', '9' );

   charRangeInsert( letter, 'A', 'Z' );
   charRangeInsert( capletter, 'A', 'Z' );

   charRangeInsert( letter, 'a', 'z' );
   charRangeInsert( lowletter, 'a', 'z' );

   identchar = unionOp( letter, digit );
   alphanum = identchar;
   // set ident character array
   identchar.insert( '_' );

   // set label character array - unquoted characters
   labelchar = identchar;

   insertAllChars( labelchar, "+-" );
   insertAllChars( textquote, "\"\'" );
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
      // don't need quotes
      if( const char sk = s[i]; !utils::in( sk, labelchar ) )
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
