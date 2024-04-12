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

#pragma once

#include <string>
#include <unordered_map>
#include <set>
#include <array>
#include <limits>
#include <cassert>


// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace gdlib::charmaps
{

const char charff = '\f',
           charcr = '\r',
           chareof = std::char_traits<char>::eof(),
           chareol = '\0',
           charlf = '\n',
           chartab = '\t',
           char200 = static_cast<char>( 200 );

extern std::set<char> digit,
        letter,
        alphanum,
        capletter,
        lowletter,
        identchar,
        labelchar,
        textquote,
        setcomch;

extern char quotecharx;

constexpr int numCharVals {std::numeric_limits<unsigned char>::max()+1};
extern std::array<char, numCharVals> mapcharBuf;

inline char mapchar(char c) {
   assert( (unsigned char) c >= 0 && (unsigned char) c <= 255 );
   return mapcharBuf[(unsigned char)c];
}

void InitChars( bool AllChars );
void InitCharacterMaps();
char DetermineQuote( const char *s );

}// namespace gdlib::charmaps