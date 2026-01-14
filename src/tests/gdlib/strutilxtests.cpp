/*
* GAMS - General Algebraic Modeling System GDX API
*
* Copyright (c) 2017-2026 GAMS Software GmbH <support@gams.com>
* Copyright (c) 2017-2026 GAMS Development Corp. <support@gams.com>
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

#include "strutilx.hpp"
#include "utils.hpp"
#include "../doctest.hpp"

using namespace std::literals::string_literals;
using namespace gdlib::strutilx;
using namespace utils;

namespace tests::gdlibtests::strutilxtests
{

TEST_SUITE_BEGIN( "gdlib::strutilx" );

TEST_CASE( "Extract extension of filename ex-version" )
{
   REQUIRE_EQ( ".pdf", ExtractFileExtEx( "xyz.pdf" ) );
   REQUIRE( ExtractFileExtEx( "xyz" ).empty() );
}

TEST_CASE( "Width of integer in number of digits" )
{
   REQUIRE_EQ( 2, IntegerWidth( 23 ) );
   REQUIRE_EQ( 1, IntegerWidth( 0 ) );
   REQUIRE_EQ( 1, IntegerWidth( 1 ) );
   REQUIRE_EQ( 3, IntegerWidth( 100 ) );
   REQUIRE_EQ( 4, IntegerWidth( 2000 ) );
}

TEST_CASE( "Turn mixed case string into uppercase" )
{
   REQUIRE_EQ( "UPPERCASE", UpperCase( "UpperCase" ) );
   REQUIRE_EQ( "EIN  KLEINER TEST!", UpperCase( "ein  kleiner test!" ) );
}

TEST_CASE( "Turn mixed case string into lowercase" )
{
   REQUIRE_EQ( "lowercase", LowerCase( "LowerCase" ) );
   REQUIRE_EQ( "ein  kleiner test!", LowerCase( "EIN  KLEINER TEST!" ) );
}

TEST_CASE( "Convert integer number to 'nice' string representation with given width, leading blanks and thousand separators" )
{
   REQUIRE_EQ( "-23"s, IntToNiceStrW( -23, 3 ) );
   REQUIRE_EQ( "23"s, IntToNiceStrW( 23, 2 ) );
   REQUIRE_EQ( " 23"s, IntToNiceStrW( 23, 3 ) );
   REQUIRE_EQ( "1,000"s, IntToNiceStrW( 1000, 5 ) );
   REQUIRE_EQ( "-1,000"s, IntToNiceStrW( -1000, 6 ) );
   REQUIRE_EQ( " 1,000,000"s, IntToNiceStrW( 1000000, 10 ) );
   REQUIRE_EQ( " 1,234,567"s, IntToNiceStrW( 1234567, 10 ) );
   REQUIRE_EQ( "-1,234,567"s, IntToNiceStrW( -1234567, 10 ) );
   REQUIRE_EQ( "1,234,567"s, IntToNiceStrW( 1234567, 9 ) );
   REQUIRE_EQ( "1,234,567"s, IntToNiceStrW( 1234567, 0 ) );
   REQUIRE_EQ( "1,234,567"s, IntToNiceStrW( 1234567, 3 ) );
   REQUIRE_EQ( "1,234,567"s, IntToNiceStrW( 1234567, 8 ) );
}

TEST_CASE( "Convert double to string" )
{
   REQUIRE_EQ( "23", DblToStr( 23 ) );
   REQUIRE_EQ( "-23", DblToStr( -23 ) );
   REQUIRE_EQ( "0", DblToStr( 0 ) );
   REQUIRE_EQ( "3.1415926", DblToStr( 3.1415926 ) );
   REQUIRE_EQ( "0.141592653589793", DblToStr( 0.14159265358979312 ) );

   REQUIRE_EQ( "23", DblToStrSep( 23, '.' ) );
   REQUIRE_EQ( "0", DblToStrSep( 0, '.' ) );
   REQUIRE_EQ( "3.1415926", DblToStrSep( 3.1415926, '.' ) );
   REQUIRE_EQ( "0.141592653589793", DblToStrSep( 0.14159265358979312, '.' ) );

   sstring buf{};
   DblToStr(23, buf.data());
   REQUIRE(!std::strcmp(buf.data(), "23"));

   // Edge cases
   REQUIRE_EQ("1E15", DblToStr(1e15));
   REQUIRE_EQ("1E-10", DblToStr(1e-10));
   REQUIRE_EQ("1E15", DblToStr(1e15+1));
   REQUIRE_EQ("0.000101", DblToStr(1.01e-4));
}

TEST_CASE( "Test generating blank string of specified length" )
{
   REQUIRE( BlankStr( 0 ).empty() );
   REQUIRE_EQ( "    ", BlankStr( 4 ) );
}

TEST_CASE( "Testing extended string to double conversion" )
{
   double v;
   StrAsDoubleEx( "3.141", v );
   REQUIRE_EQ( 3.141, v );
}

TEST_CASE( "Test extended string to integer conversion" )
{
   int v;
   StrAsIntEx( "3", v );
   REQUIRE_EQ( 3, v );
}

// FIXME: Isn't this the same as utils::sameText?
TEST_CASE( "Test case insensitive string equality comparison" )
{
   REQUIRE( StrUEqual( "aBc", "AbC" ) );
   REQUIRE( StrUEqual( "abc", "abc" ) );
   REQUIRE_FALSE( StrUEqual( "_abc", "abc" ) );
}

// TODO: Add tests for parse number to make it quicker (also port more closely to Delphi)

// See: https://docwiki.embarcadero.com/Libraries/Sydney/en/System.ShortString

TEST_CASE( "Test converting char buffer string with C-style layout (PChar) to a Delphi short string layout" )
{
   // Easy case: three-letter word "yes"
   std::array<char, 4> buf {};
   buf[0] = 'y';
   buf[1] = 'e';
   buf[2] = 's';
   buf[3] = '\0';
   strConvCtoDelphi( buf.data() );
   REQUIRE_EQ( 3, buf.front() );
   REQUIRE_EQ( 'y', buf[1] );
   REQUIRE_EQ( 'e', buf[2] );
   REQUIRE_EQ( 's', buf[3] );

   // More difficult: A string with full length of 255 actual characters
   sstring bufMaxSize {};
   bufMaxSize.fill( 'X' );
   bufMaxSize.back() = '\0';
   strConvCtoDelphi( bufMaxSize.data() );
   REQUIRE_EQ( 255, static_cast<uint8_t>( bufMaxSize.front() ) );
   for( int i = 1; i <= 255; i++ )
      REQUIRE_EQ( 'X', bufMaxSize[i] );

   // Exceeding maximum short string length: Should contain error message
   std::array<char, 257> bufTooBig {};
   bufTooBig.fill( 'X' );
   bufTooBig.back() = '\0';
   strConvCtoDelphi( bufTooBig.data() );
   REQUIRE_EQ( 0, bufTooBig.front() );
   REQUIRE_NE( 'X', bufTooBig[1] );
   std::string msg;
   msg.assign( &bufTooBig[1] );
   REQUIRE( posOfSubstr( "Error", msg ) != -1 );
}

TEST_CASE( "Test converting char buffer with Delphi short string layout to C-style string (PChar)" )
{
   // Easy case, three-letter word "yes"
   std::array<char, 4> buf {};
   buf[0] = 3;
   buf[1] = 'y';
   buf[2] = 'e';
   buf[3] = 's';
   strConvDelphiToC( buf.data() );
   REQUIRE_EQ( 'y', buf[0] );
   REQUIRE_EQ( 'e', buf[1] );
   REQUIRE_EQ( 's', buf[2] );
   REQUIRE_EQ( '\0', buf[3] );

   // Full length string
   sstring bufMaxSize {};
   bufMaxSize.fill( 'X' );
   bufMaxSize.front() = static_cast<uint8_t>( 255 );
   strConvDelphiToC( bufMaxSize.data() );
   for( int i = 0; i < 255; i++ )
      REQUIRE_EQ( 'X', bufMaxSize[i] );
   REQUIRE_EQ( '\0', bufMaxSize[255] );
}

TEST_CASE( "Test converting char buffer with Delphi short string layout to C++ standard library string" )
{
   // Easy case, three-letter word "yes"
   std::array<char, 4> buf {};
   buf[0] = 3;
   buf[1] = 'y';
   buf[2] = 'e';
   buf[3] = 's';
   const std::string s { strConvDelphiToCpp( buf.data() ) };
   REQUIRE_EQ( "yes"s, s );

   // Full length string
   sstring bufMaxSize {};
   bufMaxSize.fill( 'X' );
   bufMaxSize.front() = static_cast<uint8_t>( 255 );
   const std::string sMaxSize { strConvDelphiToCpp( bufMaxSize.data() ) };
   REQUIRE_EQ( std::string( 255, 'X' ), sMaxSize );
}

TEST_CASE( "Test converting C++ standard library string to char buffer with Delphi short string layout" )
{
   // Easy case
   std::string s { "yes" };
   std::array<char, 4> buf {};
   strConvCppToDelphi( s, buf.data() );
   REQUIRE_EQ( 3, buf.front() );
   REQUIRE_EQ( 'y', buf[1] );
   REQUIRE_EQ( 'e', buf[2] );
   REQUIRE_EQ( 's', buf[3] );

   // Full length string
   std::string sMaxSize( 255, 'X' );
   sstring bufMaxSize {};
   strConvCppToDelphi( sMaxSize, bufMaxSize.data() );
   REQUIRE_EQ( 255, static_cast<uint8_t>( bufMaxSize.front() ) );
   for( int i { 1 }; i < (int)bufMaxSize.size(); i++ )
      REQUIRE_EQ( 'X', bufMaxSize[i] );

   // Exceeding maximum short string length: Should contain error message
   std::string sTooBig( 256, 'X' );
   std::array<char, 257> bufTooBig {};
   strConvCppToDelphi( sTooBig, bufTooBig.data() );
   REQUIRE_EQ( 0, bufTooBig.front() );
   REQUIRE_NE( 'X', bufTooBig[1] );
   std::string msg;
   msg.assign( &bufTooBig[1] );
   REQUIRE( posOfSubstr( "Error", msg ) != -1 );
}

TEST_CASE( "Find position of substring in string starting from an offset position")
{
   const std::string s {"wherever you go, there you are"s}, sub {", there"s};

   // special case: substring is a single character
   REQUIRE_EQ(10, LStrPos( "o"s, s ));
   REQUIRE_EQ(24, LStrPosSp("o"s, s, 22 ));
   REQUIRE_EQ(-1, LStrPosSp("o"s, s, 27 ));
   REQUIRE_EQ(-1, LStrPos("x"s, s));

   // find "wherever" at the start
   REQUIRE_EQ(0, LStrPosSp( "wherever"s, s, 0 ));
   REQUIRE_EQ(0, LStrPos( "wherever"s, s ));

   // find ", there" in the middle
   REQUIRE_EQ(15, LStrPosSp( sub, s, 0 ));
   REQUIRE_EQ(15, LStrPosSp( sub, s, 15 ));
   REQUIRE_EQ(15, LStrPos(sub, s));

   // not found case
   REQUIRE_EQ(-1, LStrPosSp( "wherever"s, s, 1 ));
   REQUIRE_EQ(-1, LStrPosSp( sub, s, 16 ));
   REQUIRE_EQ(-1, LStrPosSp( "invalid-substring"s, s, 0 ));
   REQUIRE_EQ(-1, LStrPos( "invalid-substring"s, s ));
}

TEST_CASE( "Test using gsgetchar to get a char with index from string" )
{
   REQUIRE_EQ( gsgetchar( " ? ", 2 ), '?' );
   REQUIRE( !gsgetchar( "x", -1 ) );
   REQUIRE( !gsgetchar( "x", 2 ) );
}

TEST_CASE("Test cleaning a path")
{
#if defined(_WIN32)
   // One dot gets crushed
   std::string oneDot {R"(\abc\.\)"};
   cleanpath(oneDot, '\\');
   REQUIRE_EQ("\\abc\\"s, oneDot);
   // Two dots go one level up
   std::string twoDots {R"(\abc\def\..\)"};
   cleanpath(twoDots, '\\');
   REQUIRE_EQ("\\abc\\"s, twoDots);
#else
   // One dot gets crushed
   auto oneDot {"/abc/./"s};
   cleanpath(oneDot, '/');
   REQUIRE_EQ("/abc/"s, oneDot);
   // Two dots go one level up
   auto twoDots {"/abc/def/../"s};
   cleanpath(twoDots, '/');
   REQUIRE_EQ("/abc/"s, twoDots);
#endif
   // TODO: Extend unit test!
}

TEST_SUITE_END();

}
