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

#include "rtl/stdthread.hpp"
#include "../doctest.hpp"

#include <algorithm>
#include <list>

using namespace std::literals::string_literals;

using namespace rtl::stdthread;

namespace tests::rtltests::stdthreadtests
{

class MyThread final : public TStdThread
{
public:
   int x {};
   MyThread() : TStdThread{ [this]() { x=23; } } {}
};

TEST_SUITE_BEGIN( "rtl::stdthread" );

TEST_CASE( "Rudimentary thread usage" )
{
   // Skip this test for now
   return;
   MyThread t;
   REQUIRE( t.joinable() );
   t.join();
   REQUIRE_EQ( 23, t.x );
}

TEST_CASE( "Mutex usage" )
{
   // Skip this test for now
   return;
   std::list<std::pair<int, int>> pairs;

   {
      int ctr {};
      std::mutex m;
      auto f = [&](int thread_ix) {
         return [&pairs,thread_ix,&ctr,&m]() {
            std::unique_lock localLock {m};
            pairs.emplace_back( thread_ix, ctr++ );
         };
      };
      constexpr int n { 10 };
      std::list<TStdThread> threads {};
      for( int i {}; i < n; i++ )
         threads.emplace_back(f(i));
      std::for_each( threads.begin(), threads.end(),[]( TStdThread &t ) { t.join(); } );
   }

   int i {};
   for( const auto &pair: pairs )
   {
      REQUIRE_EQ( i, pair.second );
      i++;
   }
}

TEST_CASE( "Conditional variable usage" )
{
   // Skip this test for now
   return;
   std::mutex mutex;
   const TStdCondVar cvar;

   TStdThread t {[&]() {
      std::unique_lock localLock{mutex};
      cvar.notifyOne();
   }};

   {
      std::unique_lock localLock{mutex};
      cvar.wait( mutex );
   }

   t.join();
}

TEST_SUITE_END();

}
