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

#include "../utils.h"// for sameTextPChar, in, trim, strCompare, strConvCp...
#include "doctest.h" // for ResultBuilder, REQUIRE_EQ, Expression_lhs, Tes...
#include <array>     // for array
#include <cstdint>   // for uint8_t
#include <cstring>   // for strcmp
#include <memory>    // for unique_ptr
#include <string>    // for operator""s, string, basic_string, allocator
#include <ostream>

using namespace std::literals::string_literals;

namespace gdx::tests::utilstests
{

TEST_SUITE_BEGIN( "utils" );

TEST_CASE( "Test character upper/lowercase conversion" )
{
   REQUIRE_EQ( 'A', utils::toupper( 'a' ) );
   REQUIRE_EQ( 'Z', utils::toupper( 'z' ) );
   REQUIRE_EQ( '$', utils::toupper( '$' ) );
   REQUIRE_EQ( 'a', utils::tolower( 'A' ) );
   REQUIRE_EQ( 'z', utils::tolower( 'Z' ) );
   REQUIRE_EQ( '$', utils::tolower( '$' ) );
}

class ClassWithContains : public utils::IContainsPredicate<int>
{
public:
   [[nodiscard]] bool contains( const int &elem ) const override
   {
      return elem == 23;
   }
};

TEST_CASE( "Test inclusion predicate" )
{
   REQUIRE( utils::in( 23, 1, 2, 23, 0 ) );
   REQUIRE_FALSE( utils::in( 24, 1, 2, 23, 0 ) );

   REQUIRE( utils::in( 1, 1 ) );
   REQUIRE_FALSE( utils::in( 1, 2 ) );

   {
      ClassWithContains cwc;
      REQUIRE( utils::in( 23, cwc ) );
      REQUIRE_FALSE( utils::in( 1, cwc ) );
   }
}

TEST_CASE( "Comparing strings the Delphi way" )
{
   // With std::string
   REQUIRE( utils::sameText( ""s, ""s ) );
   REQUIRE( utils::sameText( "aBc"s, "AbC"s ) );
   REQUIRE_FALSE( utils::sameText<false>( "aBc"s, "AbC"s ) );

   // With pchars (pointer to char -> char *)
   // Case-insensitive
   REQUIRE( utils::sameTextPChar( nullptr, nullptr ) );
   REQUIRE_FALSE( utils::sameTextPChar( "", nullptr ) );
   REQUIRE_FALSE( utils::sameTextPChar( nullptr, "" ) );
   REQUIRE( utils::sameTextPChar( "", "" ) );
   REQUIRE( utils::sameTextPChar( "aBc", "AbC" ) );
   REQUIRE( utils::sameTextPChar( "abc", "abc" ) );
   // Case-sensitive
   REQUIRE( utils::sameTextPChar<false>( nullptr, nullptr ) );
   REQUIRE_FALSE( utils::sameTextPChar<false>( "aBc", "AbC " ) );
   REQUIRE_FALSE( utils::sameTextPChar<false>( "aBc", "AbC" ) );
   REQUIRE( utils::sameTextPChar<false>( "abc", "abc" ) );
}

TEST_CASE( "Trim blanks from beginning and end of strings but not in between" )
{
   REQUIRE_EQ( ""s, utils::trim( ""s ) );
   REQUIRE_EQ( ""s, utils::trim( "                  \t \n           "s ) );
   REQUIRE_EQ( "abc def"s, utils::trim( "   abc def "s ) );
   REQUIRE_EQ( "abc"s, utils::trim( "     abc"s ) );
   REQUIRE_EQ( "abc"s, utils::trim( "abc     "s ) );
}

TEST_CASE( "Uppercase all characters in a string" )
{
   REQUIRE_EQ( utils::uppercase( " gams software gmbh "s ), " GAMS SOFTWARE GMBH "s );
}

TEST_CASE( "Test trimming blanks from the tail of a string" )
{
   const std::string expectedStr{ "  surrounded by blanks"s };
   std::string s{ "  surrounded by blanks  "s };

   int len;
   std::array<char, 256> buf{};
   const char *out = utils::trimRight( s.c_str(), buf.data(), len );
   REQUIRE_EQ( expectedStr.length(), len );
   REQUIRE( !strcmp( expectedStr.c_str(), out ) );

   out = utils::trimRight( " ", buf.data(), len );
   REQUIRE_EQ( 0, len );
   REQUIRE_EQ( '\0', out[0] );

   out = utils::trimRight( "1", buf.data(), len );
   REQUIRE_EQ( 1, len );
   REQUIRE( !strcmp( "1", out ) );

   std::string _64_chars( 64, 'i' );
   const char *backing = _64_chars.c_str();
   out = utils::trimRight( backing, buf.data(), len );
   REQUIRE_EQ( 64, len );
   REQUIRE( !strcmp( _64_chars.c_str(), out ) );
   // no blanks at end of this i*64 ('iii...iii') str, so should not create new buffer!
   REQUIRE_EQ( backing, out );
}

TEST_CASE( "Test conversion of string view into Delphi-format ShortString" )
{
   std::array<char, 256> buf{};

   REQUIRE_FALSE( utils::strConvCppToDelphi( "abc"s, buf.data() ) );
   REQUIRE_EQ( 3, buf.front() );
   REQUIRE_EQ( 'a', buf[1] );
   REQUIRE_EQ( 'b', buf[2] );
   REQUIRE_EQ( 'c', buf[3] );

   REQUIRE_FALSE( utils::strConvCppToDelphi( ""s, buf.data() ) );
   REQUIRE_EQ( 0, buf.front() );

   REQUIRE_FALSE( utils::strConvCppToDelphi( std::string( 255, 'x' ), buf.data() ) );
   REQUIRE_EQ( 255, (uint8_t) buf.front() );
   for( int i{}; i < 255; i++ )
      REQUIRE_EQ( 'x', buf[i + 1] );

   REQUIRE( utils::strConvCppToDelphi( std::string( 256, 'x' ), buf.data() ) );
   REQUIRE_GT( buf.front(), 0 );
   REQUIRE_EQ( 'E', buf[1] );
   REQUIRE_EQ( 'r', buf[2] );
   REQUIRE_EQ( 'r', buf[3] );
   REQUIRE_EQ( 'o', buf[4] );
   REQUIRE_EQ( 'r', buf[5] );
}

TEST_CASE( "Test quoting a string iff. it contains blanks" )
{
   REQUIRE_EQ( "nowhitespace"s, utils::quoteWhitespace( "nowhitespace"s, '\"' ) );
   REQUIRE_EQ( "\"has whitespace\""s, utils::quoteWhitespace( "has whitespace"s, '\"' ) );
}

TEST_CASE( "Test lexicographical string comparison (optionally case-sensitive)" )
{
   REQUIRE_EQ( 0, utils::strCompare( "", "" ) );
   REQUIRE_EQ( 1, utils::strCompare( "Alpha", "" ) );
   REQUIRE_EQ( -1, utils::strCompare( "", "Beta" ) );
   REQUIRE_EQ( -1, utils::strCompare( "Alpha", "Beta" ) );
   REQUIRE_EQ( 1, utils::strCompare( "Beta", "Alpha" ) );
   REQUIRE_EQ( 0, utils::strCompare( "alpha", "Alpha" ) );
   REQUIRE_EQ( 32, utils::strCompare( "alpha", "Alpha", false ) );
}

TEST_CASE( "Test copying the contents of a pchar into a char buffer" )
{
   std::array<char, 256> buf{};
   utils::assignPCharToBuf( "abc", buf.data(), buf.size() );
   REQUIRE( !std::strcmp( "abc", buf.data() ) );
   std::string tooLong( 256, 'x' );
   utils::assignPCharToBuf( tooLong.c_str(), buf.data(), buf.size() );
   REQUIRE_EQ( '\0', buf.back() );
}

TEST_CASE( "Test copying contents of a string into a char buffer" )
{
   std::array<char, 3> buf{};
   utils::assignStrToBuf( ""s, buf.data(), buf.size() );
   REQUIRE_EQ( '\0', buf.front() );
   utils::assignStrToBuf( std::string( 100, 'x' ), buf.data(), buf.size() );
   std::array<char, 256> buf2{};
   utils::assignStrToBuf( "abc", buf2.data(), buf2.size() );
   REQUIRE( !std::strcmp( "abc", buf2.data() ) );
}

TEST_CASE( "Test copying contents of a string view into a char buffer" )
{
   std::array<char, 256> buf{};
   utils::assignViewToBuf( ""s, buf.data(), buf.size() );
   REQUIRE_EQ( '\0', buf.front() );
   utils::assignViewToBuf( "abc"s, buf.data(), buf.size() );
   REQUIRE( !std::strcmp( "abc", buf.data() ) );
   utils::assignViewToBuf( std::string( 300, 'x' ), buf.data(), buf.size() );
}

TEST_CASE( "Test constructing a std::array object filled by repeating a single value" )
{
   std::array<int, 3> arr{ utils::arrayWithValue<int, 3>( 23 ) },
           expArr{ 23, 23, 23 };
   REQUIRE_EQ( expArr, arr );
}

TEST_CASE( "Test creating a new C-style string on the heap from the contents of another buffer" )
{
   size_t memSize{};
   std::unique_ptr<char[]> s{ utils::NewString( "abc", 3, memSize ) };
   REQUIRE_FALSE( utils::NewString( nullptr, 23 ) );
   REQUIRE( !std::strcmp( s.get(), "abc" ) );
   REQUIRE_EQ( 4, memSize );
}

TEST_SUITE_END();

}// namespace gdx::tests::utilstests
