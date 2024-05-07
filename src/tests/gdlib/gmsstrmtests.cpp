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

#include "gdlib/gmsstrm.h"
#include "../doctest.h"
#include <fstream>
#include <filesystem>

using namespace std::literals::string_literals;
using namespace gdlib::gmsstrm;

namespace tests::gdlibtests::gmsstrmtests
{

TEST_SUITE_BEGIN( "gdlib::gmsstrm" );

template<typename StrmClazz>
void testWritingOneByteIntoTextFile()
{
   const uint8_t examplePayload { 23 };
   const std::string exampleFn { "test.txt"s };

   if( std::filesystem::exists( exampleFn ) )
      std::filesystem::remove( exampleFn );

   // Write example payload to disk with unbuffered GAMS stream
   {
      StrmClazz fs { exampleFn, fmOpenWrite };
      fs.WriteByte( examplePayload );
   }

   REQUIRE( std::filesystem::exists( exampleFn ) );

   // Use C++ standard library file stream to slurp file and compare payload contents
   {
      std::ifstream ifs { exampleFn };
      char buf;
      ifs.read( &buf, 1 );
      REQUIRE_EQ( examplePayload, buf );
      REQUIRE_EQ( EOF, ifs.peek() );
   }

   std::filesystem::remove( exampleFn );
}

TEST_CASE( "Use unbuffered file stream to write a file" )
{
   testWritingOneByteIntoTextFile<TXFileStream>();
}

TEST_CASE( "Use buffered file stream to write a file" )
{
   testWritingOneByteIntoTextFile<TBufferedFileStream>();
}

TEST_CASE( "Use binary text file IO to write an uncompressed file" )
{
   const bool compression {}, password {};
   const auto examplePayload = 23;
   const std::string exampleFn { "testfile"s + ( compression ? ".gz"s : ""s ) };
   const std::string pw = password ? "mypass"s : ""s;

   if( std::filesystem::exists( exampleFn ) )
      std::filesystem::remove( exampleFn );

   {
      int errNr;
      std::string errMsg;
      TBinaryTextFileIO fs { exampleFn, "testCppMEX"s, pw, compression ? TFileSignature::fsign_gzip : TFileSignature::fsign_text, compression, errNr, errMsg };
      REQUIRE( errMsg.empty() );
      char buf { 23 };
      REQUIRE_EQ( 1, fs.Write( &buf, 1 ) );
   };

   REQUIRE( std::filesystem::exists( exampleFn ) );

   {
      std::ifstream ifs { exampleFn, std::ios::binary };
      char buf {};
      ifs.read( &buf, 1 );
      REQUIRE_EQ( examplePayload, buf );
      REQUIRE_EQ( EOF, ifs.peek() );
   }

   std::filesystem::remove( exampleFn );
}

static void testWritingSingleLineModel( const bool useCompression, const bool usePassword )
{
   std::string miniModelText = "scalar x / 23 /;"s;

   int ErrNr;
   std::string ErrMsg;
   const std::string mypass = usePassword ? "strenggeheim"s : ""s, model_fn = "test.gms"s;

   if( std::filesystem::exists( model_fn ) )
      std::filesystem::remove( model_fn );

   {
      TBinaryTextFileIO Fout { model_fn, "SingleLineCompressDecompress"s, mypass, fsign_text, useCompression, ErrNr, ErrMsg };
      Fout.Write( miniModelText.data(), static_cast<int>(miniModelText.size()) );
   }

   ErrMsg.clear();

   {
      TBinaryTextFileIO Fin { model_fn, mypass, ErrNr, ErrMsg };
      std::array<char, 4096> readBuf {};
      auto len = Fin.Read( readBuf.data(), static_cast<uint32_t>(readBuf.size()) );
      readBuf[len] = '\0';
      std::string decompressedText( readBuf.data() );
      REQUIRE_EQ( miniModelText, decompressedText );
   }

   std::filesystem::remove( model_fn );
}

TEST_CASE( "Writing a single line model without compression" )
{
   testWritingSingleLineModel( false, false );
}

TEST_CASE( "Writing a single line model with compression" )
{
   testWritingSingleLineModel( true, false );
}

TEST_SUITE_END();

}
