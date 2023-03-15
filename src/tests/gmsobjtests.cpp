/*
 * GAMS - General Algebraic Modeling System GDX API
 *
 * Copyright (c) 2017-2023 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2023 GAMS Development Corp. <support@gams.com>
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

#include "../gmsobj.h"// for TXStrPool, TBooleanBitArray, TXList, TXCustom...
#include "doctest.h"  // for ResultBuilder, REQUIRE_EQ, Expression_lhs
#include <algorithm>  // for find
#include <array>      // for array
#include <cstdint>    // for uint8_t
#include <cstring>    // for strcmp
#include <list>       // for list, operator!=, _List_iterator
#include <string>     // for operator+, to_string, string, operator""s
#include <vector>     // for vector, vector<>::reference

#include "../strhash.h"
#include <chrono>
#include <iostream>
#include <random>

using namespace std::literals::string_literals;
using namespace gdx::collections::gmsobj;

namespace gdx::tests::gmsobjtests
{

TEST_SUITE_BEGIN( "gdx::collections::gmsobj" );

static std::vector<bool> asBoolVec( const TBooleanBitArray &bba )
{
   std::vector<bool> res( bba.GetHighIndex() + 1 );
   for( int i{}; i < static_cast<int>( res.size() ); i++ )
      res[i] = bba.GetBit( i );
   return res;
}

TEST_CASE( "Simple use TBooleanBitArray" )
{
   TBooleanBitArray bba;
   for( int i{ -2 }; i < 4; i++ )
      REQUIRE_FALSE( bba.GetBit( i ) );
   REQUIRE_EQ( 0, bba.MemoryUsed() );
   bba.SetBit( 3, true );
   REQUIRE( bba.MemoryUsed() > 0 );
   REQUIRE( bba.GetBit( 3 ) );
   REQUIRE_EQ( 3, bba.GetHighIndex() );
   for( int i{ -2 }; i < 3; i++ )
      REQUIRE_FALSE( bba.GetBit( i ) );
   for( int i{ 4 }; i < 6; i++ )
      REQUIRE_FALSE( bba.GetBit( i ) );
   bba.SetHighIndex( 4 );
   REQUIRE_FALSE( bba.GetBit( 4 ) );
   auto oldMem = bba.MemoryUsed();
   bba.SetBit( 4, true );
   REQUIRE_EQ( bba.MemoryUsed(), oldMem );
   auto vec = asBoolVec( bba );
   REQUIRE_EQ( 5, vec.size() );
   std::vector<bool> expectedVec{ false, false, false, true, true };
   for( int i{}; i < 5; i++ )
      REQUIRE_EQ( expectedVec[i], vec[i] );
   // Make sure we need a second alloc
   oldMem = bba.MemoryUsed();
   int highIndex = bba.MemoryUsed() * 8 + 10;
   bba.SetBit( highIndex, true );
   REQUIRE( bba.GetBit( 3 ) );
   REQUIRE( bba.GetBit( 4 ) );
   REQUIRE( bba.GetBit( highIndex ) );
   REQUIRE( bba.MemoryUsed() > oldMem );
}

TEST_CASE( "Simple use of TXList" )
{
   TXList<int> lst;
   std::array<int, 23> nums{};
   for( int i = 0; i < (int) nums.size(); i++ )
   {
      nums[i] = i + 1;
      REQUIRE_EQ( i, lst.Add( &nums[i] ) );
   }
   REQUIRE_EQ( nums.size(), lst.GetCount() );
   REQUIRE( lst.MemoryUsed() > 0 );
   for( int i = 0; i < lst.GetCount(); i++ )
      REQUIRE_EQ( &nums[i], lst[i] );
   REQUIRE_EQ( &nums.back(), lst.GetLast() );

   lst.Delete( 0 );
   REQUIRE_EQ( 22, lst.GetCount() );
   REQUIRE_EQ( 2, *lst[0] );
   lst.Insert( 0, &nums.front() );
   REQUIRE_EQ( 23, lst.GetCount() );
   REQUIRE_EQ( 1, *lst[0] );

   lst.Insert( 10, &nums.front() );
   REQUIRE_EQ( 24, lst.GetCount() );
   REQUIRE_EQ( 1, *lst[10] );
   REQUIRE_EQ( lst[0], lst[10] );
   REQUIRE_EQ( 10, lst.Remove( &nums.front() ) );
   REQUIRE_EQ( 23, lst.GetCount() );
   REQUIRE_EQ( 0, lst.Remove( &nums.front() ) );
   REQUIRE_EQ( 22, lst.GetCount() );
}

TEST_CASE( "Simple use of TXStrings" )
{
   TXStrings lst;
   lst.Add( "First", 5 );
   lst.Add( "Second", 6 );
   lst.Add( "Third", 5 );
   REQUIRE_EQ( 1, lst.IndexOf( "Second" ) );
   REQUIRE_EQ( "First"s, lst[0] );
}

TEST_CASE( "Simple use TXCustomStringList" )
{
   TXCustomStringList<uint8_t> lst;
   lst.Add( "First", 5 );
   lst.Add( "Second", 6 );
   lst.Add( "Third", 5 );
   REQUIRE_EQ( 1, lst.IndexOf( "Second" ) );
   REQUIRE_EQ( "First"s, lst[0] );
}

TEST_CASE( "Extensive use of TXStrPool" )
{
   TXStrPool<uint8_t> lst;
   REQUIRE_FALSE( lst.OneBased );
   REQUIRE_EQ( 0, lst.Count() );
   REQUIRE_EQ( 0, lst.GetCapacity() );
   REQUIRE_EQ( 0, lst.size() );
   REQUIRE_EQ( 0, lst.MemoryUsed() );
   const int n{ 100 };
   lst.SetCapacity( n );
   std::list<int> knownIndices{};
   for( int i{}; i < n; i++ )
   {
      std::string s{ "i" + std::to_string( i + 1 ) };
      int ix{ lst.Add( s.c_str(), s.length() ) };
      REQUIRE_GT( ix, -1 );
      auto it{ std::find( knownIndices.begin(), knownIndices.end(), ix ) };
      REQUIRE_EQ( knownIndices.end(), it );
      knownIndices.push_back( ix );
   }
   REQUIRE_LE( n, lst.GetCapacity() );
   REQUIRE_EQ( n, lst.size() );
   REQUIRE( !std::strcmp( "i1", lst.GetName( 0 ) ) );
   REQUIRE( !lst.GetObject( 0 ) );
   lst.Delete( 0 );
   REQUIRE_EQ( n - 1, lst.size() );
   REQUIRE( !std::strcmp( "i2", lst.GetName( 0 ) ) );
   REQUIRE_GT( lst.MemoryUsed(), 0 );
   REQUIRE_GT( lst.Add( "", 0 ), -1 );
}

template<typename T, typename f>
void runStressTest( const std::string_view caption, f getelem )
{
   auto t{ std::chrono::high_resolution_clock::now() };
   constexpr int card{ 90000 }, ntries{ 40 };
   static_assert( card < std::numeric_limits<int>::max() );
   std::array<int, card> nums;
   std::random_device rd;
   for( int k{}; k < ntries; k++ )
   {
      T shlst;
      std::mt19937 g( rd() );
      std::iota( nums.begin(), nums.end(), 1 );
      std::shuffle( nums.begin(), nums.end(), g );
      int ctr{};
      for( int n: nums )
      {
         std::string s{ "i"s + std::to_string( n ) };
         REQUIRE_EQ( ctr++, shlst.Add( s.c_str(), s.length() ) );
      }
      int sum{};
      for( int i{}; i < card; i++ )
         sum += (int) std::strlen( shlst.GetName( i ) );
   }
   auto delta{ std::chrono::high_resolution_clock::now() - t };
   std::cout << "Time in milliseconds for "s << caption << ": "s << delta / std::chrono::milliseconds( 1 ) << std::endl;
}

TEST_CASE( "TXStrPool vs. TXCSStrHashList" )
{
   runStressTest<TXStrPool<uint8_t>>( "TXStrPool"s, []( uint8_t &n ) { return &n; } );
   runStressTest<collections::strhash::TXCSStrHashList<uint8_t>>( "TXCSStrHashList"s, []( uint8_t &n ) { return n; } );
}

TEST_SUITE_END();

}// namespace gdx::tests::gmsobjtests
