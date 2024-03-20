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

using namespace std::literals::string_literals;
using namespace rtl::sysutils_p3;

namespace tests::rtltests::sysutilsp3tests
{

TEST_SUITE_BEGIN( "rtl::sysutils_p3" );

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
   REQUIRE_EQ( 5, LastDelimiter( "/\\"s, "/some\\path"s ) );
   REQUIRE_EQ( -1, LastDelimiter( "/", "C:\\some\\path" ) );
}

TEST_CASE( "Extract extension of filename" )
{
   REQUIRE_EQ( ".pdf", ExtractFileExt( "xyz.pdf" ) );
   REQUIRE( ExtractFileExt( "xyz" ).empty() );
}

TEST_SUITE_END();

}
