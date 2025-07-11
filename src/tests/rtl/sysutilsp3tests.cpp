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

#include "sysutils_p3.hpp"
#include "utils.hpp"
#include "../doctest.hpp"
#include <string>
#include <chrono>
#include <filesystem>
#include <algorithm>
#include <fstream>

#if defined(_WIN32)
#include <windows.h>
#undef max
#endif


using namespace std::literals::string_literals;
using namespace rtl::sysutils_p3;

namespace tests::rtltests::sysutilsp3tests
{

TEST_SUITE_BEGIN( "rtl::sysutils_p3" );

#if 0
TEST_CASE( "Extracting Windows short path name" )
{
   const std::string path {R"(C:\Program Files\Application Verifier)"};
   REQUIRE_EQ("C:\\PROGRA~1\\APPLIC~1"s, ExtractShortPathName( path ));
}
#endif

TEST_CASE( "Remove trailing path delimiter from path" )
{
   char sep = '/';
#if defined( _WIN32 )
   sep = '\\';
#endif
   const auto base = utils::join( sep, { "some", "path", "with", "trailing", "path", "delim" } );
   const auto actual = ExcludeTrailingPathDelimiter( base + sep );
   REQUIRE_EQ( base, actual );
}

TEST_CASE( "Index of last delimiter in path" )
{
   REQUIRE_EQ( 5, LastDelimiter( "/\\"s, "/some/path"s ) );
   REQUIRE_EQ( 15, LastDelimiter( "/\\."s, "/some/path/file.txt"s ) );
   REQUIRE_EQ( 5, LastDelimiter( "/\\"s, "/some\\path"s ) );
   REQUIRE_EQ( -1, LastDelimiter( "/", "C:\\some\\path" ) );
}

TEST_CASE( "Extract extension of filename" )
{
   REQUIRE_EQ( ".pdf", ExtractFileExt( "xyz.pdf" ) );
   REQUIRE( ExtractFileExt( "xyz" ).empty() );
}

TEST_CASE( "Change a file extension")
{
   REQUIRE_EQ("abc.cpp"s, ChangeFileExt("abc"s, ".cpp"s));
   REQUIRE_EQ("abc.cpp"s, ChangeFileExt("abc.txt"s, ".cpp"s));
}

TEST_CASE("Test extracting the path from an full filename path")
{
#if defined(_WIN32)
   REQUIRE_EQ("C:\\home\\username\\"s, ExtractFilePath("C:\\home\\username\\xyz.gms"s));
   REQUIRE_EQ("C:\\home\\username\\"s, ExtractFilePath("C:\\home\\username\\"s));
   REQUIRE_EQ("C:\\home\\"s, ExtractFilePath("C:\\home\\username"s));
#else
   REQUIRE_EQ("/home/username/"s, ExtractFilePath("/home/username/xyz.gms"));
   REQUIRE_EQ("/home/username/"s, ExtractFilePath("/home/username/"));
   REQUIRE_EQ("/home/"s, ExtractFilePath("/home/username"));
#endif
}

TEST_CASE("Test filename from a path")
{
#if defined(_WIN32)
   REQUIRE_EQ("xyz.gms"s, ExtractFileName("C:\\home\\username\\xyz.gms"));
   REQUIRE_EQ("xyz.gms"s, ExtractFileName("C:\\home\\..\\xyz.gms"));
   REQUIRE_EQ("xyz.gms"s, ExtractFileName("C:\\home\\xyz.gms"));
#else
   REQUIRE_EQ("xyz.gms"s, ExtractFileName("/home/username/xyz.gms"));
#endif
}

TEST_CASE( "Test decoding a date" )
{
   const auto now {Now()};
   uint16_t year, month, day;
   DecodeDate(now, year, month, day);
   const auto nowRef {std::chrono::system_clock::now()};
   const std::time_t tnow {std::chrono::system_clock::to_time_t(nowRef)};
   {
      const auto locTime {std::localtime(&tnow)};
      REQUIRE_EQ(locTime->tm_year + 1900, year);
      REQUIRE_EQ(locTime->tm_mon + 1, month);
      REQUIRE_EQ(locTime->tm_mday, day);
   }
   {
      double lastDayOfYear;
      tryEncodeDate(2024, 12, 31, lastDayOfYear);
      DecodeDate(lastDayOfYear, year, month, day);
   }
   REQUIRE_EQ(2024, year);
   REQUIRE_EQ(12, month);
   REQUIRE_EQ(31, day);
}

TEST_CASE( "Test encoding and then decoding a date (roundtrip)")
{
   uint16_t year, month, day;
   DecodeDate( EncodeDate(2024, 8, 6), year, month, day );
   REQUIRE_EQ(2024, year);
   REQUIRE_EQ(8, month);
   REQUIRE_EQ(6, day);
}

TEST_CASE( "Test encoding and then decoding a time (roundtrip)")
{
   uint16_t h, m, s, ms;
   DecodeTime( EncodeTime( 10, 52, 23, 42), h, m, s, ms);
   REQUIRE_EQ(10, h);
   REQUIRE_EQ(52, m);
   REQUIRE_EQ(23, s);
   REQUIRE_EQ(42, ms);
}

TEST_CASE( "Test conversion between datetime and filedate")
{
   const double dt {EncodeDate( 1987, 12, 11)};
   const int fd {DateTimeToFileDate( dt )};
   const double dtRt {FileDateToDateTime( fd )};
   REQUIRE_EQ(dtRt, dt);
}

TEST_CASE("Test integer to string conversion")
{
   REQUIRE_EQ("23"s, IntToStr( 23 ));
   REQUIRE_EQ("1024"s, IntToStr( 1024 ));
   REQUIRE_EQ(std::to_string(std::numeric_limits<int>::max()), IntToStr( std::numeric_limits<int>::max() ));
   REQUIRE_EQ("-23"s, IntToStr( -23 ));
   REQUIRE_EQ("0"s, IntToStr( 0 ));
   std::array<char, 256> buf {};
   size_t len {};
   IntToStr( 23, buf.data(), len );
   REQUIRE_EQ(2, len);
   REQUIRE_EQ(buf.front(), '2');
   REQUIRE_EQ(buf[1], '3');
   REQUIRE_EQ(buf[2], '\0');
   for(int i{}; i<123; i++)
      REQUIRE_EQ(std::to_string(i), IntToStr(i));
}

TEST_CASE("Test Find{First,Next,Close}")
{
   constexpr int nfiles {10};
   for(int i{}; i<nfiles; i++)
   {
      const auto fn {"abc"s + std::to_string( i+1 ) + ".txt"s};
      std::ofstream ofs{fn};
      REQUIRE(std::filesystem::is_regular_file( fn ));
   }
   TSearchRec F;
   REQUIRE_EQ(0, FindFirst( "abc*.txt", faAnyFile, F ));
   std::vector collectedFiles {F.Name};
   while(!FindNext( F ))
      collectedFiles.push_back( F.Name );
   REQUIRE_EQ(nfiles, collectedFiles.size());
   for(int i{}; i<nfiles; i++)
   {
      const auto fn {"abc"s + std::to_string( i+1 ) + ".txt"s};
      REQUIRE(std::find(collectedFiles.begin(), collectedFiles.end(), fn) != collectedFiles.end());
      std::filesystem::remove( fn );
   }
}

TEST_CASE( "Test file exists" )
{
   std::string foldername( 100, '\0' );
   for( int i {}; i < static_cast<int>( foldername.length() ); i++ )
      foldername[i] = static_cast<char>( '0' + i % 10 );

#if defined( _WIN32 )
   constexpr char sep {'\\'};
   std::array<char, MAX_PATH> tmpDirBuf;
   GetTempPathA(tmpDirBuf.size(), tmpDirBuf.data());
   const std::string pathRoot = "\\\\?\\"s + tmpDirBuf.data();
#else
   constexpr char sep {'/'};
   #if defined( __APPLE__ )
   const std::string pathRoot = ""s + getenv( "TMPDIR" ) + '/';
   #else
   const std::string pathRoot = "/tmp/"s;
   #endif
#endif

   const std::string fileLocation {pathRoot + foldername + sep + foldername};
   const std::string gdxFn { fileLocation + sep + foldername + ".gdx"s };

   std::filesystem::create_directories( fileLocation );
   std::ofstream ofs { gdxFn };
   ofs.close();
   REQUIRE( FileExists( gdxFn ) );

   std::filesystem::remove_all( foldername );
   REQUIRE_FALSE( FileExists( foldername ) );
}

TEST_SUITE_END();

}
