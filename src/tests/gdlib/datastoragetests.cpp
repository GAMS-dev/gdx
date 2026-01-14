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

#include <string>
#include <vector>
#include <array>
#include "../doctest.hpp"
#include "datastorage.hpp"

using namespace std::literals::string_literals;
using namespace gdlib::datastorage;

namespace tests::gdlibtests::datastoragetests
{

TEST_SUITE_BEGIN( "gdlib::datastorage" );

TEST_CASE( "Simple use of linked data" )
{
   TLinkedData<int, double> ld { 2, 1 * (int) sizeof( double ) };
   std::vector<int> keys {0, 0};
   std::array<double, 1> vals { 23.0 };
   for( int i {}; i < 4; i++ )
   {
      keys.front() = 4 - i;// insert in reverse order
      auto node = ld.AddItem( keys.data(), vals.data() );
      REQUIRE_EQ( keys.front(), ( reinterpret_cast<int *>( node->RecData ) )[0] );
      REQUIRE_EQ( 0, ( reinterpret_cast<int *>( node->RecData ) )[1] );
      REQUIRE_EQ( 23.0, *( reinterpret_cast<double *>( &node->RecData[(int) sizeof( int ) * 2] ) ) );
   }
   REQUIRE_EQ( 4, ld.Count() );
   auto it = ld.StartRead();// this calls sort!
   for( int i {}; i < 4; i++ )
   {
      bool res = ld.GetNextRecord( &it.value(), keys.data(), vals.data() );
      REQUIRE( ( i == 3 || res ) );
      REQUIRE_EQ( i + 1, keys.front() );
   }
}

TEST_SUITE_END();

}// namespace tests::datastoragetests
