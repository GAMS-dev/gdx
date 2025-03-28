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

#include <string>
#include <array>
#include <iostream>
#include <limits>

#include "p3io.hpp"
#include "../doctest.hpp"

using namespace std::literals::string_literals;
using namespace rtl::p3io;

namespace tests::rtltests::p3iotests
{

TEST_SUITE_BEGIN( "rtl::p3iotests" );

TEST_CASE("Test P3_Str_dd0") {
   std::array<char, 64> buf {};
   size_t len{};
   P3_Str_dd0(std::numeric_limits<double>::quiet_NaN(), buf.data(), static_cast<uint8_t>(buf.size()), &len);
   REQUIRE_EQ("                    Nan"s, buf.data());
   REQUIRE_EQ(0, len);
   P3_Str_dd0(23.42, buf.data(), static_cast<uint8_t>(buf.size()), &len);
   REQUIRE_EQ(" 2.34200000000000E+0001"s, buf.data());
   REQUIRE_EQ(23, len);
}

TEST_CASE( "Test dig2Exp" )
{
   std::array<char, 256> buf {};
   size_t outLen;
   dig2Exp("2342", 4, 2, 0, 23, 15, buf.data(), &outLen);
   REQUIRE_EQ(" 2.34200000000000E+0001"s, buf.data());
}

TEST_SUITE_END();

}
