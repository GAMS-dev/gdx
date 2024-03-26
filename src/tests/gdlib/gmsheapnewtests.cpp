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

#include <array>                   // for array
#include <cstring>                 // for memcpy
#include <string>                  // for operator""s, string_literals
#include "gdlib/gmsheapnew.h"// for THeapMgr, gmsheapnew
#include "../doctest.h"            // for ResultBuilder, TestCase, REQUIRE_EQ, TEST...

using namespace std::literals::string_literals;
using namespace gdlib::gmsheapnew;

namespace tests::gdlibtests::gmsheapnew
{

TEST_SUITE_BEGIN( "gmsheapnew" );

struct MyRec {
   std::array<uint8_t, 48> data;
};

TEST_CASE( "Test simple usage of the custom heap" )
{
   THeapMgr thm { "heapmgr-test"s };
   const auto data { "hej" };
   constexpr int l { 4 };
   void *buf = thm.XGetMem( l );
   std::memcpy( buf, data, l );
   thm.XFreeMem( buf, l );
   constexpr int n { 10000 };
   std::array<void *, n> ptrs {};
   std::array<uint8_t, 20> buf2 {};
   const auto ref { reinterpret_cast<int64_t *>( buf2.data() ) };
   for( int i {}; i < n; i++ )
   {
      ptrs[i] = thm.XGetMem( (int) buf2.size() );
      *ref = static_cast<int64_t>( i );
      std::memcpy( ptrs[i], buf2.data(), buf2.size() );
   }
   for( int i {}; i < n; i++ )
   {
      REQUIRE_EQ( *static_cast<int64_t *>( ptrs[i] ), static_cast<int64_t>( i ) );
      thm.XFreeMem( ptrs[i], (int) buf2.size() );
   }
   
   // this is close to what new* and disp(ose)* are doing
   constexpr int recCnt { 100 };
   constexpr auto recSlot = ( sizeof( MyRec ) - 1 ) / 8 + 1;
   std::array<MyRec*, recCnt> recPtrs {};
   for(int i{}; i<recCnt; i++)
      recPtrs[i] = static_cast<MyRec *>(thm.GMSGetMem( recSlot ));
   for(int i{}; i<recCnt; i++)
      thm.GMSFreeMem(recPtrs[i], recSlot );
}

TEST_SUITE_END();

}// namespace tests::gdlib::gmsheapnew
