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

#include <set>
#include <fstream>
#include <filesystem>

#include "utils.hpp"

#include "tests/doctest.hpp"

using namespace std::literals::string_literals;

namespace tests::gdlibtests::utilstests
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

   // Exhaustive test
   for( char c { std::numeric_limits<char>::min() }; c < std::numeric_limits<char>::max(); c++ )
   {
      REQUIRE_EQ( ( c >= 'a' && c <= 'z' ? std::toupper( c ) : c ), utils::toupper( c ) );
      REQUIRE_EQ( ( c >= 'A' && c <= 'Z' ? std::tolower( c ) : c ), utils::tolower( c ) );
   }
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
   const std::vector nums = { 1, 2, 3, 23, 0 };
   REQUIRE( utils::in( 23, nums ) );
   REQUIRE_FALSE( utils::in( 24, nums ) );

   REQUIRE( utils::in( 23, 1, 2, 23, 0 ) );
   REQUIRE_FALSE( utils::in( 24, 1, 2, 23, 0 ) );

   REQUIRE( utils::in( 1, 1 ) );
   REQUIRE_FALSE( utils::in( 1, 2 ) );

   {
      std::vector<int> v;
      REQUIRE_FALSE( utils::in( 1, v ) );
      v.push_back( 1 );
      REQUIRE( utils::in( 1, v ) );

      std::map<int, int> m;
      REQUIRE_FALSE( utils::in( 1, m ) );
      m[23] = 42;
      REQUIRE( utils::in( 23, m ) );

      std::map<std::string, int> m2;
      REQUIRE_FALSE( utils::in( "abc"s, m2 ) );
      m2["abc"] = 42;
      REQUIRE( utils::in( "abc"s, m2 ) );
   }

   {
      ClassWithContains cwc;
      REQUIRE( utils::in( 23, cwc ) );
      REQUIRE_FALSE( utils::in( 1, cwc ) );
   }
}

