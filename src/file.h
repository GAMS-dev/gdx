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

#pragma once

#include <string>
#include <fstream>
#include <cstdint>

namespace gdx::file
{

enum FileOpenAction
{
   fileOpenRead,
   fileOpenWrite,
   fileOpenReadWrite
};

class IFile {
public:
   virtual ~IFile() = default;
   virtual int open(const std::string &fileName, FileOpenAction mode) = 0;
   virtual int read(char *buffer, uint32_t buflen, uint32_t &numRead) = 0;
   virtual int write(const char *buffer, uint32_t count) = 0;
   virtual int close() = 0;
   virtual int seek(int64_t p) = 0;
};

class FStreamFile : public IFile
{
   std::fstream fs {};
public:
   ~FStreamFile() override;
   int open(const std::string &fileName, FileOpenAction mode) override;
   int close() override;
   int read( char *buffer, uint32_t buflen, uint32_t &numRead ) override;
   int seek( int64_t p ) override;
   int write( const char *buffer, uint32_t count ) override;
};

#if defined(_WIN32)
class WinAPIFile : public IFile {
   void *handle{};
public:
   ~WinAPIFile() override;
   int open( const std::string &fileName, FileOpenAction mode ) override;
   int read( char *buffer, uint32_t buflen, uint32_t &numRead ) override;
   int write( const char *buffer, uint32_t count ) override;
   int close() override;
   int seek( int64_t p ) override;
};
#else
class POSIXFile : public IFile {
   int h{}; // file descriptor
public:
   ~POSIXFile() override;
   int open( const std::string &fileName, FileOpenAction mode ) override;
   int read( char *buffer, uint32_t buflen, uint32_t &numRead ) override;
   int write( const char *buffer, uint32_t count ) override;
   int close() override;
   int seek( int64_t p ) override;
};
#endif

}
