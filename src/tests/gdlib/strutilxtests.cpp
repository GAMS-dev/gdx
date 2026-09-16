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

#include <filesystem>
#include <iostream>
#include <string>

#if __cplusplus >= 202002L
#include <format>
#include <ranges>
#endif

namespace fs = std::filesystem;

struct PathTestCase {
    std::string description;
    std::string prefixDir;
    std::string path;
    fs::path expectedPath;
};


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

#if __cplusplus >= 202002L

static std::vector<PathTestCase> GetAsciiDirTests(void)
{
#if _WIN32
   return {
      // ======================================================================
      // CASE 1: Root directory without root name (Starts with '\')
      // Appends to the drive or server root of the prefix.
      // ======================================================================
      { "Standard Local: Replaces path, keeps drive",
         R"(C:\ProgramData\MyApp)",     R"(\Logs\session\)",       R"(C:\Logs\session\)" },

      // ======================================================================
      // CASE 2a: Pure Relative Path
      // Appends normally to the prefix.
      // ======================================================================
      { "Standard Local: Normal append",
         R"(C:\ProgramData\MyApp)",     R"(Cache\thumbnails\)",       R"(C:\ProgramData\MyApp\Cache\thumbnails\)" },

      // ======================================================================
      // CASE 2b: Relative Path with Dot (e.g. .\dir\test)
      // Resolves the dot out, appends normally.
      // ======================================================================
      { "Standard Local: Explicit current dir",
         R"(C:\ProgramData\MyApp\)",     R"(.\Cache\thumbnails\)",     R"(C:\ProgramData\MyApp\Cache\thumbnails\)" },

      // ======================================================================
      // CASE 2c: Relative Path with double Dot (e.g. ..\dir\test)
      // Resolves the dot out, appends normally.
      // ======================================================================
      { "Standard Local: Explicit relative dir",
         R"(C:\ProgramData\MyApp\)",     R"(..\Cache\thumbnails\)",     R"(C:\ProgramData\Cache\thumbnails\)" },



      // ======================================================================
      // CASE 4: Empty Directory
      // Returns the prefix unaltered.
      // ======================================================================
      { "Empty dir against Standard UNC",
         R"(\\FileServer\Public\Data\)", "",                           R"(\\FileServer\Public\Data\)" },

      // there is a bug in libc++
#ifndef _LIBCPP_VERSION
      { "Empty dir against Long Local",
         R"(\\?\D:\Database\Main)", "",                           R"(\\?\D:\Database\Main\)" },
#endif


      // ======================================================================
      // CASE 5: Absolute Directory
      // Completely overrides the prefix.
      // ======================================================================
      { "Absolute Local overriding UNC prefix",
         R"(\\FileServer\Public\Data)", R"(D:\LocalCache\Temp\)",     R"(D:\LocalCache\Temp\)" },

      { "Absolute UNC overriding Local prefix",
         R"(C:\ProgramData\MyApp)",     R"(\\BackupServer\ColdStorage\)", R"(\\BackupServer\ColdStorage\)" },

      { "Absolute Long UNC overriding Standard UNC",
         R"(\\FileServer\Public)",      R"(\\?\UNC\NAS01\Archive\)",  R"(\\?\UNC\NAS01\Archive\)" },

      // there is a bug in libc++
#ifndef _LIBCPP_VERSION
      { "Absolute Local UNC overriding Standard UNC",
         R"(\\FileServer\Public)",      R"(\\?\E:\NAS01\Archive\)",  R"(\\?\E:\NAS01\Archive\)" }
#endif
   };
#else
   return {

   };
#endif
}