TEST_CASE( "System.Val function to parse numbers from strings" )
{
   {
      int num, code;
      utils::val( "23", num, code );
      REQUIRE_EQ( num, 23 );
      REQUIRE_FALSE( code );
      utils::val( "23$", num, code );
      REQUIRE_EQ( code, 3 );
   }

   {
      double num;
      int code;
      utils::val( "23.1", num, code );
      REQUIRE_EQ( num, 23.1 );
      REQUIRE_FALSE( code );
      utils::val( "23$", num, code );
      REQUIRE_EQ( 3, code );
   }

   {
      auto valTest = []( std::string in, auto out, int expectedEC = 0 ) {
         decltype( out ) x {};
         int errorCode {};
         utils::val( in, x, errorCode );
         REQUIRE_EQ( expectedEC, errorCode );
         if( !expectedEC )// only parsed value if we don't expect failure!
            REQUIRE_EQ( out, x );
      };

      valTest( "23", 23 );
      valTest( ".141", (double)0.141 );
      valTest( "3.141", 3.141 );
      valTest( "3.141", (int) 0, 2 );
      valTest( "003e+0000", 3.0 );
      valTest( "003e-0000", 3.0 );
      valTest( "003e+1", 30.0 );
      valTest( "3e-1", 0.3 );
      valTest( "1e-1", 0.1 );
      valTest( "12345_678", (int) 0, 6 );
      valTest( "0xFF", (double) 0.0, 2 );
      valTest( "$FF", (double) 0.0, 1 );
      valTest( "0xFF", 255 );
      valTest( "$FF", 255 );
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

TEST_CASE( "Arbitrary size blank strings" )
{
   REQUIRE_EQ( "   ", utils::blanks( 3 ) );
}

TEST_CASE( "Trim blanks from beginning and end of strings but not in between" )
{
   REQUIRE_EQ( ""s, utils::trim( ""s ) );
   REQUIRE_EQ( ""s, utils::trim( "                  \t \n           "s ) );
   REQUIRE_EQ( "abc def"s, utils::trim( "   abc def "s ) );
   REQUIRE_EQ( "abc"s, utils::trim( "     abc"s ) );
   REQUIRE_EQ( "abc"s, utils::trim( "abc     "s ) );
}

TEST_CASE( "Trim space chars from the beginning of a string in place" ) {
   std::string a{"    xyz "s}, b{}, c{"       "}, d{"xyz "};
   for(std::string *s : {&a,&b,&c,&d})
      utils::trimLeft(*s);
   REQUIRE_EQ("xyz "s, a);
   REQUIRE(b.empty());
   REQUIRE(c.empty());
   REQUIRE_EQ("xyz "s, d);
}

TEST_CASE( "Split string into parts using a separating character" )
{
   REQUIRE_EQ( utils::split( "a;b;c"s, ';' ), std::list { "a"s, "b"s, "c"s } );
}

TEST_CASE( "Split string into its whitespace separated parts but don't split inside quotes" )
{
   REQUIRE_EQ( std::list { "first"s, "\"second part\""s, "\'third part\'"s, "fourth"s }, utils::splitWithQuotedItems( " first   \"second part\" \'third part\' fourth "s ) );
}

TEST_CASE( "Uppercase all characters in a string" )
{
   REQUIRE_EQ( utils::uppercase( " gams software gmbh "s ), " GAMS SOFTWARE GMBH "s );
}

TEST_CASE( "Join multiple strings with separator. Inverse operation to split." )
{
   REQUIRE_EQ( "a;b;c"s, utils::join( ';', { "a"s, "b"s, "c"s } ) );
}

TEST_CASE( "All positions (index) of substrings in a string" )
{
   REQUIRE_EQ( std::vector<size_t> { 5 }, utils::substrPositions( "3.14e+01"s, "+"s ) );
   REQUIRE_EQ( std::vector<size_t> { 0 }, utils::substrPositions( "+3.14"s, "+"s ) );
   REQUIRE_EQ( std::vector<size_t> { 0, 1, 3 }, utils::substrPositions( "??x?x"s, "?"s ) );
}

TEST_CASE( "Test get line with separator" )
{
   const std::string tmpfname = "tempfile";
   utils::spit( tmpfname, "first\nsecond\nthird"s );
   std::fstream fs { tmpfname, std::ios::in };
   REQUIRE_EQ( "first\n"s, utils::getLineWithSep( fs ) );
   REQUIRE_EQ( "second\n"s, utils::getLineWithSep( fs ) );
   REQUIRE_EQ( "third"s, utils::getLineWithSep( fs ) );
   fs.close();
   std::filesystem::remove( tmpfname );
}

TEST_CASE( "Test double to string function with width and precision" )
{
   REQUIRE_EQ( "3"s, utils::doubleToString( 3.141, 1, 0 ) );
   REQUIRE_EQ( "  23", utils::doubleToString( 23, 4, 0 ) );
   REQUIRE_EQ( "   3.1416"s, utils::doubleToString( 3.141592653589793, 9, 4 ) );
}

TEST_CASE( "Test integer num to string conversion with specified width and padding from left" )
{
   REQUIRE_EQ( "  23"s, utils::strInflateWidth( 23, 4 ) );
   REQUIRE_EQ( "23"s, utils::strInflateWidth( 23, 2 ) );
   REQUIRE_EQ( "23"s, utils::strInflateWidth( 23, 0 ) );
}

TEST_CASE( "Test quote directory names in path with whitespace" )
{
   REQUIRE_EQ( "/home/andre/\"gms test\"/"s, utils::quoteWhitespaceDir( "/home/andre/gms test/", '/', '\"' ) );
   REQUIRE_EQ( "/home/andre/\"gms test\""s, utils::quoteWhitespaceDir( "/home/andre/gms test", '/', '\"' ) );
}

TEST_CASE( "Test checking for a prefix in a string" )
{
   REQUIRE_FALSE( utils::starts_with( "9", "999" ) );
   REQUIRE_FALSE( utils::starts_with( "Das hier ist ein Test", "Test" ) );
   REQUIRE( utils::starts_with( "999", "9" ) );
   REQUIRE( utils::starts_with( "999", "999" ) );
}

TEST_CASE( "Test checking for a suffix in a string" )
{
   REQUIRE_FALSE( utils::ends_with( "9", "999" ) );
   REQUIRE_FALSE( utils::ends_with( "Das hier ist ein Test!", "Test" ) );
   REQUIRE( utils::ends_with( "999", "9" ) );
   REQUIRE( utils::ends_with( "999", "999" ) );
}

TEST_CASE( "Test parse number" )
{
   REQUIRE_EQ( 3, utils::parseNumber( "3" ) );
   REQUIRE_EQ( 3.141, utils::parseNumber( "3.141" ) );
   REQUIRE_EQ( -1, utils::parseNumber( "-1" ) );
   REQUIRE_EQ( 0, utils::parseNumber( "0" ) );
   REQUIRE_EQ( 3, utils::parseNumber( "3e0" ) );
   REQUIRE_EQ( 30, utils::parseNumber( "3e1" ) );
   REQUIRE_EQ( 0.3, utils::parseNumber( "3e-1" ) );
}

TEST_CASE( "Test replacing all matching substrings of given string with a replacement string" )
{
   REQUIRE_EQ( "Amazing.", utils::replaceSubstrs( "This is Amazing.", "This is ", "" ) );
   REQUIRE_EQ( "", utils::replaceSubstrs( "          ", " ", "" ) );

   const auto res = utils::replaceSubstrs( "There is no such thing as a substring called substring in this string.",
                                           "substring",
                                           "thing" );
   REQUIRE_EQ( "There is no such thing as a thing called thing in this string.", res );
}

TEST_CASE( "Test assigning a single value to all array cells in a range between bounds (inclusive)" )
{
   std::array<int, 5> nums { 1, 2, 3, 4, 5 }, expNums { 1, 0, 0, 0, 5 };
   utils::assignRange<int, 5>( nums, 1, 3, 0 );
   REQUIRE_EQ( expNums, nums );
}

TEST_CASE( "Test trimming blanks from the tail of a string" )
{
   const std::string expectedStr { "  surrounded by blanks"s };
   std::string s { "  surrounded by blanks  "s };
   REQUIRE_EQ( expectedStr, utils::trimRight( s ) );
   std::string storage( 64, 'x' );
   utils::trimRight( s, storage );
   REQUIRE_EQ( expectedStr, storage );

   int len;
   std::array<char, 256> buf {};
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
   std::array<char, 256> buf {};

   REQUIRE_FALSE( utils::strConvCppToDelphi( "abc"s, buf.data() ) );
   REQUIRE_EQ( 3, buf.front() );
   REQUIRE_EQ( 'a', buf[1] );
   REQUIRE_EQ( 'b', buf[2] );
   REQUIRE_EQ( 'c', buf[3] );

   REQUIRE_FALSE( utils::strConvCppToDelphi( ""s, buf.data() ) );
   REQUIRE_EQ( 0, buf.front() );

   REQUIRE_FALSE( utils::strConvCppToDelphi( std::string( 255, 'x' ), buf.data() ) );
   REQUIRE_EQ( 255, (uint8_t) buf.front() );
   for( int i {}; i < 255; i++ )
      REQUIRE_EQ( 'x', buf[i + 1] );

   REQUIRE( utils::strConvCppToDelphi( std::string( 256, 'x' ), buf.data() ) );
   REQUIRE_GT( buf.front(), 0 );
   REQUIRE_EQ( 'E', buf[1] );
   REQUIRE_EQ( 'r', buf[2] );
   REQUIRE_EQ( 'r', buf[3] );
   REQUIRE_EQ( 'o', buf[4] );
   REQUIRE_EQ( 'r', buf[5] );
}

TEST_CASE( "Test checking if any character from a string satisfies a given predicate" )
{
   REQUIRE( utils::anychar( []( char c ) { return c == 's'; }, "test"s ) );
   REQUIRE_FALSE( utils::anychar( []( char c ) { return c == 'x'; }, "test"s ) );
   REQUIRE_FALSE( utils::anychar( []( char c ) { return true; }, ""s ) );
}

TEST_CASE( "Test permutated assigned of string characters" )
{
   std::string dest( 3, ' ' ), src { "cab" };
   std::vector<int> readIndices { 1, 2, 0 }, writeIndices { 0, 1, 2 };
   utils::permutAssign( dest, src, writeIndices, readIndices );
   REQUIRE_EQ( "abc"s, dest );
}

TEST_CASE( "Test removing line ending marker" )
{
   std::string s1 { "First line\r\nSecond line\r"s },
           s2 { "First line\r\nSecond line\n"s };
   utils::removeTrailingCarriageReturnOrLineFeed( s1 );
   REQUIRE_EQ( "First line\r\nSecond line"s, s1 );
   utils::removeTrailingCarriageReturnOrLineFeed( s2 );
   REQUIRE_EQ( "First line\r\nSecond line"s, s2 );
}

TEST_CASE( "Test upper-/lowercasing of strings (copy)" )
{
   std::string s { "This is a Test!"s },
           expectUppercase { "THIS IS A TEST!" },
           expectLowercase { "this is a test!" };
   REQUIRE_EQ( expectUppercase, utils::uppercase( s ) );
   REQUIRE_EQ( expectLowercase, utils::lowercase( s ) );
}

TEST_CASE( "Test checking if a string case-insensitive matches at least one element from a string list" )
{
   REQUIRE_FALSE( utils::sameTextAsAny( "test", "abc", "aBc", "test X" ) );
   REQUIRE( utils::sameTextAsAny( "test", "abc", "TEST" ) );
   REQUIRE( utils::sameTextAsAny( "test"s, "abc"s, "TEST"s ) );
}

TEST_CASE( "Test if a string starts with a prefix" )
{
   REQUIRE( utils::sameTextPrefix( "test", "te" ) );
   REQUIRE_FALSE( utils::sameTextPrefix( "asset", "te" ) );
   REQUIRE_FALSE( utils::sameTextPrefix( "test", "testing" ) );
}

TEST_CASE( "Test trimming zeroes from string from the right/back but only for decimals (not integers)" )
{
   REQUIRE_EQ( "1230.045"s, utils::trimZeroesRight( "1230.045000"s ) );
   REQUIRE_EQ( "1230,045"s, utils::trimZeroesRight( "1230,045000"s, ',' ) );
   REQUIRE_EQ( "1230045000"s, utils::trimZeroesRight( "1230045000"s ) );
}

TEST_CASE( "Test checking if a string contains a char with code less than given integer" )
{
   REQUIRE( utils::hasCharLt( "a"s, 'b' ) );
   REQUIRE_FALSE( utils::hasCharLt( "b"s, 'a' ) );
}

TEST_CASE( "Test rounding to n-digits" )
{
   const double eps { 1e-4 };
   REQUIRE_LT( 23.42 - utils::round( 23.4242, 2 ), eps );
   REQUIRE_LT( 23.4 - utils::round( 23.4242, 1 ), eps );
   REQUIRE_LT( 23.0 - utils::round( 23.4242, 0 ), eps );
}

TEST_CASE( "Test double to integral type rounding function that should mimic System.Round Delphi behavior" )
{
   REQUIRE_EQ(0, utils::round<int>(0.45));
   REQUIRE_EQ(0, utils::round<int>(-0.45));
   // round tie away from 0
   REQUIRE_EQ(-1, utils::round<int>(-0.5));
   REQUIRE_EQ(1, utils::round<int>(0.5));
   REQUIRE_EQ(-3, utils::round<int>(-2.5));
   REQUIRE_EQ(3, utils::round<int>(2.5));
}

TEST_CASE( "Test replacing specific char with another one in a string (in place)" )
{
   std::string s { "Letter X"s };
   utils::replaceChar( 'X', 'Y', s );
   REQUIRE_EQ( "Letter Y"s, s );
}

TEST_CASE( "Test generating string of repeating zeroes" )
{
   REQUIRE_EQ( "0000"s, utils::zeros( 4 ) );
   REQUIRE( utils::zeros( 0 ).empty() );
}

TEST_CASE( "Test getting index of last occurence of character in string" )
{
   REQUIRE_EQ( 13, utils::lastOccurence( "abcdefabcdefabcdef"s, 'b' ) );
   REQUIRE_EQ( -1, utils::lastOccurence( std::string( 23, 'a' ), 'b' ) );
}

TEST_CASE( "Test computing the length of a string without blanks" )
{
   REQUIRE_EQ( 4, utils::strLenNoWhitespace( " te s t  "s ) );
   REQUIRE_EQ( 4, utils::strLenNoWhitespace( "test"s ) );
   REQUIRE_EQ( 0, utils::strLenNoWhitespace( std::string( 10, ' ' ) ) );
}

TEST_CASE( "Test getting a ref to char in a str at index pos with null-terminator-tail enforcement" )
{
   std::string s { "test"s };
   REQUIRE_EQ( s.front(), utils::getCharAtIndexOrAppend( s, 0 ) );
   REQUIRE_EQ( '\0', utils::getCharAtIndexOrAppend( s, 4 ) );
}

TEST_CASE( "Test if string contains a specific character or a char from a set of chars" )
{
   REQUIRE( utils::strContains( "test"s, 't' ) );
   REQUIRE_FALSE( utils::strContains( "test"s, 'x' ) );
   REQUIRE( utils::strContains( "test"s, { 'f', 'g', 'e' } ) );
   REQUIRE_FALSE( utils::strContains( "test"s, { 'x', 'y', 'z' } ) );
}

TEST_CASE( "Test boolean 'exclusive or' (xor) operator" )
{
   REQUIRE( utils::excl_or( true, false ) );
   REQUIRE( utils::excl_or( false, true ) );
   REQUIRE_FALSE( utils::excl_or( false, false ) );
   REQUIRE_FALSE( utils::excl_or( true, true ) );
}

TEST_CASE( "Test finding the index of where a substring starts in an enclosing string" )
{
   REQUIRE_EQ( 4, utils::posOfSubstr( "osteron"s, "testosteron"s ) );
   REQUIRE_EQ( -1, utils::posOfSubstr( "xyz"s, "test"s ) );
}

TEST_CASE( "Test constructing string via lambda with index arg" )
{
   std::string s { "test"s };
   REQUIRE_EQ( s, utils::constructStr( (int) s.size(), [&s]( int ix ) { return s[ix]; } ) );
   REQUIRE( utils::constructStr( 0, []( int ix ) { return 0; } ).empty() );
}

TEST_CASE( "Test quoting a string iff. it contains blanks" )
{
   REQUIRE_EQ( "nowhitespace"s, utils::quoteWhitespace( "nowhitespace"s, '\"' ) );
   REQUIRE_EQ( "\"has whitespace\""s, utils::quoteWhitespace( "has whitespace"s, '\"' ) );
}

TEST_CASE( "Test string to bool conversion" )
{
   REQUIRE( utils::strToBool( "yes"s ) );
   REQUIRE_FALSE( utils::strToBool( "no"s ) );
}

TEST_CASE( "Test lexicographical string comparison (optionally case-sensitive)" )
{
   REQUIRE_EQ( 0, utils::strCompare( ""s, ""s ) );
   REQUIRE_EQ( 1, utils::strCompare( "Alpha"s, ""s ) );
   REQUIRE_EQ( -1, utils::strCompare( ""s, "Beta"s ) );
   REQUIRE_EQ( -1, utils::strCompare( "Alpha"s, "Beta"s ) );
   REQUIRE_EQ( 1, utils::strCompare( "Beta"s, "Alpha"s ) );
   REQUIRE_EQ( 0, utils::strCompare( "alpha"s, "Alpha"s ) );
   REQUIRE_EQ( 32, utils::strCompare( "alpha"s, "Alpha"s, false ) );
}

TEST_CASE( "Test copying the contents of a pchar into a char buffer" )
{
   std::array<char, 256> buf {};
   utils::assignPCharToBuf( "abc", buf.data(), buf.size() );
   REQUIRE( !std::strcmp( "abc", buf.data() ) );
   std::string tooLong( 256, 'x' );
   utils::assignPCharToBuf( tooLong.c_str(), buf.data(), buf.size() );
   REQUIRE_EQ( '\0', buf.back() );
}

TEST_CASE( "Test copying contents of a string into a char buffer" )
{
   std::array<char, 3> buf {};
   utils::assignStrToBuf( ""s, buf.data(), static_cast<int>(buf.size()) );
   REQUIRE_EQ( '\0', buf.front() );
   utils::assignStrToBuf( std::string( 100, 'x' ), buf.data(), static_cast<int>(buf.size()) );
   std::array<char, 256> buf2 {};
   utils::assignStrToBuf( "abc", buf2.data(), static_cast<int>(buf2.size()) );
   REQUIRE( !std::strcmp( "abc", buf2.data() ) );
}

TEST_CASE( "Test copying contents of a string view into a char buffer" )
{
   std::array<char, 256> buf {};
   utils::assignViewToBuf( ""s, buf.data(), buf.size() );
   REQUIRE_EQ( '\0', buf.front() );
   utils::assignViewToBuf( "abc"s, buf.data(), buf.size() );
   REQUIRE( !std::strcmp( "abc", buf.data() ) );
   utils::assignViewToBuf( std::string( 300, 'x' ), buf.data(), buf.size() );
}

TEST_CASE( "Test slurping the contents from a text file" )
{
   std::string fn { "tmp.txt"s },
           contents { "First line\nsecond line\nthird line"s };
   {
      std::ofstream ofs { fn };
      ofs.write( contents.c_str(), (long) contents.size() );
   }
   REQUIRE_EQ( contents, utils::slurp( fn ) );
   std::filesystem::remove( fn );
}

TEST_CASE( "Test getting index of element in collection (list, vector, array, ...)" )
{
   auto checkForTwo = []( const int &elem ) {
      return elem == 2;
   };

   // list
   REQUIRE_EQ( 1, utils::indexOf( std::list<int> { 1, 2, 3 }, 2 ) );
   REQUIRE_EQ( -1, utils::indexOf( std::list<int> { 1, 2, 3 }, 5 ) );
   REQUIRE_EQ( -1, utils::indexOf<int>( std::list<int> {}, 2 ) );
   REQUIRE_EQ( 1, utils::indexOf<int>( std::list<int> { 1, 2, 3 }, checkForTwo ) );

   // vector
   REQUIRE_EQ( 1, utils::indexOf( std::vector<int> { 1, 2, 3 }, 2 ) );
   REQUIRE_EQ( -1, utils::indexOf( std::vector<int> { 1, 2, 3 }, 5 ) );
   REQUIRE_EQ( -1, utils::indexOf( std::vector<int> {}, 2 ) );
   REQUIRE_EQ( 1, utils::indexOf<int>( std::vector<int> { 1, 2, 3 }, checkForTwo ) );

   // array
   REQUIRE_EQ( 1, utils::indexOf<int, 3>( std::array<int, 3> { 1, 2, 3 }, 2 ) );
   REQUIRE_EQ( -1, utils::indexOf<int, 3>( std::array<int, 3> { 1, 2, 3 }, 5 ) );
   REQUIRE_EQ( 1, utils::indexOf<int, 3>( std::array<int, 3> { 1, 2, 3 }, checkForTwo ) );

   // vector of pairs
   std::vector<std::pair<int, double>> pairs {
           { 1, 1.0 },
           { 2, 2.0 },
           { 2, 3.0 },
   };
   int n { 2 };
   REQUIRE_EQ( 1, utils::pairIndexOfFirst<int, double>( pairs, n ) );
   n = 5;
   REQUIRE_EQ( -1, utils::pairIndexOfFirst<int, double>( pairs, n ) );
}

TEST_CASE( "Test ptr to array to std::array conversion helpers" )
{
   const int n { 3 };
   int nums[n];
   for( int i {}; i < n; i++ )
      nums[i] = i + 1;
   std::array<int, n> numsArr = utils::asArray<int, n>( nums ),
                      expArr { 1, 2, 3 };
   REQUIRE_EQ( expArr, numsArr );
   auto numsArrShort = utils::asArrayN<int, n>( nums, 2 );
   std::array<int, n> expArrShort { 1, 2, 0 };
   REQUIRE_EQ( expArrShort, numsArrShort );
}

TEST_CASE( "Test constructing a std::array object filled by repeating a single value" )
{
   std::array<int, 3> arr { utils::arrayWithValue<int, 3>( 23 ) },
           expArr { 23, 23, 23 };
   REQUIRE_EQ( expArr, arr );
}

TEST_CASE( "Test creating a new C-style string on the heap from the contents of another buffer" )
{
   size_t memSize {};
   std::unique_ptr<char[]> s { utils::NewString( "abc", 3, memSize ) };
   REQUIRE_FALSE( utils::NewString( nullptr, 23 ) );
   REQUIRE( !std::strcmp( s.get(), "abc" ) );
   REQUIRE_EQ( 4, memSize );
}

TEST_CASE("Test charset type") {
   utils::charset empty, a{'a'}, b{'b'}, c{'a', 'b', 'c'};
   REQUIRE_FALSE(empty.contains('x'));
   REQUIRE(a.contains('a'));
   REQUIRE_FALSE(a.contains('b'));
   REQUIRE(b.contains('b'));
   REQUIRE_FALSE(b.contains('a'));
   REQUIRE(c.contains('a'));
   REQUIRE(c.contains('b'));
   REQUIRE(c.contains('c'));
   REQUIRE_FALSE(c.contains('d'));
   c.erase('b');
   REQUIRE_FALSE(c.contains('b'));
}

TEST_SUITE_END();

}
