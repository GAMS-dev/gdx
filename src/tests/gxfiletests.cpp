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

#include "doctest.hpp"// for ResultBuilder, Expressi...
#include "../gxfile.hpp"

using namespace gdx;
using namespace std::literals::string_literals;

namespace gdx::tests::gxfiletests
{
TEST_SUITE_BEGIN( "GDX object tests private" );

TEST_CASE( "Test checking if an identifier string is well formed" )
{
   REQUIRE_FALSE( IsGoodIdent( "" ) );
   REQUIRE( IsGoodIdent( "x" ) );
   REQUIRE( IsGoodIdent( "x6" ) );
   REQUIRE_FALSE( IsGoodIdent( "6x" ) );
   REQUIRE_FALSE( IsGoodIdent( "_" ) );
   REQUIRE( IsGoodIdent( "x_" ) );
   std::string longestValidUEL( 63, 'x' ), tooLong( 64, 'x' );
   REQUIRE( IsGoodIdent( longestValidUEL.c_str() ) );
   REQUIRE_FALSE( IsGoodIdent( tooLong.c_str() ) );
}

TEST_CASE( "Test 'can be quoted' function" )
{
   REQUIRE_FALSE( CanBeQuoted( nullptr, 0 ) );
   REQUIRE( CanBeQuoted( "abc", 3 ) );
   REQUIRE( CanBeQuoted( "a\"bc", 4 ) );
   REQUIRE( CanBeQuoted( "a'bc", 4 ) );
   REQUIRE_FALSE( CanBeQuoted( "a'b\"c", 5 ) );
   REQUIRE_FALSE( CanBeQuoted( "ab\tc", 4 ) );
}

TEST_CASE( "Test checking for good UEL string" )
{
   REQUIRE_FALSE( GoodUELString( nullptr, 0 ) );
   REQUIRE( GoodUELString( "abc", 3 ) );
   REQUIRE_FALSE( GoodUELString( "abc\0d", 5 ));
   REQUIRE_FALSE( GoodUELString( "a", 100 ));
   std::string tooLong( 64, 'x' );
   REQUIRE_FALSE( GoodUELString( tooLong.c_str(), tooLong.size() ) );
   std::string stillOk( 63, 'x' );
   REQUIRE( GoodUELString( stillOk.c_str(), stillOk.size() ) );
}

TEST_CASE( "Test making a good explanatory text function" )
{
   REQUIRE_FALSE( MakeGoodExplText( nullptr ) );

   char explTxt1[256] = "'Test 1' and \"Test 2\"\t";
   std::string expectedExplTxt1 { "'Test 1' and 'Test 2'?" };
   REQUIRE_EQ( expectedExplTxt1.length(), MakeGoodExplText( explTxt1 ) );
   REQUIRE( !strcmp( expectedExplTxt1.c_str(), explTxt1 ) );

   char explTxt2[256] = "\"Test 1\"\n and 'Test 2'";
   std::string expectedExplTxt2 { R"("Test 1"? and "Test 2")" };
   REQUIRE_EQ( expectedExplTxt2.length(), MakeGoodExplText( explTxt2 ) );
   REQUIRE( !strcmp( expectedExplTxt2.c_str(), explTxt2 ) );

   // Make sure special chars (e.g. scandinavian alphabet, norwegian wind park names, ...) aren't
   // treated as control chars and replaced with question mark (?)
   char explTxt3[256] = "Skellefteå";
   std::string expectedExplTxt3 { "Skellefteå"s };
   REQUIRE_EQ( expectedExplTxt3.length(), MakeGoodExplText( explTxt3 ) );
   REQUIRE( !strcmp( expectedExplTxt3.c_str(), explTxt3 ) );
}

TEST_CASE( "Test TIntegerMapping" )
{
   TIntegerMapping im;
   REQUIRE( im.empty() );
   im.SetMapping( 3, 5 );
   REQUIRE_GE( im.size(), 3 );
   REQUIRE_FALSE( im.empty() );
   REQUIRE_EQ( 3, im.GetHighestIndex() );
   REQUIRE_EQ( 5, im.GetMapping( 3 ) );
   im.SetMapping( 2048, 8 );
   REQUIRE_GE( im.size(), 2048 );
   REQUIRE_EQ( 2048, im.GetHighestIndex() );
   REQUIRE_FALSE( im.empty() );
   REQUIRE_EQ( 8, im.GetMapping( 2048 ) );
   REQUIRE_EQ( 5, im.GetMapping( 3 ) );
}

TEST_SUITE_END();

}// namespace gdx::tests::gxfiletests