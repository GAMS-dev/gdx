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

#include <array>      // for array
#include <cstdint>    // for uint32_t, uint8_t, int64_t, uint16_t
#include <cstring>    // for strlen
#include <fstream>    // for fstream, basic_fstream
#include <map>        // for map
#include <memory>     // for allocator, unique_ptr
#include <string>     // for string, basic_string
#include <string_view>// for string_view
#include <vector>     // for vector

#if defined( NO_ZLIB )
inline int uncompress( void *dest, unsigned long *destLen, const void *source, unsigned long sourceLen )
{
   *destLen = sourceLen;
   std::memcpy( dest, source, sourceLen );
   return 0;
}

inline int compress( void *dest, unsigned long *destLen, const void *source, unsigned long sourceLen )
{
   *destLen = sourceLen;
   std::memcpy( dest, source, sourceLen );
   return 0;
}
#else
#include <zlib.h>
#endif

#include "file.h"

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace gdx::gmsstrm
{
std::string SysErrorMessage( int errorCore );

const int
        // TXStream seek origins
        soFromBeginning = 0,
        soFromCurrent = 1,
        soFromEnd = 2,

        BufferSize = 32 * 1024,//internal maximum for compression

        strmErrorNoError = 0,
        strmErrorIOResult = 1,
        strmErrorGAMSHeader = 2,
        strmErrorNoPassWord = 3,
        strmErrorIntegrity = 4,
        strmErrorZLib = 5,
        strmErrorEncoding = 6;

const uint16_t PAT_WORD = 0x1234u;
const /*uint32_t*/ int PAT_INTEGER = 0x12345678 /*u*/;
const double PAT_DOUBLE = 3.1415926535897932385;
const int PAT_BAD_ORDER = 254;
const int PAT_BAD_SIZE = 255;

// File Mode creation constants
enum FileAccessMode
{
   fmCreate = 0xFFFF,
   fmOpenRead = 0x0000,
   fmOpenWrite = 0x001,
   fmOpenReadWrite = 0x002
};

// File Mode creation constants
const std::map<FileAccessMode, std::string> modeStrs = {
        { FileAccessMode::fmCreate, "w" },
        { FileAccessMode::fmOpenRead, "r" },
        { FileAccessMode::fmOpenWrite, "w" },
        { FileAccessMode::fmOpenReadWrite, "w+" } };

enum class RWType
{
   rw_byte,
   rw_bool,
   rw_char,
   rw_word,
   rw_integer,
   rw_int64,
   rw_double,
   rw_string,
   rw_pchar,
   rw_pstring,
   rw_count
};
const std::array<std::string, 10> RWTypeText = { "Byte", "Bool", "Char", "Word", "Integer", "Int64", "Double", "String", "PChar", "PString" };

/**
     * Defines the base class for a stream. Only to be used for defining derived objects.
     */
class TXStreamDelphi
{
protected:
   template<typename T>
   void WriteValue( RWType rwt, T &v )
   {
      Write( &v, sizeof( T ) );
   }

   template<typename T>
   T ReadValue( RWType rwt )
   {
      T res;
      Read( &res, sizeof( T ) );
      return res;
   }

   virtual int64_t GetPosition() = 0;
   virtual void SetPosition( int64_t P ) = 0;

public:
   virtual ~TXStreamDelphi() = default;

   virtual uint32_t Read( void *Buffer, uint32_t Count ) = 0;
   virtual uint32_t Write( const void *Buffer, uint32_t Count ) = 0;

   void WriteString( std::string_view s );

   inline void WriteString( const char *s )
   {
      WriteString( std::string_view { s, std::strlen( s ) } );
   }

   template<int count>
   inline void WriteString( const std::array<char, count> &s )
   {
      WriteString( s.data() );
   }

   void WriteDouble( double x );
   void WriteInteger( int n );
   void WriteInt64( int64_t N );
   void WriteByte( uint8_t b );
   void WriteWord( uint16_t W );
   void WriteBool( bool B );
   void WriteChar( char C );
   void WritePChar( const char *s, int L );

   std::string ReadString();
   virtual double ReadDouble();
   virtual int ReadInteger();
   uint8_t ReadByte();
   virtual uint16_t ReadWord();
   virtual int64_t ReadInt64();
   bool ReadBool();
   char ReadChar();
   void ReadPChar( char *P, int &L );

   void ActiveWriteOpTextDumping( const std::string &dumpFilename );
};

#if defined(USE_FSTREAM)
using FileImpl = gdx::file::FStreamFile;
#elif defined(_WIN32)
using FileImpl = gdx::file::WinAPIFile;
#else
using FileImpl = gdx::file::POSIXFile;
#endif

class TXFileStreamDelphi : public TXStreamDelphi
{
   FileImpl FS {};
   bool FileIsOpen {};
   std::string FFileName {}, FPassWord {};

protected:
   int FLastIOResult {};
   int64_t PhysPosition {};

   int64_t GetPosition() override;
   void SetPosition( int64_t P ) override;

public:
   TXFileStreamDelphi( std::string AFileName, FileAccessMode AMode );
   ~TXFileStreamDelphi() override;
   void ApplyPassWord( const char *PR, char *PW, int Len, int64_t Offs );
   uint32_t Read( void *Buffer, uint32_t Count ) override;
   uint32_t Write( const void *Buffer, uint32_t Count ) override;
   void SetLastIOResult( int V );
   int GetLastIOResult();
   void SetPassWord( const std::string &s );
   bool GetUsesPassWord();
   [[nodiscard]] std::string GetFileName() const;
};

struct TCompressHeader {
   // 0=no compression, 1=zlib
   uint8_t cxTyp;
   uint8_t cxB1, cxB2;
};

struct TCompressBuffer {
   TCompressHeader cxHeader;
   uint8_t cxData;
};
using PCompressBuffer = TCompressBuffer *;

class TBufferedFileStreamDelphi : public TXFileStreamDelphi
{
   uint32_t NrLoaded, NrRead, NrWritten, BufSize, CBufSize;
   std::vector<uint8_t> BufPtr;
   PCompressBuffer CBufPtr;
   bool FCompress, FCanCompress;

   bool FillBuffer();

public:
   TBufferedFileStreamDelphi( const std::string &FileName, uint16_t Mode );
   ~TBufferedFileStreamDelphi() override;
   bool FlushBuffer();
   uint32_t Read( void *Buffer, uint32_t Count ) override;
   char ReadCharacter();
   uint32_t Write( const void *Buffer, uint32_t Count ) override;
   [[nodiscard]] bool GetCompression() const;
   void SetCompression( bool V );
   [[nodiscard]] bool GetCanCompress() const;
   int64_t GetPosition() override;
   void SetPosition( int64_t p ) override;
};

void reverseBytesMax8( void *psrc, void *pdest, int sz );

class TMiBufferedStreamDelphi : public TBufferedFileStreamDelphi
{
   uint8_t order_word {}, order_integer {}, order_double {}, size_word {}, size_integer {}, size_double {};
   bool NormalOrder {};

   template<typename T>
   T ReadValueOrdered( RWType rwt, bool order_type )
   {
      T res;
      if( !order_type ) Read( &res, sizeof( T ) );
      else
      {
         T tmp;
         Read( &tmp, sizeof( T ) );
         reverseBytesMax8( &tmp, &res, sizeof( T ) );
      }
      return res;
   }

   template<typename T>
   void initOrderCommon( uint8_t &order_type, uint8_t &size_type, T patConstant )
   {
      T v1, v2;
      Read( &size_type, sizeof( uint8_t ) );
      if( size_type != sizeof( T ) )
      {
         order_type = PAT_BAD_SIZE;
         SetPosition( GetPosition() + size_type );
      }
      else
      {
         Read( &v1, sizeof( T ) );
         order_type = 0;
         if( v1 != patConstant )
         {
            order_type = 1;
            reverseBytesMax8( &v1, &v2, sizeof( T ) );
            if( v2 != patConstant ) order_type = PAT_BAD_ORDER;
         }
      }
   };

   void DetermineByteOrder();

public:
   TMiBufferedStreamDelphi( const std::string &FileName, uint16_t Mode );
   static void ReverseBytes( void *psrc, void *pdest, int sz );
   [[nodiscard]] int GoodByteOrder() const;
   double ReadDouble() override;
   int ReadInteger() override;
   uint16_t ReadWord() override;
   int64_t ReadInt64() override;
   [[nodiscard]] bool WordsNeedFlip() const;
   [[nodiscard]] bool IntsNeedFlip() const;
   void WriteGmsInteger( int N );
   void WriteGmsDouble( double D );
   int ReadGmsInteger();
   double ReadGmsDouble();
};
}// namespace gdx::gmsstrm
