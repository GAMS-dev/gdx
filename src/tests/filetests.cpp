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

#include "doctest.h"// for ResultBuilder, Expressi...
#include "../file.h"

#include <array>
#include <filesystem>
#include <functional>
#include <list>

using namespace gdx::file;
using namespace std::literals::string_literals;

namespace gdx::tests::filetests
{
TEST_SUITE_BEGIN( "File object tests" );

const static std::array<char, 4> exampleChars { 'a', 'b', 'c', '\0' };
using SmallBuf = std::array<char, exampleChars.size()>;

static void bufferHasExampleChars( const SmallBuf &buf )
{
   for( int i {}; i < (int)exampleChars.size(); i++ )
      REQUIRE_EQ( exampleChars[i], buf[i] );
}

static void simpleCharacterReadTest( IFile &fsf, const std::string &fn )
{
   SmallBuf buf {};
   uint32_t numRead {};
   REQUIRE_FALSE( fsf.open( fn, FileOpenAction::fileOpenRead ) );
   REQUIRE_FALSE( fsf.read( buf.data(), buf.size(), numRead ) );
   REQUIRE_FALSE( fsf.close() );
   bufferHasExampleChars( buf );
   REQUIRE_EQ( exampleChars.size(), numRead );
}

static void writeExampleFile( const std::string &fn )
{
   std::ofstream ofs { fn, std::ios::binary | std::ios::out };
   REQUIRE( ofs.good() );
   ofs.write( exampleChars.data(), exampleChars.size() );
}

static void simpleCharacterWriteTest( IFile &fsf, const std::string &fn )
{
   REQUIRE_FALSE( fsf.open( fn, FileOpenAction::fileOpenWrite ) );
   REQUIRE_FALSE( fsf.write( exampleChars.data(), exampleChars.size() ) );
   REQUIRE_FALSE( fsf.close() );

   {
      std::ifstream ifs { fn, std::ios::binary | std::ios::in };
      REQUIRE( ifs.good() );
      SmallBuf buf {};
      ifs.read( buf.data(), buf.size() );
      auto readCount { ifs.gcount() };
      REQUIRE_EQ( exampleChars.size(), readCount );
      bufferHasExampleChars( buf );
   }

   std::filesystem::remove( fn );
}

static void simpleSeekTest( IFile &f, const std::string &fn ) {
   REQUIRE_FALSE(f.open(fn, FileOpenAction::fileOpenRead));
   const auto seekOffset{2};
   REQUIRE_FALSE(f.seek(seekOffset));
   SmallBuf buf {};
   uint32_t numRead {};
   REQUIRE_FALSE(f.read(buf.data(), buf.size(), numRead));
   REQUIRE_EQ(exampleChars.size()-seekOffset, numRead);
   for(int i{seekOffset}; i<(int)exampleChars.size(); i++) {
      REQUIRE_EQ(buf[i-seekOffset], exampleChars[i]);
   }
   REQUIRE_FALSE(f.seek(0));
   REQUIRE_FALSE(f.read(buf.data(), buf.size(), numRead));
   REQUIRE_EQ(exampleChars.size(), numRead);
   bufferHasExampleChars(buf);
   REQUIRE_FALSE(f.close());
}

#if defined(_WIN32)
using LowLevelOSFile = WinAPIFile;
#else
using LowLevelOSFile = POSIXFile;
#endif

static const std::array<std::function<IFile*()>, 2> fileImplGenerators {
        []() { return new FStreamFile(); },
        []() { return new LowLevelOSFile(); }
};

static void runForAllFileImpls(const std::function<void(IFile&)> &func) {
   for(const auto &fig : fileImplGenerators) {
      IFile *f {fig()};
      func(*f);
      delete f;
   }
}

TEST_CASE( "Simple character reading" )
{
   const auto fn { "test_read"s };
   writeExampleFile( fn );
   runForAllFileImpls([&](IFile &f) {
      simpleCharacterReadTest(f, fn);
   });
   std::filesystem::remove( fn );
}

TEST_CASE( "Simple character writing" )
{
   runForAllFileImpls([&](IFile &f) {
      simpleCharacterWriteTest(f, "test_write"s);
   });
}

TEST_CASE( "Move reading cursor" )
{
   const auto fn { "test_seek"s };
   writeExampleFile( fn );
   runForAllFileImpls([&](IFile &f) { simpleSeekTest(f, fn); });
   std::filesystem::remove(fn);
}

TEST_CASE("Test opening non-existant file") {
   runForAllFileImpls([](IFile &f) {
      REQUIRE(f.open("this_for_sure_does_not_exist"s, FileOpenAction::fileOpenRead));
   });
}

}// namespace gdx::tests::filetests