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

#include "gdlib/xcompress.h"
#include "../doctest.h"

#include <vector>
#include <array>
#include <cstring>
#include <iostream>
#include <cstdint>
#include <cassert>

using namespace std::literals::string_literals;
using namespace gdlib::xcompress;

namespace tests::gdlibtests::xcompresstests
{

static auto compressStringToBlob( const std::string &s )
{
   std::vector<char> outBuf( 4096, 0 );
   ulong outBufLen { static_cast<uint32_t>( outBuf.size() ) };
   std::array<char, 256> srcBuf {};
   std::memcpy(srcBuf.data(), s.c_str(), s.length()+1);
   const int rc = compress( outBuf.data(), outBufLen, srcBuf.data(), static_cast<unsigned long>( s.length() + 1 ) );
   REQUIRE( !rc );
   outBuf.resize( outBufLen );
   return outBuf;
}

static std::string uncompressBlobToString( const std::vector<char> &blob )
{
   std::array<char, 4096> outBuf {};

   // AS: Why is this needed
   // Why does uncompress write 4 times \0 at the beginning of the out buffer if it was
   // initially filled just with zeroes?
   outBuf.fill( 23 );

   ulong outBufLen { 4096 };
   const int rc = uncompress( outBuf.data(), outBufLen, blob.data(), static_cast<unsigned long>(blob.size()) );
   REQUIRE( !rc );
   REQUIRE( outBufLen > 0 );
   for( int i {}; !outBuf[i]; i++ )
      outBuf[i] = '0';
   return outBuf.data();
}

TEST_SUITE_BEGIN( "gdlib::xcompress" );

TEST_CASE( "Fundamental test" )
{
   if( !ZLibDllLoaded() )
   {
      std::string loadMsg;
      LoadZLibLibrary( "gmszlib1", loadMsg );
      assert(loadMsg.empty());
   }
   constexpr int countUpTo = 10, bigEnough = 50;
   char outBuf[50], srcBuf[countUpTo];
   for( int i = 0; i < countUpTo; i++ )
      srcBuf[i] = static_cast<char>(i);
   unsigned long outBufLen = bigEnough;
   int rc = compress( outBuf, outBufLen, &srcBuf, countUpTo );
   REQUIRE( !rc );
   char outBuf2[bigEnough];
   unsigned long outBuf2Len = bigEnough;
   rc = uncompress( outBuf2, outBuf2Len, outBuf, outBufLen );
   REQUIRE( !rc );
   for( int i = 0; i < countUpTo; i++ )
   {
      REQUIRE_EQ( srcBuf[i], outBuf2[i] );
   }
}

TEST_CASE( "Test compression and decompression" )
{
   if( !ZLibDllLoaded() )
   {
      std::string loadMsg;
      LoadZLibLibrary( "gmszlib1", loadMsg );
      REQUIRE(loadMsg.empty());
   }
   const std::string testStr { "Das ist ein Test!" };
   const auto blob = compressStringToBlob( testStr );
   REQUIRE_EQ( uncompressBlobToString( blob ), testStr );
}

TEST_SUITE_END();

}
