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

#include "gclgms.h" // for GLOBAL_MAX_INDEX_DIM
#include "gdlib/gmsdata.h"// for TTblGamsData, TTblGams...
#include "tests/doctest.h"   // for ResultBuilder, REQUIRE_EQ
#include <algorithm>   // for fill
#include <array>       // for array

using namespace std::literals::string_literals;
using namespace gdlib::gmsdata;

namespace tests::gdlibtests::gmsdatatests
{
TEST_SUITE_BEGIN( "gdx::collections::gmsdata" );

TEST_CASE( "Test basic usage of TTblGamsData" )
{
   TTblGamsData<double> gdl { 1, sizeof( double ) * 2 };
   REQUIRE_EQ( 1, gdl.GetDimension() );
   REQUIRE_EQ( 0, gdl.GetCount() );

   std::array<int, GLOBAL_MAX_INDEX_DIM> keys {};
   std::array<double, 2> vals {};

   vals.front() = 23.0;
   for( int i {}; i < 10; i++ )
   {
      keys.front() = i + 1;
      gdl.AddRecord( keys.data(), vals.data() );
   }
   REQUIRE_EQ( 10, gdl.GetCount() );
   REQUIRE_GT( gdl.MemoryUsed(), 0 );

   std::fill( keys.begin(), keys.end(), 0 );
   std::fill( vals.begin(), vals.end(), 0 );

   for( int i {}; i < 10; i++ )
   {
      gdl.GetRecord( i, keys.data(), vals.data() );
      REQUIRE_EQ( i + 1, keys.front() );
      REQUIRE_EQ( 23.0, vals.front() );
      std::fill( keys.begin(), keys.end(), 0 );
      gdl.GetKeys( i, keys.data() );
      REQUIRE_EQ( i + 1, keys.front() );
      std::fill( vals.begin(), vals.end(), 0 );
      gdl.GetData( i, vals.data() );
      REQUIRE_EQ( 23.0, vals.front() );
      REQUIRE_EQ( 23.0, static_cast<double *>( gdl.GetDataPtr( i ) )[0] );
   }

   gdl.Clear();
   REQUIRE_EQ( 0, gdl.GetCount() );

   std::fill( keys.begin(), keys.end(), 0 );
   std::fill( vals.begin(), vals.end(), 0 );
   vals.front() = 23.0;

   for( int i { 9 }; i >= 0; i-- )
   {
      keys.front() = i + 1;
      gdl.AddRecord( keys.data(), vals.data() );
   }
   gdl.Sort();
   REQUIRE_EQ( 10, gdl.GetCount() );
   for( int i {}; i < 10; i++ )
   {
      gdl.GetRecord( i, keys.data(), vals.data() );
      REQUIRE_EQ( i + 1, keys.front() );
      REQUIRE_EQ( 23.0, vals.front() );
   }
}

TEST_SUITE_END();

}
