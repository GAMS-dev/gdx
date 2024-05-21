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

#include <array>
#include <numeric>
#include <string>

#include "gdlib/strhash.h"
#include "tests/doctest.h"

using namespace std::literals::string_literals;
using namespace gdlib::strhash;

namespace tests::gdlibtests::strhashtests
{

TEST_SUITE_BEGIN( "gdlib::strhash" );

template<typename T>
void runTest( const std::array<int, 10> &nums )
{
   T shlst;
   // 0-based
   for( const int &n: nums )
   {
      auto s { "i" + std::to_string( n ) };
      REQUIRE_EQ( n - 1, shlst.AddObject( s.c_str(), s.length(), n ) );
   }
   REQUIRE_EQ( 2, shlst.IndexOf( "i3" ) );
};

TEST_CASE( "Adding some set element names that are mapped to numbers" )
{
   std::array<int, 10> nums {};
   std::iota( nums.begin(), nums.end(), 1 );
   runTest<TXStrHashList<int>>( nums );
}

TEST_SUITE_END();

}
