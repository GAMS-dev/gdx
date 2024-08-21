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

#include "gmsstrm.h"

#include <algorithm>             // for min, max
#include <cassert>               // for assert
#include <cmath>                 // for round, abs
#include <cstdint>               // for uint32_t, uint8_t, int64_t, uint16_t
#include <cstdlib>               // for malloc, abs, free
#include <cstring>               // for memcpy, strerror
#include <functional>            // for function
#include <stdexcept>             // for runtime_error
#include <string>                // for basic_string, string, operator+, ope...
#include <utility>               // for move

#include "../rtl/sysutils_p3.h"  // for IntToStr
#include "gclgms.h"              // for GMS_SV_ACR, GMS_SV_UNDEF, GMS_SV_EPS
#include "global/delphitypes.h"  // for Bounded
#include "rtl/p3utils.h"         // for Tp3FileOpenAction, p3FileRead, p3Fil...
#include "utils.h"               // for in, ord, checkBOMOffset, constructStr

// only supported by MSVC so far :(
//#include <format>

using namespace std::literals::string_literals;
using namespace rtl::p3utils;

#if defined(__IN_CPPMEX__)
#include "../gdlib/statlib.h"
#endif

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace gdlib::gmsstrm
{

std::string SysErrorMessage( int errorCode )
{
#if defined( _WIN32 )
   static std::array<char, 256> errMsgBuf;
   strerror_s( errMsgBuf.data(), (int) errMsgBuf.size(), errorCode );
   char *errMsg = errMsgBuf.data();
#else
   char *errMsg = strerror( errorCode );
   if( !errMsg ) return "Unknown error " + rtl::sysutils_p3::IntToStr( errorCode );
#endif
   return errMsg;
}

enum CustomOpenAction : uint8_t
{
   custOpenRead,
   custOpenWrite,
   custOpenReadWrite
};

constexpr uint8_t signature_header = 0xFF;
const std::string signature_gams = "*GAMS*"s;
constexpr int verify_offset = 100;

constexpr static char substChar {/*0x1A*/ std::char_traits<char>::eof()};

union TDoubleVar
{
   //bool realNumOrBytes;
   double V;
   std::array<uint8_t, 8> VA;
};

// take the bytes of psrc and return them - in reverse order - in pdest
// sz should be at most 8, but we do not promise to check this!
void reverseBytesMax8( const void *psrc, void *pdest, int sz )
{
   std::array<uint8_t, 8> orig {}, flip {};
   const int n { std::min( sz - 1, 7 ) };
   std::memcpy( orig.data(), psrc, n + 1 );
   for( int k {}; k <= n; k++ )
      flip[k] = orig[n - k];
   std::memcpy( pdest, flip.data(), n + 1 );
}

void CompressTextFile( const std::string &fn, const std::string &fo, const std::string &PassWord, bool Comp, int &ErrNr, std::string &ErrMsg )
{
   TBinaryTextFileIO Fin { fn, "", ErrNr, ErrMsg };
   if( !ErrMsg.empty() ) return;

   TBinaryTextFileIO Fout { fo, "CompressTextFile", PassWord, fsign_text, Comp, ErrNr, ErrMsg };
   if( !ErrMsg.empty() ) return;

   std::array<char, 4096> Buffer {};
   uint32_t NrRead;
   do {
      NrRead = Fin.Read( Buffer.data(), (int) Buffer.size() );
      if( !NrRead ) break;
      Fout.Write( Buffer.data(), NrRead );
   } while( NrRead >= static_cast<decltype(NrRead)>( Buffer.size() ) );
}

void UnCompressTextFile( const std::string &fn, const std::string &fo, const std::string &PassWord, int &ErrNr, std::string &ErrMsg )
{
   TBinaryTextFileIO Fin { fn, PassWord, ErrNr, ErrMsg };
   if( !ErrMsg.empty() ) return;

   TBinaryTextFileIO Fout { fo, "", "", fsign_text, false, ErrNr, ErrMsg };
   if( !ErrMsg.empty() ) return;

   constexpr int BufSize = 4096;

   uint32_t NrRead;
   std::array<char, BufSize> Buffer {};
   do {
      NrRead = Fin.Read( Buffer.data(), BufSize );
      if( !NrRead ) break;
      Fout.Write( Buffer.data(), NrRead );
   } while( NrRead >= BufSize );
}

TGZipInputStream::TGZipInputStream( const std::string &fn, std::string &ErrMsg )
: pgz{gzopen( fn.c_str(), "r" )}
{
   if( !pgz )
      ErrMsg = "Cannot open file";
   else
   {
      ErrMsg.clear();
      Buf.resize( BufferSize );
      NrRead = NrLoaded = 0;
   }
}

TGZipInputStream::~TGZipInputStream()
{
   gzclose( pgz );
}

uint32_t TGZipInputStream::Read( void *buffer, uint32_t Count )
{
   const auto FillBuffer = [&]() -> bool {
      NrLoaded = gzread( pgz, Buf.data(), static_cast<int>( this->Buf.size() ) );
      NrRead = 0;
      return NrLoaded > 0;
   };

   if( Count <= NrLoaded - NrRead )
   {
      std::memcpy( buffer, &Buf[NrRead], Count );
      NrRead += Count;
      return Count;
   }
   int UsrReadCnt {};
   for( uint32_t NrBytes {}; Count > 0; NrRead += NrBytes, UsrReadCnt += static_cast<int>(NrBytes), Count -= NrBytes )
   {
      if( NrRead >= NrLoaded && !FillBuffer() ) break;
      NrBytes = NrLoaded - NrRead;
      if( NrBytes > Count ) NrBytes = Count;
      std::memcpy( &static_cast<uint8_t *>( buffer )[UsrReadCnt], &Buf[NrRead], NrBytes );
   }
   return UsrReadCnt;
}

void TGZipInputStream::ReadLine( std::string &buffer, int MaxInp, char &LastChar )
{
   buffer.clear();
   while( !utils::in<char>( LastChar, '\r', '\n', substChar ) || static_cast<int>( buffer.size() ) == MaxInp )
   {
      buffer.push_back( LastChar );
      if( NrLoaded - NrRead >= 1 )
         LastChar = static_cast<char>( Buf[NrRead++] );
      else if( !Read( &LastChar, 1 ) )
         LastChar = substChar;
   }
}

void TGZipInputStream::ReadLine( std::vector<uint8_t> &buffer, int MaxInp, char &LastChar )
{
   buffer.clear();
   while( !utils::in<char>( LastChar, '\r', '\n', substChar ) || static_cast<int>( buffer.size() ) == MaxInp )
   {
      buffer.push_back( LastChar );
      if( NrLoaded - NrRead >= 1 )
         LastChar = static_cast<char>( Buf[NrRead++] );
      else if( !Read( &LastChar, 1 ) )
         LastChar = substChar;
   }
}

void TGZipInputStream::ReadLine( char *buffer, int MaxInp, char &LastChar, int &Len )
{
   Len = 0;
   while( !utils::in<char>( LastChar, '\r', '\n', substChar ) || static_cast<int>( Len ) == MaxInp )
   {
      buffer[Len++] = LastChar;
      if( NrLoaded - NrRead >= 1 )
         LastChar = static_cast<char>( Buf[NrRead++] );
      else if( !Read( &LastChar, 1 ) )
         LastChar = substChar;
   }
}

void TXStream::ParWrite( RWType T )
{
   Write( &T, 1 );
}

void TXStream::ParCheck( RWType T )
{
   uint8_t B;
   Read( &B, 1 );
   if( B != static_cast<int>(T) )
   {
      const std::string msg {( B >= static_cast<int>(RWType::rw_count) ? "???"s + rtl::sysutils_p3::IntToStr( B ) : RWTypeText[B] )};
      throw std::runtime_error( "Stream check failed: Expected = "s + RWTypeText[static_cast<int>(T)] + " Read = "s + msg );
   }
}

void TXStream::WriteString( const std::string_view s )
{
#ifdef WF_TEXT
   if( fstext )
   {
      static int cnt {};
      *fstext << "WriteString@" << GetPosition() << "#" << ++cnt << ": " << s << "\n";
   }
#endif
   static std::array<char, 256> buf {};
   if( Paranoid ) ParWrite( RWType::rw_string );
   utils::strConvCppToDelphi( s, buf.data() );
   Write( buf.data(), static_cast<uint32_t>( s.length() ) + 1 );
}

void TXStream::WriteDouble( double x )
{
#ifdef WF_TEXT
   if( fstext )
   {
      static int cnt {};
      *fstext << "WriteDouble@" << GetPosition() << "#" << ++cnt << ": " << utils::asdelphifmt( x, 12 ) << '\n';
   }
#endif
   WriteValue( RWType::rw_double, x );
}

void TXStream::WriteInteger( int n )
{
#ifdef WF_TEXT
   if( fstext )
   {
      static int cnt {};
      *fstext << "WriteInteger@" << GetPosition() << "#" << ++cnt << ": " << n << '\n';
   }
#endif
   WriteValue( RWType::rw_integer, n );
}

void TXStream::WriteInt64( int64_t N )
{
#ifdef WF_TEXT
   if( fstext )
   {
      static int cnt {};
      *fstext << "WriteInt64@" << GetPosition() << "#" << ++cnt << ": " << N << '\n';
   }
#endif
   WriteValue( RWType::rw_int64, N );
}

void TXStream::WriteByte( uint8_t b )
{
#ifdef WF_TEXT
   if( fstext )
   {
      static int cnt {};
      *fstext << "WriteByte@" << GetPosition() << "#" << ++cnt << ": " << rtl::sysutils_p3::IntToStr( b ) << '\n';
   }
#endif
   WriteValue( RWType::rw_byte, b );
}

void TXStream::WriteWord( uint16_t W )
{
#ifdef WF_TEXT
   if( fstext )
   {
      static int cnt {};
      *fstext << "WriteWord@" << GetPosition() << "#" << ++cnt << ": " << W << '\n';
   }
#endif
   WriteValue( RWType::rw_word, W );
}

void TXStream::WriteBool( bool B )
{
#ifdef WF_TEXT
   if( fstext )
   {
      static int cnt {};
      *fstext << "WriteBool@" << GetPosition() << "#" << ++cnt << ": " << ( B ? "True" : "False" ) << '\n';
   }
#endif
   WriteValue( RWType::rw_bool, B );
}

void TXStream::WriteChar( char C )
{
#ifdef WF_TEXT
   if( fstext )
   {
      static int cnt {};
      *fstext << "WriteChar@" << GetPosition() << "#" << ++cnt << ": " << C << '\n';
   }
#endif
   WriteValue( RWType::rw_char, C );
}

void TXStream::WritePChar( const char *s, int L )
{
   if( Paranoid ) ParWrite( RWType::rw_pchar );
   WriteInteger( L );
   if( L > 0 ) Write( s, L );
}

std::string TXStream::ReadString()
{
   if( Paranoid ) ParCheck( RWType::rw_string );
   uint8_t len {};
   if( !Read( &len, 1 ) || !len ) return ""s;
   std::string s;
   s.resize( len );
   Read( s.data(), len );
   if(len != s.size())
      s.resize( len );
   return s;
}

double TXStream::ReadDouble()
{
   return ReadValue<double>( RWType::rw_double );
}

int TXStream::ReadInteger()
{
   return ReadValue<int>( RWType::rw_integer );
}

uint8_t TXStream::ReadByte()
{
   return ReadValue<uint8_t>( RWType::rw_byte );
}

uint16_t TXStream::ReadWord()
{
   return ReadValue<uint16_t>( RWType::rw_word );
}

int64_t TXStream::ReadInt64()
{
   return ReadValue<int64_t>( RWType::rw_int64 );
}

bool TXStream::ReadBool()
{
   return ReadValue<bool>( RWType::rw_bool );
}

char TXStream::ReadChar()
{
   return ReadValue<char>( RWType::rw_char );
}

std::string TXStream::ReadPChar( int &L )
{
   if( Paranoid ) ParCheck( RWType::rw_pchar );
   L = ReadInteger();
   if( L <= 0 ) return ""s;
   std::string s;
   s.resize(L);
   Read( s.data(), L );
   if(L != static_cast<int>(s.size()))
      s.resize(L);
   return s;
}

std::string TXStream::ReadPChar()
{
   int dummyLen;
   return ReadPChar(dummyLen);
}

void TXStream::ReadPChar( char *P, int &L )
{
   if( Paranoid ) ParCheck( RWType::rw_pchar );
   L = ReadInteger();
   if( L <= 0 ) P = nullptr;
   else
   {
      P = new char[L];
      Read( P, L );
   }
}

void TXStream::ActiveWriteOpTextDumping( const std::string &dumpFilename )
{
#ifdef WF_TEXT
   fstext = std::make_unique<std::ofstream>( dumpFilename );
#endif
}

void TXFileStream::SetLastIOResult( int V )
{
   if( !FLastIOResult ) FLastIOResult = V;
}

void TXFileStream::SetPassWord( const std::string &s )
{
   FPassWord.clear();
   if( s.empty() ) return;
   bool BB {};
   for( int K {}; K < static_cast<int>( s.length() ); K++ )
   {
      if( s[K] != ' ' ) BB = false;
      else
      {
         if( BB ) continue;
         BB = true;
      }
      uint8_t B { static_cast<unsigned char>(s[K]) };
      if( !( B & 1 ) ) B >>= 1;
      else B = static_cast<unsigned char>( 0x80 + ( B >> 1 ) );
      FPassWord += static_cast<char>(B);
   }
}

bool TXFileStream::GetUsesPassWord() const
{
   return !FPassWord.empty();
}

std::string TXFileStream::RandString( int L )
{
   int Seed {};
   auto RandCh = [&]() {
      Seed = ( Seed * 12347 + 1023 ) & 0x7FFFFFF;
      return static_cast<char>( Seed & 0xFF );
   };
   Seed = 1234 * L;
   return utils::constructStr( L, [&]( int i ) { return RandCh(); } );
}

int64_t TXFileStream::GetSize()
{
#ifdef __IN_CPPMEX__
   int64_t res;
   SetLastIOResult( p3FileGetSize( FS, res ) );
   return res;
#else
   return 0;
#endif
}

int64_t TXFileStream::GetPosition()
{
   return PhysPosition;
}

void TXFileStream::SetPosition( int64_t P )
{
   PhysPosition = P;
   int64_t NP;
   SetLastIOResult( rtl::p3utils::p3FileSetPointer( FS, P, NP, rtl::p3utils::p3_FILE_BEGIN ) );
}

TXFileStream::TXFileStream( std::string AFileName, const FileAccessMode AMode )
    : FFileName { std::move( AFileName ) }
{
   Tp3FileOpenAction FMode { p3OpenRead };
   switch( AMode )
   {
      case fmCreate:
      case fmOpenWrite:
         FMode = p3OpenWrite;
         break;
      case fmOpenRead:
         FMode = p3OpenRead;
         break;
      case fmOpenReadWrite:
         FMode = p3OpenReadWrite;
         break;
      default:
         throw std::runtime_error( "TXFileStream.Create = "s + rtl::sysutils_p3::IntToStr( AMode ) );
   }
   SetLastIOResult( p3FileOpen( FFileName, FMode, FS ) );
   FileIsOpen = !FLastIOResult;
}

TXFileStream::~TXFileStream()
{
   if( FileIsOpen )
      SetLastIOResult( p3FileClose(FS) );
}

void TXFileStream::ApplyPassWord( const char *PR, char *PW, int Len, int64_t Offs ) const
{
   const auto L = static_cast<int>( FPassWord.length() );
   auto FPwNxt = static_cast<int>( Offs ) % L;
   for( int N {}; N < Len; N++ )
   {
      FPwNxt++;
      if( FPwNxt > L ) FPwNxt = 1;
      PW[N] = static_cast<char>( PR[N] ^ FPassWord[FPwNxt] );
   }
}

uint32_t TXFileStream::Read( void *Buffer, uint32_t Count )
{
   uint32_t res;
   if( FPassWord.empty() )
      SetLastIOResult( p3FileRead( FS, static_cast<char *>( Buffer ), Count, res ) );
   else
   {
      const auto PW = static_cast<char *>( Buffer );
      std::vector<char> PR( Count );
      SetLastIOResult( p3FileRead( FS, PR.data(), Count, res ) );
      ApplyPassWord( PR.data(), PW, static_cast<int>( Count ), PhysPosition );
   }
   PhysPosition += res;
   return res;
}

uint32_t TXFileStream::Write( const void *Buffer, uint32_t Count )
{
   uint32_t res {};
   if( FPassWord.empty() )
      SetLastIOResult( p3FileWrite( FS, static_cast<const char *>( Buffer ), Count, res ) );
   else
   {
      const auto PR = static_cast<const char *>( Buffer );
      std::vector<char> PW( Count );
      ApplyPassWord( PR, PW.data(), static_cast<int>( Count ), PhysPosition );
      SetLastIOResult( p3FileWrite( FS, PW.data(), Count, res ) );
   }
   PhysPosition += res;
   return res;
}

int TXFileStream::GetLastIOResult()
{
   const int res { FLastIOResult };
   FLastIOResult = 0;
   return res;
}

std::string TXFileStream::GetFileName() const
{
   return FFileName;
}

bool TBufferedFileStream::FillBuffer()
{
   if( !FCompress ) NrLoaded = TXFileStream::Read( BufPtr.data(), BufSize );
   else if( !FCanCompress )
   {
      NrLoaded = 0;
      FLastIOResult = -100044;// check with gxdefs.pas
   }
   else
   {
      if( const uint16_t RLen = TXFileStream::Read( &CBufPtr->cxHeader, sizeof( TCompressHeader ) );
         RLen < sizeof( TCompressHeader ) )
         NrLoaded = 0;
      else
      {
         const uint16_t WLen = ( CBufPtr->cxHeader.cxB1 << 8 ) + CBufPtr->cxHeader.cxB2;
         if( !CBufPtr->cxHeader.cxTyp ) NrLoaded = TXFileStream::Read( BufPtr.data(), WLen );
         else
         {
            TXFileStream::Read( &CBufPtr->cxData, WLen );
            unsigned long XLen = BufSize;// we need a var parameter
            uncompress( BufPtr.data(), &XLen, &CBufPtr->cxData, WLen );
            NrLoaded = XLen;
         }
      }
   }
   NrRead = NrWritten = 0;
   return NrLoaded > 0;
}

int64_t TBufferedFileStream::GetPosition()
{
   if( !NrWritten ) return PhysPosition - NrLoaded + NrRead;
   if( FCompress ) FlushBuffer();
   return PhysPosition + NrWritten;
}

void TBufferedFileStream::SetPosition( int64_t p )
{
   if( NrWritten > 0 )
   {
      if( p == PhysPosition + NrWritten && !FCompress ) return;
      FlushBuffer();
   }
   if( NrLoaded > 0 && !FCompress )
   {
      const int64_t StartOfBuf { PhysPosition - NrLoaded };
      if( p >= StartOfBuf && p < PhysPosition )
      {
         NrRead = static_cast<uint32_t>( p - StartOfBuf );
         return;
      }
   }
   TXFileStream::SetPosition( p );
   NrLoaded = NrRead = 0;
}

int64_t TBufferedFileStream::GetSize()
{
   int64_t res { TXFileStream::GetSize() };
   if( NrWritten > 0 ) res = std::max( res, PhysPosition + NrWritten );
   return res;
}

TBufferedFileStream::TBufferedFileStream( const std::string &FileName, uint16_t Mode )
    : TXFileStream { FileName, static_cast<FileAccessMode>( Mode ) },
      NrLoaded {},
      NrRead {},
      NrWritten {},
      BufSize { BufferSize },
      CBufSize {  utils::round<uint32_t>( static_cast<double>( BufferSize ) * 12.0 / 10.0 ) + 20 },
      BufPtr( BufferSize ),
      CBufPtr { static_cast<PCompressBuffer>( malloc( sizeof( TCompressHeader ) + CBufSize ) ) },
      FCompress {},
      FCanCompress { true }// no longer a fatal error
{
}

TBufferedFileStream::~TBufferedFileStream()
{
   if( NrWritten > 0 )
      FlushBuffer();
   free( CBufPtr );
}

bool TBufferedFileStream::FlushBuffer()
{
   bool res { true };
   uint32_t ActWritten;
   if( !NrWritten ) return res;
   if( !FCompress || !FCanCompress )
   {
      ActWritten = TXFileStream::Write( BufPtr.data(), NrWritten );
      res = NrWritten == ActWritten;
   }
   else
   {
      unsigned long Len = CBufSize - sizeof( TCompressHeader );
      compress( &CBufPtr->cxData, &Len, BufPtr.data(), NrWritten );
      if( Len < NrWritten )
      {
         CBufPtr->cxHeader.cxTyp = 1;// indicates compressed
         CBufPtr->cxHeader.cxB1 = static_cast<uint8_t>( Len >> 8 );
         CBufPtr->cxHeader.cxB2 = Len & 0xFF;
         Len += sizeof( TCompressHeader );
         ActWritten = TXFileStream::Write( &CBufPtr->cxHeader.cxTyp, Len );
         res = Len == ActWritten;
      }
      else
      {
         CBufPtr->cxHeader.cxTyp = 0;// indicates no compression
         CBufPtr->cxHeader.cxB1 = NrWritten >> 8;
         CBufPtr->cxHeader.cxB2 = NrWritten & 0xFF;
         TXFileStream::Write( &CBufPtr->cxHeader.cxTyp, sizeof( TCompressHeader ) );
         ActWritten = TXFileStream::Write( BufPtr.data(), NrWritten );
         res = NrWritten == ActWritten;
      }
   }
   NrWritten = NrLoaded = NrRead = 0;
   return res;
}

uint32_t TBufferedFileStream::Read( void *Buffer, uint32_t Count )
{
   if( NrWritten > 0 ) FlushBuffer();
   if( Count <= NrLoaded - NrRead )
   {
      std::memcpy( Buffer, &BufPtr[NrRead], Count );
      NrRead += Count;
      return Count;
   }
   auto *UsrPtr = static_cast<char *>( Buffer );
   uint32_t UsrReadCnt = 0;
   while( Count > 0 )
   {
      if( NrRead >= NrLoaded && !FillBuffer() ) break;
      const uint32_t NrBytes = std::min( Count, NrLoaded - NrRead );
      std::memcpy( &UsrPtr[UsrReadCnt], &BufPtr[NrRead], NrBytes );
      NrRead += NrBytes;
      UsrReadCnt += NrBytes;
      Count -= NrBytes;
   }
   return UsrReadCnt;
}

char TBufferedFileStream::ReadCharacter()
{
   if( NrWritten > 0 ) FlushBuffer();
   if( NrRead >= NrLoaded && !FillBuffer() ) return substChar;
   return static_cast<char>( BufPtr[NrRead++] );
}

uint32_t TBufferedFileStream::Write( const void *Buffer, uint32_t Count )
{
   if( NrLoaded > 0 )
   {// we have been reading ahead
      TXFileStream::SetPosition( PhysPosition - NrLoaded + NrRead );
      NrLoaded = NrRead = 0;
   }
   if( Count <= BufSize - NrWritten )
   {// the simple case
      std::memcpy( &BufPtr[NrWritten], Buffer, Count );
      NrWritten += Count;
      return Count;
   }
   const auto UsrPtr = static_cast<const char *>( Buffer );
   int UsrWriteCnt {};// total number of bytes written
   while( Count > 0 )
   {
      const auto NrBytes = std::min( Count, BufSize - NrWritten );
      if( NrBytes > 0 )
         std::memcpy( &BufPtr[NrWritten], &UsrPtr[UsrWriteCnt], NrBytes );
      NrWritten += NrBytes;
      UsrWriteCnt += static_cast<int>( NrBytes );
      Count -= NrBytes;
      if( NrWritten >= BufSize && !FlushBuffer() ) break;
   }
   return UsrWriteCnt;
}

bool TBufferedFileStream::IsEof()
{
   return NrRead >= NrLoaded && GetPosition() >= GetSize();
}

void TBufferedFileStream::SetCompression( bool V )
{
   if( ( FCompress || V ) && NrWritten > 0 ) FlushBuffer();
   if( FCompress != V )
      NrLoaded = NrRead = 0;
   FCompress = V;
}

bool TBufferedFileStream::GetCompression() const { return FCompress; }

bool TBufferedFileStream::GetCanCompress() const { return FCanCompress; }


void TMiBufferedStream::DetermineByteOrder()
{
   initOrderCommon<uint16_t>( order_word, size_word, PAT_WORD );
   initOrderCommon<int>( order_integer, size_integer, PAT_INTEGER );
   initOrderCommon<double>( order_double, size_double, PAT_DOUBLE );
}

TMiBufferedStream::TMiBufferedStream( const std::string &FileName, uint16_t Mode ) : TBufferedFileStream { FileName, Mode }
{
   if( FLastIOResult ) return;
   if( Mode != FileAccessMode::fmCreate ) DetermineByteOrder();// we cannot update a mixed environment file!
   else
   {// avoid using writebyte so Paranoid flag works
      uint8_t B = sizeof( uint16_t );
      Write( &B, sizeof( uint8_t ) );
      const uint16_t W = PAT_WORD;
      Write( &W, sizeof( uint16_t ) );
      B = sizeof( uint32_t /*int*/ );
      Write( &B, sizeof( uint8_t ) );
      const int I = PAT_INTEGER;
      Write( &I, sizeof( uint32_t /*int*/ ) );
      B = sizeof( uint64_t /*double*/ );
      Write( &B, sizeof( uint8_t ) );
      const double D = PAT_DOUBLE;
      Write( &D, sizeof( double ) );
   }
   TDoubleVar X {};
   X.V = 1.0;
   NormalOrder = !X.VA.front();
}

//note: this only works when src and dest point to different areas
void TMiBufferedStream::ReverseBytes( void *psrc, void *pdest, int sz )
{
   auto pdestc {static_cast<char *>( pdest )},
        psrcc {static_cast<char *>( psrc )};
   pdestc += sz - 1;
   for( int k { 0 }; k < sz; k++ )
   {
      *pdestc = *psrcc;
      psrcc++;
      pdestc--;
   }
}

int TMiBufferedStream::GoodByteOrder() const
{
   int res {};
   if( order_word == PAT_BAD_SIZE ) res += 1;
   if( order_word == PAT_BAD_ORDER ) res += 2;
   if( order_integer == PAT_BAD_SIZE ) res += 4;
   if( order_integer == PAT_BAD_ORDER ) res += 8;
   if( order_double == PAT_BAD_SIZE ) res += 16;
   if( order_double == PAT_BAD_ORDER ) res += 32;
   return res;
}

double TMiBufferedStream::ReadDouble()
{
   return ReadValueOrdered<double>( RWType::rw_double, order_double );
}

int TMiBufferedStream::ReadInteger()
{
   return ReadValueOrdered<int>( RWType::rw_integer, order_integer );
}

uint16_t TMiBufferedStream::ReadWord()
{
   return ReadValueOrdered<uint16_t>( RWType::rw_word, order_word );
}

int64_t TMiBufferedStream::ReadInt64()
{
   return ReadValueOrdered<int64_t>( RWType::rw_int64, order_integer );
}

bool TMiBufferedStream::WordsNeedFlip() const
{
   return order_word;
}

bool TMiBufferedStream::IntsNeedFlip() const
{
   return order_integer;
}

void TMiBufferedStream::WriteGmsInteger( int N )
{
#ifdef WF_TEXT
   if( fstext )
   {
      static int cnt {};
      *fstext << "WriteGmsInteger@" << GetPosition() << "#" << ++cnt << ": " << N << '\n';
   }
#endif

   uint8_t B { static_cast<uint8_t>( N >= 0 ? 0 : 128 ) };
   if( N < 0 ) N = -N;
   B |= N & 15;
   N >>= 4;
   int C {};
   std::array<uint8_t, 5> W {};
   while( N )
   {
      W[++C] = N & 255;
      N >>= 8;
   }
   W[0] = B | C << 4;
   Write( W.data(), C + 1 );
}

enum tgmsvalue : uint8_t
{
   xvreal,
   xvund,
   xvna,
   xvpin,
   xvmin,
   xveps,
   xvacr
};

static tgmsvalue mapval( double x )
{
   if( x < GMS_SV_UNDEF ) return xvreal;
   if( x >= GMS_SV_ACR ) return xvacr;
   x /= GMS_SV_UNDEF;
   const int k = utils::round<int>( x );
   if( std::abs( k - x ) > 1.0e-5 )
      return xvund;
   constexpr std::array<tgmsvalue, 5> kToRetMapping = {
           xvund, xvna, xvpin, xvmin, xveps };
   return k >= 1 && k <= static_cast<int>( kToRetMapping.size() ) ? kToRetMapping[k - 1] : xvacr;
}

void TMiBufferedStream::WriteGmsDouble( double D )
{
#ifdef WF_TEXT
   static int cnt {};
   if( fstext )
      *fstext << "WriteGmsDouble@" << GetPosition() << "#" << ++cnt << ": " << utils::asdelphifmt( D ) << '\n';
#endif

   const tgmsvalue gv = mapval( D );
   uint8_t B = gv;
   if( gv == xvreal )
   {
      if( D == 0.0 ) B = 7;
      else if( D == 1.0 )
         B = 8;
      else if( D == -1.0 )
         B = 9;
   }
   if( B )
   {
      Write( &B, 1 );
      if( gv == xvacr ) WriteGmsInteger( utils::round<int>( D / GMS_SV_ACR ) );
      return;
   }
   int C {};
   TDoubleVar Z {};
   Z.V = D;
   if( NormalOrder )
   {
      for( const auto &cell: Z.VA )
      {
         if( !cell ) C++;
         else
            break;
      }
      B = 128 | C;
      Write( &B, 1 );
      assert( C >= 0 && C <= 7 );
      Write( &Z.VA[C], static_cast<uint32_t>( Z.VA.size() ) - C );
   }
   else
   {
      for( int i { static_cast<int>( Z.VA.size() ) - 1 }; i >= 0; i-- )
      {
         if( !Z.VA[i] ) C++;
         else
            break;
      }
      B = 128 | C;
      Write( &B, 1 );
      for( int i = 7 - C; i >= 0; i-- )
         Write( &Z.VA[i], 1 );
   }
}

int TMiBufferedStream::ReadGmsInteger()
{
   uint8_t B;
#if !defined( NDEBUG )
   auto numBytesRead { Read( &B, 1 ) };
   assert( numBytesRead == 1 );
   // should not happen
   if( !numBytesRead )
      B = 0;
#else
   Read( &B, 1 );
#endif
   std::array<uint8_t, 5> W {};
   W[0] = B & 15;
   const bool Neg = B >= 128;
   global::delphitypes::Bounded<int, 0, 6> C { ( B >> 4 ) & 7 };
   if( C > 0 ) Read( &W[1], C );
   int res {};
   while( C >= 1 )
   {
      res = ( res << 8 ) | W[C];
      --C;
   }
   res = ( res << 4 ) | W[0];
   if( Neg ) res *= -1;
   return res;
}

double TMiBufferedStream::ReadGmsDouble()
{
   constexpr static std::array<double, 9> bToRes { GMS_SV_UNDEF, GMS_SV_NA, GMS_SV_PINF, GMS_SV_MINF, GMS_SV_EPS, GMS_SV_ACR, 0.0, 1.0, -1.0 };
   const auto B { ReadByte() };
   if( !( B & 128 ) )
      return B >= 1 && B <= 9 ? ( B == 6 ? ReadGmsInteger() : 1.0 ) * bToRes[B-1] : 0.0;
   TDoubleVar Z {};
   auto C = B & 127;
   if( NormalOrder )
   {
      for( auto &cell: Z.VA )
      {
         if( !C ) cell = ReadByte();
         else
         {
            cell = 0;
            C--;
         }
      }
   }
   else
   {
      for( int i = static_cast<int>( Z.VA.size() ) - 1; i >= 0; i-- )
      {
         if( !C ) Z.VA[i] = ReadByte();
         else
         {
            Z.VA[i] = 0;
            C--;
         }
      }
   }
   return Z.V;
}

TBinaryTextFileIO::TBinaryTextFileIO( const std::string &fn, const std::string &PassWord, int &ErrNr, std::string &errMsg )
: FS{std::make_unique<TBufferedFileStream>( fn, fmOpenRead )}
{
   ErrNr = FS->GetLastIOResult();
   if( ErrNr )
   {
      errMsg = SysErrorMessage( ErrNr );
      ErrNr = strmErrorIOResult;
      return;
   }
   const auto B1 = FS->ReadByte(), B2 = FS->ReadByte();
   if( B1 == 31 && B2 == 139 )
   {//header for gzip
      //assume it is GZIP format
      FFileSignature = fsign_gzip;
      FS = nullptr;
      gzFS = std::make_unique<TGZipInputStream>( fn, errMsg );
      if( !errMsg.empty() ) ErrNr = 1;
      return;
   }

   std::string srcBuf;
   srcBuf.resize( B2 );
   if( B1 == signature_header ) Read( srcBuf.data(), B2 );
   if( B1 != signature_header || srcBuf != signature_gams )
   {// nothing special
      const utils::tBomIndic fileStart { B1, B2, FS->ReadByte(), FS->ReadByte() };
      int BOMOffset;
      if( !utils::checkBOMOffset( fileStart, BOMOffset, errMsg ) )
      {
         ErrNr = strmErrorEncoding;
         return;
      }
      FS->SetPosition( BOMOffset );
      FRewindPoint = BOMOffset;
      FMajorVersionRead = 0;
      FMinorVersionRead = 0;
      FFileSignature = fsign_text;
      errMsg.clear();
      return;
   }
   ErrNr = strmErrorGAMSHeader;
   errMsg = "GAMS header not found";
   FFileSignature = static_cast<TFileSignature>( FS->ReadByte() - utils::ord('A') );
   FS->ReadString();
   FMajorVersionRead = FS->ReadByte();
   FMinorVersionRead = FS->ReadByte();
   char Ch { static_cast<char>( FS->ReadByte() ) };

   bool hasPswd;
   if( Ch == 'P' ) hasPswd = true;
   else if( Ch == 'p' )
      hasPswd = false;
   else
      return;

   Ch = static_cast<char>( FS->ReadByte() );
   bool hasComp;
   if( Ch == 'C' ) hasComp = true;
   else if( Ch == 'c' )
      hasComp = false;
   else
      return;

   if( hasPswd && PassWord.empty() )
   {
      ErrNr = strmErrorNoPassWord;
      errMsg = "A Password is required";
      return;
   }

   ErrNr = strmErrorIntegrity;
   errMsg = "Integrity check failed";

   if( hasPswd )
   {
      FS->SetPassWord( PassWord );
      const std::string src = FS->ReadString();
      std::array<char, 256> targBuf {};
      FS->ApplyPassWord( src.c_str(), targBuf.data(), (int) src.length(), verify_offset );
      if( gdlib::gmsstrm::TBufferedFileStream::RandString( static_cast<int>( src.length() ) ) != std::string(targBuf.data()) ) return;
   }

   FRewindPoint = FS->GetPosition();
   FS->SetCompression( true );
   FS->SetPosition( FRewindPoint );
   if( !hasComp ) FS->SetCompression( false );
   if( FS->ReadString() != signature_gams ) return;
   ErrNr = strmErrorNoError;
   errMsg.clear();
}

TBinaryTextFileIO::TBinaryTextFileIO( const std::string &fn, const std::string &Producer, const std::string &PassWord, TFileSignature signature, bool comp, int &ErrNr, std::string &errMsg )
    : FS {std::make_unique<TBufferedFileStream>( fn, fmCreate )}, frw{fm_write}, FFileSignature{signature}
{
   if( signature != fsign_text || !PassWord.empty() || comp )
   {
      FS->WriteByte( signature_header );
      FS->WriteString( signature_gams );
      FS->WriteByte( signature + utils::ord('A') );
      FS->WriteString( Producer );
      FS->WriteByte( 1 );// version
      FS->WriteByte( 1 );// sub-version
      FS->WriteByte( PassWord.empty() ? 'p' : 'P' );
      FS->WriteByte( comp ? 'C' : 'c' );
      if( !PassWord.empty() )
      {
         FS->FlushBuffer();
         FS->SetPassWord( PassWord );
         std::string src = gdlib::gmsstrm::TBufferedFileStream::RandString( (int) PassWord.length() );
         std::array<char, 256> targBuf {};
         FS->ApplyPassWord( src.c_str(), targBuf.data(), (int) src.length(), verify_offset );
         FS->SetPassWord( "" );
         FS->WriteString( targBuf.data() );
      }
      if( comp ) FS->SetCompression( true );
      else
         FS->FlushBuffer();
      FS->SetPassWord( PassWord );
      //write a few bytes to be recognized later (compression / password is now active)
      FS->WriteString( signature_gams );
   }
   ErrNr = FS->GetLastIOResult();
   if( !ErrNr )
   {
      ErrNr = strmErrorNoError;
      errMsg.clear();
   }
   else
   {
      errMsg = SysErrorMessage( ErrNr );
      FS = nullptr;
   }
}

uint32_t TBinaryTextFileIO::Read( char *Buffer, uint32_t Count )
{
   return FFileSignature == fsign_gzip ? static_cast<int>( gzFS->Read( Buffer, Count ) ) : static_cast<int>( FS->Read( Buffer, Count ) );
}

char TBinaryTextFileIO::ReadCharacter()
{
   if( FFileSignature == fsign_gzip )
   {
      char res{};
      if( !gzFS->Read( &res, 1 ) )
         return substChar;
      return res;
   }
   return FS->ReadCharacter();
}

void TBinaryTextFileIO::ReadLine( std::vector<uint8_t> &Buffer, int &Len, int MaxInp, char &LastChar )
{
   // moved here for performance reasons
   // reading a single byte at a time is avoided this way
   if( FFileSignature == fsign_gzip )
      gzFS->ReadLine( Buffer, MaxInp, LastChar );
   else
   {
      Buffer.clear();
      while( !( utils::in( LastChar, substChar, '\n', '\r' ) || static_cast<int>( Buffer.size() ) == MaxInp ) )
      {
         Buffer.push_back( LastChar );
         if( FS->NrLoaded - FS->NrRead >= 1 )
         {// the simple case
            LastChar = static_cast<char>( FS->BufPtr[FS->NrRead] );
            FS->NrRead++;
         }
         // we should we fill the buffer???
         else if( !FS->Read( &LastChar, 1 ) )
            LastChar = substChar;
      }
   }
   Len = static_cast<int>(Buffer.size());
}

void TBinaryTextFileIO::ReadLine( char *Buffer, int &Len, int MaxInp, char &LastChar )
{
   // moved here for performance reasons
   // reading a single byte at a time is avoided this way
   if( FFileSignature == fsign_gzip )
      gzFS->ReadLine( Buffer, MaxInp, LastChar, Len );
   else
   {
      Len = 0;
      while( LastChar != substChar && LastChar != '\n' && LastChar != '\r' && static_cast<int>( Len ) != MaxInp )
      {
         Buffer[Len++] = LastChar;
         if( FS->NrLoaded - FS->NrRead >= 1 )
         {// the simple case
            LastChar = static_cast<char>( FS->BufPtr[FS->NrRead] );
            FS->NrRead++;
         }
         // we should we fill the buffer???
         else if( !FS->Read( &LastChar, 1 ) )
            LastChar = substChar;
      }
   }
}

void TBinaryTextFileIO::ReadLine( std::string &StrBuffer, int &Len, const int MaxInp, char &LastChar ) const
{
   // moved here for performance reasons
   // reading a single byte at a time is avoided this way
   if( FFileSignature == fsign_gzip )
   {
      gzFS->ReadLine( StrBuffer, MaxInp, LastChar );
      Len = static_cast<int>( StrBuffer.size() );
      return;
   }
   StrBuffer.clear();
   while( !( utils::in( LastChar, substChar, '\n', '\r', static_cast<char>(0x1A) ) || static_cast<int>( StrBuffer.size() ) == MaxInp ) )
   {
      StrBuffer.push_back( LastChar );
      // the simple case
      if( FS->NrLoaded - FS->NrRead >= 1 )
      {
         LastChar = static_cast<char>( FS->BufPtr[FS->NrRead] );
         FS->NrRead++;
      }
      // we should we fill the buffer???
      else if( !FS->Read( &LastChar, 1 ) )
         LastChar = substChar;
   }
   Len = static_cast<int>( StrBuffer.size() );
}

uint32_t TBinaryTextFileIO::Write( const char *Buffer, const uint32_t Count ) const
{
   assert( frw == fm_write && "TBinaryTextFileIO.Read" );
   return static_cast<uint32_t>( !FS ? -1 : FS->Write( Buffer, Count ) );
}

bool TBinaryTextFileIO::UsesPassWord()
{
   return FS && FS->GetUsesPassWord();
}

void TBinaryTextFileIO::ReWind()
{
   assert( frw == fm_read && "TBinaryTextFileIO.ReWind1" );
   assert( FS && "TBinaryTextFileIO.ReWind2" );
   FS->SetPosition( FRewindPoint );
   if( FS->GetCompression() ) FS->ReadString();// skip verification string
}

int TBinaryTextFileIO::GetLastIOResult()
{
   return FS->GetLastIOResult();
}
}// namespace gdlib::gmsstrm