static std::vector<PathTestCase> GetAsciiFileTests(void)
{
#ifdef _WIN32
   return {
      // ======================================================================
      // CASE 1: Root directory without root name (Starts with '\')
      // Appends to the drive or server root of the prefix.
      // ======================================================================
      { "Standard Local: Replaces path, keeps drive",
         R"(C:\ProgramData\MyApp\)",     R"(\Logs\session\test.log)",       R"(C:\Logs\session\test.log)" },

      // ======================================================================
      // CASE 2a: Pure Relative Path
      // Appends normally to the prefix.
      // ======================================================================
      { "Standard Local: Normal append",
         R"(C:\ProgramData\MyApp\)",     R"(Cache\thumbnails\test.log)",       R"(C:\ProgramData\MyApp\Cache\thumbnails\test.log)" },

      // ======================================================================
      // CASE 2b: Relative Path with Dot (e.g. .\dir\test)
      // Resolves the dot out, appends normally.
      // ======================================================================
      { "Standard Local: Explicit current dir",
         R"(C:\ProgramData\MyApp\)",     R"(.\Cache\thumbnails\test.log)",     R"(C:\ProgramData\MyApp\Cache\thumbnails\test.log)" },

      // ======================================================================
      // CASE 2c: Relative Path with double Dot (e.g. ..\dir\test)
      // Resolves the dot out, appends normally.
      // ======================================================================
      { "Standard Local: Explicit relative dir",
         R"(C:\ProgramData\MyApp\)",     R"(..\Cache\thumbnails\test.log)",     R"(C:\ProgramData\Cache\thumbnails\test.log)" },



      // ======================================================================
      // CASE 4: Empty Directory
      // Returns the prefix unaltered.
      // ======================================================================
      { "Empty dir against Standard UNC",
         R"(\\FileServer\Public\Data\)", "test.log",                           R"(\\FileServer\Public\Data\test.log)" },

      // there is a bug in libc++
#ifndef _LIBCPP_VERSION
      { "Empty dir against Long Local",
         R"(\\?\D:\Database\Main\)", "test.log",                           R"(\\?\D:\Database\Main\test.log)" },
#endif


      // ======================================================================
      // CASE 5: Absolute Directory
      // Completely overrides the prefix.
      // ======================================================================
      { "Absolute Local overriding UNC prefix",
         R"(\\FileServer\Public\Data\)", R"(D:\LocalCache\Temp\test.log)",     R"(D:\LocalCache\Temp\test.log)" },

      { "Absolute UNC overriding Local prefix",
         R"(C:\ProgramData\MyApp\)",     R"(\\BackupServer\ColdStorage\test.log)", R"(\\BackupServer\ColdStorage\test.log)" },

      { "Absolute Long UNC overriding Standard UNC",
         R"(\\FileServer\Public\)",      R"(\\?\UNC\NAS01\Archive\test.log)",  R"(\\?\UNC\NAS01\Archive\test.log)" },

      // there is a bug in libc++
#ifndef _LIBCPP_VERSION
      { "Absolute Local UNC overriding Standard UNC",
         R"(\\FileServer\Public\)",      R"(\\?\E:\NAS01\Archive\test.log)",  R"(\\?\E:\NAS01\Archive\test.log)" }
#endif
   };
   #else

   return {};

   #endif
}

