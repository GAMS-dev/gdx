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

#include <string>
#include <numeric>
#include <utility>

#include "../doctest.h"

#include "gdlib/glookup.h"
#include "gdlib/strutilx.h"

using namespace std::literals::string_literals;

using namespace gdlib::glookup;
using namespace gdlib::strutilx;

namespace tests::gdlibtests::glookuptests
{

TEST_SUITE_BEGIN( "gdlib::glookup" );

static int numItemCreat{}, numItemDestroy{};

struct Item {
   std::string s;
   int nextBucketIx {};

   explicit Item(std::string _s) : s{std::move(_s)} {
      numItemCreat++;
   }

   ~Item() {
      numItemDestroy++;
   }
};

class MyMapping : public TGAMSRecList<Item>
{
   std::pair<gdlib::strutilx::DelphiStrRef, int *> AccessRecord( Item *prec ) override
   {
      gdlib::strutilx::DelphiStrRef dsr {};
      dsr.length = static_cast<int>(prec->s.length());
      dsr.chars = const_cast<char *>( prec->s.c_str() );
      return { dsr, &prec->nextBucketIx };
   }
};

TEST_CASE( "Adding some set element names that are mapped to numbers" )
{
   numItemCreat = numItemDestroy = 0;
   {
      MyMapping grlst;
      for( int i {}; i < 10; i++ )
      {
         Item *nitem {
            new(gmsonly::gheap.XAllocMem<Item>()) Item {
               "i"s + std::to_string( i + 1 )
            }
         };
         const int ix { grlst.AddItem( nitem ) };
         REQUIRE_EQ( i + 1, ix );
      }
      Item *item { grlst.Find( "i3"s ) };
      REQUIRE_EQ( "i3"s, item->s );
   }
   REQUIRE_EQ(numItemCreat, numItemDestroy);
}

TEST_CASE( "Simple usage of TBucketArray" )
{
   gdlib::gmsheapnew::THeapMgr myheap { "MiniHeap"s };
   TBucketArray<int> nums { myheap, sizeof( int ) };
   REQUIRE_EQ( 0, nums.GetCount() );
   REQUIRE_EQ( 0, nums.MemoryUsed() );
   int *firstElem = nums.AddItem();
   *firstElem = 23;
   int *secondElem = nums.AddItem();
   *secondElem = 42;
   int *thirdElem = nums.AddItem();
   *thirdElem = 1337;
   int *fourthElem = nums.AddItem();
   *fourthElem = 87;
   REQUIRE_GT( nums.MemoryUsed(), 0 );
   REQUIRE_EQ( 4, nums.GetCount() );
   REQUIRE_EQ( 23, *nums[0] );
   REQUIRE_EQ( 42, *nums[1] );
   REQUIRE_EQ( 1337, *nums[2] );
   REQUIRE_EQ( 87, *nums[3] );
   nums.DeleteLast();
   REQUIRE_EQ( 3, nums.GetCount() );
   REQUIRE_EQ( 23, *nums[0] );
   REQUIRE_EQ( 42, *nums[1] );
   REQUIRE_EQ( 1337, *nums[2] );
   nums.DeleteAtEnd( 2 );
   REQUIRE_EQ( 1, nums.GetCount() );
   REQUIRE_EQ( 23, *nums[0] );
   nums.Clear();
   REQUIRE_EQ( 0, nums.GetCount() );
}

TEST_CASE( "Simple usage of TBucketPtrArray" )
{
   gdlib::gmsheapnew::THeapMgr myheap { "MiniHeap"s };
   TBucketPtrArray<int> numPtrs { myheap };
   std::array numStorage { 23, 42, 1337, 87 };
   int expIx {};
   for( int &num: numStorage )
      REQUIRE_EQ( expIx++, numPtrs.AddItem( &num ) );
   REQUIRE_EQ( numStorage.size(), numPtrs.GetCount() );
   for( int i {}; i < (int)numStorage.size(); i++ )
      REQUIRE_EQ( numStorage[i], *numPtrs.GetItem( i ) );
}

struct ItemLegacy {
   std::array<char, 256> s {};
   ItemLegacy *nextBucketPtr {};
};

class MyMappingLegacy : public TGAMSRecListLegacy<ItemLegacy>
{
public:
   explicit MyMappingLegacy( gdlib::gmsheapnew::THeapMgr &heap ) : TGAMSRecListLegacy<ItemLegacy>( heap ) {}

protected:
   std::pair<char *, ItemLegacy **> AccessRecord( ItemLegacy *prec ) override
   {
      return { prec->s.data(), &prec->nextBucketPtr };
   }
};

TEST_CASE( "Simple usage of TGMSRecListLegacy" )
{
   gdlib::gmsheapnew::THeapMgr myheap { "MiniHeap"s };
   std::array<ItemLegacy, 10> items {};
   MyMappingLegacy grlst { myheap };
   for( int i {}; i < (int)items.size(); i++ )
   {
      std::string s { "i"s + std::to_string( i + 1 ) };
      std::memcpy( items[i].s.data(), s.c_str(), s.length() + 1 );
      const int ix = grlst.AddItem( &items[i] );
      REQUIRE_EQ( i + 1, ix );
   }
   REQUIRE_FALSE( grlst.Find( "doesNotExist" ) );
}

TEST_SUITE_END();

}// namespace tests::glookuptests
