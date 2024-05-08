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

#include <array>                // for array
#include <cstdint>              // for uint8_t, int64_t
#include <string>               // for string, basic_string
#include <string_view>          // for string_view, basic_string_view
#include "utils.h"              // for charset

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace gdlib::strutilx
{
class DelphiStrRef {
public:
   uint8_t length;
   char *chars;

   [[nodiscard]] std::string str() const{
      return {chars, length};
   }

   [[nodiscard]] std::string_view sview() const {
      return { chars, length };
   }
};

std::string UpperCase( std::string_view s );
std::string LowerCase( std::string_view s );

std::string IntToNiceStrW( int64_t N, int Width );
std::string IntToNiceStr( int N );
std::string BlankStr( unsigned int Len );

// Excel column names
int StrExcelCol( const std::string &s );
std::string ExcelColStr( int C );

int IntegerWidth( int n );

int PadModLength( std::string_view s, int M );

std::string PadRightMod( std::string_view s, int M );

std::string DblToStrSep( double V, char DecimalSep );
uint8_t DblToStrSep( double V, char DecimalSep, char *s );

std::string DblToStr( double V );
uint8_t DblToStr( double V, char *s );

bool StrAsDoubleEx( const std::string &s, double &v );
bool StrAsIntEx2( const std::string &s, int &v );

bool StrAsIntEx( const std::string &s, int &v );
bool SpecialStrAsInt( const std::string &s, int &v );

bool StrUEqual( std::string_view S1, std::string_view S2 );
bool StrUEqual( const DelphiStrRef &S1, std::string_view S2 );

std::string IncludeTrailingPathDelimiterEx( const std::string &S );
std::string ExcludeTrailingPathDelimiterEx( const std::string &S );

std::string ExtractFileNameEx( const std::string &FileName );

std::string ExtractFilePathEx( const std::string &FileName );

std::string PadRight( const std::string &s, int W );
std::string PadLeft( const std::string &s, int W );

std::string ExtractToken( const std::string &s, int &p );

int StrAsInt( const std::string &s );

std::string ChangeFileExtEx( const std::string &FileName, const std::string &Extension );
std::string CompleteFileExtEx( const std::string &FileName, const std::string &Extension );
std::string ExtractFileExtEx( const std::string &FileName );

constexpr int maxBOMLen { 4 };
using tBomIndic = std::array<uint8_t, maxBOMLen>;
bool checkBOMOffset( const tBomIndic &potBOM, int &BOMOffset, std::string &msg );

std::string ReplaceChar( const utils::charset &ChSet, char New, const std::string &S );

std::string ReplaceStr( const std::string &substr, const std::string &replacement, const std::string &S );

std::string ExtractShortPathNameExcept( const std::string &FileName );

int strConvCtoDelphi( char *cstr );
void strConvDelphiToC( char *delphistr );
std::string strConvDelphiToCpp( const char *delphistr );
int strConvCppToDelphi( const std::string &s, char *delphistr );

bool PStrUEqual( std::string_view P1, std::string_view P2 );
int PStrUCmp( std::string_view P1, std::string_view P2 );
int StrUCmp( std::string_view S1, std::string_view S2 );
int StrUCmp( const DelphiStrRef &S1, const DelphiStrRef &S2 );
bool PStrEqual( std::string_view P1, std::string_view P2 );

}// namespace gdlib::strutilx