static std::vector<PathTestCase> GetAdvancedDirTests(void)
{

#ifdef _WIN32
      return {

      // case 1
      { "Standard UNC: Replaces path, keeps \\server",
         R"(\\FileServer\Public\Data)", R"(\Archive\2026\)",          R"(\\FileServer\Archive\2026\)" },

      { "Standard UNC: Replaces path, keeps \\server",
         R"(\\FileServer)", R"(\Archive\2026\)",          R"(\\FileServer\Archive\2026\)" },

      { "Long UNC: Replaces path, keeps Win32 UNC root",
         R"(\\?\UNC\Server\Share\App)", R"(\Backups\db\)",         R"(\\?\UNC\Server\Backups\db\)" },

      { "Long UNC: Replaces path, keeps Win32 UNC root",
         R"(\\?\UNC\Server)", R"(\Backups\db\)",         R"(\\?\UNC\Server\Backups\db\)" },

      // there is a bug in libc++
#ifndef _LIBCPP_VERSION
      { "Long Local: Replaces path, keeps Win32 drive",
         R"(\\?\C:\ProgramData\MyApp)", R"(\System Volume Info\)",     R"(\\?\C:\System Volume Info\)" },

      { "Long Local: Replaces path, keeps Win32 drive",
         R"(\\?\C:)", R"(\System Volume Info\)",     R"(\\?\C:\System Volume Info\)" },
#endif

      //case 2a
      { "Standard UNC: Normal append",
         R"(\\FileServer\Public)",      R"(Engineering\Specs\)",   R"(\\FileServer\Public\Engineering\Specs\)" },

      { "Standard UNC: Normal append",
         R"(\\FileServer)",      R"(Engineering\Specs\)",   R"(\\FileServer\Engineering\Specs\)" },

      { "Long UNC: Normal append",
         R"(\\?\UNC\Server\Share\App)", R"(Backups\db\)",         R"(\\?\UNC\Server\Share\App\Backups\db\)" },

      { "Long UNC: Normal append",
         R"(\\?\UNC\Server)", R"(Backups\db\)",         R"(\\?\UNC\Server\Backups\db\)" },

      // there is a bug in libc++
#ifndef _LIBCPP_VERSION
      { "Long Local: Normal append",
         R"(\\?\C:\ProgramData\MyApp)", R"(System Volume Info\)",     R"(\\?\C:\ProgramData\MyApp\System Volume Info\)" },

      { "Long Local: Normal append",
         R"(\\?\C:)", R"(System Volume Info\)",     R"(\\?\C:\System Volume Info\)" },
#endif

      { "Standard UNC: Explicit current dir",
         R"(\\FileServer\Public\)",      R"(.\Engineering\Specs\)", R"(\\FileServer\Public\Engineering\Specs\)" },

      { "Standard UNC: Explicit current dir",
         R"(\\FileServer)",      R"(.\Engineering\Specs\)", R"(\\FileServer\Engineering\Specs\)" },

      { "Long UNC: Explicit current dir",
         R"(\\?\FileServer\Public\)",      R"(.\Engineering\Specs\)", R"(\\?\FileServer\Public\Engineering\Specs\)" },

      // there is a bug in libc++
#ifndef _LIBCPP_VERSION
      { "Long Local: Explicit current dir",
         R"(\\?\D:\Database\Main)",     R"(.\Logs\transaction\)",  R"(\\?\D:\Database\Main\Logs\transaction\)" },

      { "Long Local: Explicit current dir",
         R"(\\?\D:)",     R"(.\Logs\transaction\)",  R"(\\?\D:\Logs\transaction\)" },
#endif
   };

#else

   return {};

#endif
}

static bool checkOldVsNewCompleteDir(const struct PathTestCase &tc, int fc, bool keepRelPath)
{
   // First try the old function
   std::string oldRes = CompleteDirEx(std::string_view(tc.prefixDir),
                                      std::string_view(tc.path),
                                      fc,
                                      keepRelPath);


   fs::path newRes = CompleteDirEx(fs::path(tc.prefixDir),
                                   fs::path(tc.path),
                                   fc,
                                   keepRelPath);

   if (fs::path(oldRes) != newRes) {

      std::cerr << std::format("\n\nDifferences detected: fc = {}; keepRelPath = {}\n", fc, keepRelPath);
      std::cerr << "Old function: " << oldRes << "\n";
      std::cerr << "New function: " << newRes.string() << "\n";
      if (fc == 0 && keepRelPath == false) {
         std::cerr << "Expected dir: " << reinterpret_cast<const char *>(tc.expectedPath.u8string().c_str()) << "\n";
      }
      std::cerr << "prefix dir  : " << tc.prefixDir << "\n";
      std::cerr << "input dir   : " << tc.path << "\n";

      return false;
   }

   if (fc == 0 && keepRelPath == false && newRes != tc.expectedPath) {
      std::cerr << std::format("\n\nDifferences detected: fc = {}; keepRelPath = {}\n", fc, keepRelPath);
      std::cerr << "Returned dir: " << reinterpret_cast<const char*>(newRes.u8string().c_str()) << "\n";
      std::cerr << "Expected dir: " << reinterpret_cast<const char*>(tc.expectedPath.u8string().c_str()) << "\n";
      std::cerr << "prefix dir  : " << tc.prefixDir << "\n";
      std::cerr << "input dir   : " << tc.path << "\n";
   }

   return true;
}

