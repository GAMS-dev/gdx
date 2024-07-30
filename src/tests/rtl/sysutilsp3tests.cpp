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

#include "rtl/sysutils_p3.h"
#include "gdlib/utils.h"
#include "../doctest.h"
#include <string>
#include <chrono>
#include <filesystem>

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
   const auto locTime {std::localtime(&tnow)};
   REQUIRE_EQ(locTime->tm_year + 1900, year);
   REQUIRE_EQ(locTime->tm_mon + 1, month);
   REQUIRE_EQ(locTime->tm_mday, day);
   double lastDayOfYear;
   tryEncodeDate(2024, 12, 31, lastDayOfYear);
   DecodeDate(lastDayOfYear, year, month, day);
   REQUIRE_EQ(2024, year);
   REQUIRE_EQ(12, month);
   REQUIRE_EQ(31, day);
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
   constexpr int nfiles {9};
   for(int i{}; i<nfiles; i++)
   {
      const auto fn {"abc"s + std::to_string( i+1 ) + ".txt"s};
      std::ofstream ofs{fn};
      REQUIRE(std::filesystem::is_regular_file( fn ));
   }
   TSearchRec F;
   REQUIRE_EQ(0, FindFirst( "abc*.txt", faAnyFile, F ));
   std::vector<std::string> collectedFiles {F.Name};
   while(!FindNext( F ))
      collectedFiles.push_back( F.Name );
   REQUIRE_EQ(nfiles, collectedFiles.size());
   for(int i{}; i<nfiles; i++)
   {
      const auto fn {"abc"s + std::to_string( i+1 ) + ".txt"s};
      REQUIRE_EQ(fn, collectedFiles[i]);
      std::filesystem::remove( fn );
   }
}

TEST_SUITE_END();

}
