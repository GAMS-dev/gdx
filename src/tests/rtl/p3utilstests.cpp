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

#include "rtl/p3utils.hpp"
#include "../doctest.hpp"
#include <string>

using namespace std::literals::string_literals;

using namespace rtl::p3utils;

namespace tests::rtltests::p3utilstests
{

TEST_SUITE_BEGIN( "rtl::p3utils" );

TEST_CASE( "Test get digits" )
{
   REQUIRE_EQ( "23"s, getDigits( 23 ) );
}

TEST_SUITE_END();

}