static bool checkOldVsNewCompleteFile(const struct PathTestCase &tc, int fc, bool keepRelPath)
{
   // First try the old function
   std::string oldRes = CompleteFileNameEx(tc.prefixDir,
                                           tc.path,
                                           fc,
                                           keepRelPath);


   fs::path newRes = CompleteFileNameEx(fs::path(tc.prefixDir),
                                        fs::path(tc.path),
                                        fc,
                                        keepRelPath);

   if (fs::path(oldRes) != newRes) {

      std::cerr << std::format("\n\nDifferences detected: fc = {}; keepRelPath = {}\n", fc, keepRelPath);
      std::cerr << "Old function: " << oldRes << "\n";
      std::cerr << "New function: " << newRes.string() << "\n";
      if (fc == 0 && keepRelPath == false) {
         std::cerr << "Expected file: " << reinterpret_cast<const char *>(tc.expectedPath.u8string().c_str()) << "\n";
      }
      std::cerr << "prefix dir  : " << tc.prefixDir << "\n";
      std::cerr << "input  file : " << tc.path << "\n";

      return false;
   }

   if (fc == 0 && keepRelPath == false && newRes != tc.expectedPath) {
      std::cerr << std::format("\n\nDifferences detected: fc = {}; keepRelPath = {}\n", fc, keepRelPath);
      std::cerr << "Returned file: " << reinterpret_cast<const char*>(newRes.u8string().c_str()) << "\n";
      std::cerr << "Expected file: " << reinterpret_cast<const char*>(tc.expectedPath.u8string().c_str()) << "\n";
      std::cerr << "prefix file  : " << tc.prefixDir << "\n";
      std::cerr << "input file   : " << tc.path << "\n";
   }
   return true;
}

[[maybe_unused]] static bool checkNewCompleteDir(const struct PathTestCase &tc, int fc, bool keepRelPath)
{
   fs::path output = CompleteDirEx(fs::path(tc.prefixDir),
                                   fs::path(tc.path),
                                   fc,
                                   keepRelPath);

   if (fc == 0 && keepRelPath == false && output != tc.expectedPath) {

      std::cerr << std::format("\n\nDifferences detected: fc = {}; keepRelPath = {}\n", fc, keepRelPath);
      std::cerr << "Returned dir: " << reinterpret_cast<const char*>(output.u8string().c_str()) << "\n";
      std::cerr << "Expected dir: " << reinterpret_cast<const char*>(tc.expectedPath.u8string().c_str()) << "\n";
      std::cerr << "prefix dir  : " << tc.prefixDir << "\n";
      std::cerr << "input dir   : " << tc.path << "\n";

      return false;
   }

   return true;
}

[[maybe_unused]] static bool checkNewCompleteFile(const struct PathTestCase &tc, int fc, bool keepRelPath)
{
   fs::path output = CompleteFileNameEx(fs::path(tc.prefixDir),
                                        fs::path(tc.path),
                                        fc,
                                        keepRelPath);

   if (fc == 0 && keepRelPath == false && output != tc.expectedPath) {

      std::cerr << std::format("\n\nDifferences detected: fc = {}; keepRelPath = {}\n", fc, keepRelPath);
      std::cerr << "Returned file: " << reinterpret_cast<const char*>(output.u8string().c_str()) << "\n";
      std::cerr << "Expected file: " << reinterpret_cast<const char*>(tc.expectedPath.u8string().c_str()) << "\n";
      std::cerr << "prefix dir   : " << tc.prefixDir << "\n";
      std::cerr << "input file   : " << tc.path << "\n";

      return false;
   }

   return true;
}

