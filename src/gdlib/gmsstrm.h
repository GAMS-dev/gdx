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

#pragma once

#include <array>                // for array
#include <cassert>              // for assert
#include <cstdint>              // for uint32_t, uint8_t, int64_t, uint16_t
#include <cstring>              // for strlen
#include <memory>               // for unique_ptr
#include <string>               // for string, basic_string
#include <string_view>          // for string_view
#include <vector>               // for vector
#include "../rtl/p3utils.h"     // for Tp3FileHandle

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

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace gdlib::gmsstrm
{
constexpr bool Paranoid = false;

std::string SysErrorMessage( int errorCore );

constexpr int
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

constexpr uint16_t PAT_WORD = 0x1234u;
constexpr /*uint32_t*/ int PAT_INTEGER = 0x12345678 /*u*/;
constexpr double PAT_DOUBLE = 3.1415926535897932385;
constexpr int PAT_BAD_ORDER = 254;
constexpr int PAT_BAD_SIZE = 255;

// File Mode creation constants
enum FileAccessMode
{
   fmCreate = 0xFFFF,
   fmOpenRead = 0x0000,
   fmOpenWrite = 0x001,
   fmOpenReadWrite = 0x002
};

enum class RWType : uint8_t
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
const std::array<std::string, 10> RWTypeText { "Byte", "Bool", "Char", "Word", "Integer", "Int64", "Double", "String", "PChar", "PString" };

/**
 * Defines the base class for a stream. Only to be used for defining derived objects.
 */
class TXStream
{
protected:
#ifdef WF_TEXT
   std::unique_ptr<std::ofstream> fstext {};
#endif

   void ParWrite( RWType T );
   void ParCheck( RWType T );

   template<typename T>
   void WriteValue( RWType rwt, T &v )
   {
      if( Paranoid ) ParWrite( rwt );
      Write( &v, sizeof( T ) );
   }

   template<typename T>
   T ReadValue( RWType rwt )
   {
      if( Paranoid ) ParCheck( rwt );
      T res;
#if !defined( NDEBUG ) && defined(__IN_CPPMEX__)
      auto numBytesRead { Read( &res, sizeof( T ) ) };
      // is not required to generally hold and code automatically detects EOF otherwise
      //assert( numBytesRead == sizeof( T ) );
#else
      Read( &res, sizeof( T ) );
#endif
      return res;
   }

   virtual int64_t GetPosition() = 0;
   virtual void SetPosition( int64_t P ) = 0;
   virtual int64_t GetSize() = 0;

public:
   virtual ~TXStream() = default;

   virtual uint32_t Read( void *Buffer, uint32_t Count ) = 0;
   virtual uint32_t Write( const void *Buffer, uint32_t Count ) = 0;

   void WriteString( std::string_view s );

   void WriteString( const char *s )
   {
      WriteString( std::string_view { s, std::strlen( s ) } );
   }

   template<int count>
   void WriteString( const std::array<char, count> &s )
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
   [[nodiscard]] std::string ReadPChar( int &L );
   [[nodiscard]] std::string ReadPChar();

   void ActiveWriteOpTextDumping( const std::string &dumpFilename );
};

class TXFileStream : public TXStream
{
   friend class TBinaryTextFileIO;

   rtl::p3utils::Tp3FileHandle FS {};
   bool FileIsOpen {};
   std::string FFileName {}, FPassWord {};

   static std::string RandString( int L );

protected:
   int FLastIOResult {};
   int64_t PhysPosition {};

   int64_t GetSize() override;
   int64_t GetPosition() override;
   void SetPosition( int64_t P ) override;

public:
   TXFileStream( std::string AFileName, FileAccessMode AMode );
   ~TXFileStream() override;

   void ApplyPassWord( const char *PR, char *PW, int Len, int64_t Offs ) const;
   uint32_t Read( void *Buffer, uint32_t Count ) override;
   uint32_t Write( const void *Buffer, uint32_t Count ) override;

   void SetLastIOResult( int V );
   int GetLastIOResult();

   void SetPassWord( const std::string &s );
   [[nodiscard]] bool GetUsesPassWord() const;
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

class TBufferedFileStream : public TXFileStream
{
   friend class TBinaryTextFileIO;

   uint32_t NrLoaded, NrRead, NrWritten, BufSize, CBufSize;

   std::vector<uint8_t> BufPtr;
   PCompressBuffer CBufPtr;

   bool FCompress, FCanCompress;

   bool FillBuffer();

protected:
   int64_t GetSize() override;

public:
   TBufferedFileStream( const std::string &FileName, uint16_t Mode );
   ~TBufferedFileStream() override;
   bool FlushBuffer();
   uint32_t Read( void *Buffer, uint32_t Count ) override;
   char ReadCharacter();
   uint32_t Write( const void *Buffer, uint32_t Count ) override;
   bool IsEof();
   [[nodiscard]] bool GetCompression() const;
   void SetCompression( bool V );
   [[nodiscard]] bool GetCanCompress() const;

   int64_t GetPosition() override;

   void SetPosition( int64_t p ) override;
};

void reverseBytesMax8( const void *psrc, void *pdest, int sz );

class TMiBufferedStream : public TBufferedFileStream
{
   uint8_t order_word {}, order_integer {}, order_double {}, size_word {}, size_integer {}, size_double {};
   bool NormalOrder {};

   template<typename T>
   T ReadValueOrdered( RWType rwt, bool order_type )
   {
      if( Paranoid ) ParCheck( rwt );
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
   }

   void DetermineByteOrder();

public:
   TMiBufferedStream( const std::string &FileName, uint16_t Mode );
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

enum TFileSignature : uint8_t
{
   fsign_text,
   fsign_blocktext,
   fsign_gzip
};

class TGZipInputStream final
{
   gzFile pgz;
   std::vector<uint8_t> Buf;
   uint32_t NrLoaded {}, NrRead {};

public:
   TGZipInputStream( const std::string &fn, std::string &ErrMsg );
   ~TGZipInputStream();

   uint32_t Read( void *buffer, uint32_t Count );

   void ReadLine( std::string &buffer, int MaxInp, char &LastChar );
   void ReadLine( std::vector<uint8_t> &buffer, int MaxInp, char &LastChar );
   void ReadLine( char *buffer, int MaxInp, char &LastChar, int &Len );
};

class TBinaryTextFileIO
{
   std::unique_ptr<TBufferedFileStream> FS{};
   std::unique_ptr<TGZipInputStream> gzFS{};

   // GCC <= 11 doesn't like the maybe_unused here
#if defined(_MSC_VER) || defined(__clang__) || !defined(__GNUC__) || ((__GNUC__ > 12) || (__GNUC__ == 12 && __GNUC_MINOR__ >= 0))
   [[maybe_unused]]
#endif
   enum : uint8_t
   {
      fm_read,
      fm_write
   } frw{fm_read};
   TFileSignature FFileSignature{fsign_text};
   uint8_t FMajorVersionRead{}, FMinorVersionRead{};
   int64_t FRewindPoint{};

public:
   // OpenForRead
   TBinaryTextFileIO( const std::string &fn, const std::string &PassWord, int &ErrNr, std::string &errMsg );
   // OpenForWrite
   TBinaryTextFileIO( const std::string &fn, const std::string &Producer, const std::string &PassWord, TFileSignature signature, bool comp, int &ErrNr, std::string &errMsg );

   uint32_t Read( char *Buffer, uint32_t Count );
   char ReadCharacter();
   void ReadLine( std::vector<uint8_t> &Buffer, int &Len, int MaxInp, char &LastChar );
   void ReadLine( char *Buffer, int &Len, int MaxInp, char &LastChar );
   void ReadLine( std::string &StrBuffer, int &Len, int MaxInp, char &LastChar ) const;
   uint32_t Write( const char *Buffer, uint32_t Count ) const;
   bool UsesPassWord();
   void ReWind();
   int GetLastIOResult();
};

void CompressTextFile( const std::string &fn, const std::string &fo, const std::string &PassWord, bool Comp, int &ErrNr, std::string &ErrMsg );
void UnCompressTextFile( const std::string &fn, const std::string &fo, const std::string &PassWord, int &ErrNr, std::string &ErrMsg );
}// namespace gdlib::gmsstrm