TEST_CASE("Test completeDirEx")
{
   size_t nerr = 0;

   for (const auto &tc : GetAsciiDirTests()) {

      for (auto fc : std::views::iota(0, 5)) {

         for ( bool keepRelPath : { false, true }) {

            nerr += !checkOldVsNewCompleteDir(tc, fc, keepRelPath);

         }

      }
   }

   for (const auto &tc : GetAsciiFileTests()) {

      for (auto fc : std::views::iota(0, 5)) {

         for ( bool keepRelPath : { false, true }) {

            nerr += !checkOldVsNewCompleteFile(tc, fc, keepRelPath);

         }

      }
   }
#ifdef _WIN32
   // ======================================================================
   // CASE 3: Drive-Relative Path (Windows quirk: Drive letter, no slash)
   // Resolves against the current working directory of that specific drive.
   // ======================================================================
   //
   struct PathTestCase local_prefix[] = {

      { "Drive-relative overriding local prefix",
         R"(C:\ProgramData\MyApp)",     R"(C:db_dump)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\FileServer\Public)",      R"(C:db_dump)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\?\UNC\FileServer\Public)",      R"(C:db_dump)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\?\E:\test\dir)",      R"(C:db_dump)",           "" },

      { "Drive-relative overriding local prefix",
         R"(C:\ProgramData\MyApp)",     R"(C:.\db_dump)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\FileServer\Public)",      R"(C:.\db_dump)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\?\UNC\FileServer\Public)",      R"(C:.\db_dump)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\?\E:\test\dir)",      R"(C:.\db_dump)",           "" },

      { "Drive-relative overriding local prefix",
         R"(C:\ProgramData\MyApp)",     R"(C:tmp\db_dump)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\FileServer\Public)",      R"(C:tmp\db_dump)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\?\UNC\FileServer\Public)",      R"(C:tmp\db_dump)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\?\E:\test\dir)",      R"(C:tmp\db_dump)",           "" },

// check with LLVM >=23
#ifndef _LIBCPP_VERSION
      { "Drive-relative overriding local prefix",
         R"(C:\ProgramData\MyApp)",     R"(C:..\db_dump)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\FileServer\Public)",      R"(C:..\db_dump)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\?\UNC\FileServer\Public)",      R"(C:..\db_dump)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\?\E:\test\dir)",      R"(C:..\db_dump)",           "" },
#endif
/* This is broken in the old code
Differences detected: fc = 0; keepRelPath = false
Old function: C:\home\nb\ohuber\cppmex\db_dump\
New function: C:\home\nb\ohuber\db_dump\
Expected dir: C:\home\nb\ohuber\db_dump
prefix dir  : C:\ProgramData\MyApp
input dir   : C:..\db_dump
*/
   };

      struct PathTestCase local_prefix_file[] = {
      { "Drive-relative overriding local prefix",
         R"(C:\ProgramData\MyApp)",     R"(C:db_dump.log)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\FileServer\Public)",      R"(C:db_dump.log)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\?\UNC\FileServer\Public)",      R"(C:db_dump.log)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\?\E:\test\dir)",      R"(C:db_dump.log)",           "" },

      { "Drive-relative overriding local prefix",
         R"(C:\ProgramData\MyApp)",     R"(C:.\db_dump.log)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\FileServer\Public)",      R"(C:.\db_dump.log)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\?\UNC\FileServer\Public)",      R"(C:.\db_dump.log)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\?\E:\test\dir)",      R"(C:.\db_dump.log)",           "" },


#ifndef _LIBCPP_VERSION
      { "Drive-relative overriding local prefix",
         R"(C:\ProgramData\MyApp)",     R"(C:..\db_dump.log)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\FileServer\Public)",      R"(C:..\db_dump.log)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\?\UNC\FileServer\Public)",      R"(C:..\db_dump.log)",           "" },

      { "Drive-relative overriding UNC prefix",
         R"(\\?\E:\test\dir)",      R"(C:..\db_dump.log)",           "" },
#endif
/* the old code is broken
Differences detected: fc = 0; keepRelPath = false
Old function: C:\home\nb\ohuber\cppmex\db_dump.log
New function: C:\home\nb\ohuber\db_dump.log
Expected file: C:\home\nb\ohuber\db_dump.log
*/
   };

   fs::path curDir = fs::current_path();

   for (unsigned i = 0, len = sizeof(local_prefix) / sizeof(*local_prefix); i < len; ++i) {
      struct PathTestCase &tc = local_prefix[i];
      tc.expectedPath = fs::absolute(tc.path) / "";

      for (auto fc : std::views::iota(0, 5)) {


         if (i < len-4) {
            nerr += !checkOldVsNewCompleteDir(tc, fc, false);
         } else {
            nerr += !checkNewCompleteDir(tc, fc, false);
         }

      }

   }

   // If the drive has no wd, then it is not too meaningful
   if (!fs::absolute(L"C:").relative_path().empty()) {

      for (unsigned i = 0, len = sizeof(local_prefix_file) / sizeof(*local_prefix_file); i < len; ++i) {
         struct PathTestCase &tc = local_prefix_file[i];
         tc.expectedPath = fs::absolute(tc.path);

         for (auto fc : std::views::iota(0, 5)) {

         if (i < len-4) {
            nerr += !checkOldVsNewCompleteFile(tc, fc, false);
         } else {
            nerr += !checkNewCompleteFile(tc, fc, false);
         }

         }

      }

   }

#endif

   for (const auto &tc : GetAdvancedDirTests()) {

      for (auto fc : std::views::iota(0, 5)) {

         for ( bool keepRelPath : { false, true }) {

            nerr += !checkNewCompleteDir(tc, fc, keepRelPath);

         }

      }

   }

   REQUIRE_FALSE(nerr);
}

#endif //C++20

TEST_SUITE_END();

}
