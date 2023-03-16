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

#include "gmsstrm.h"
#include "gclgms.h"  // for GMS_SV_ACR, GMS_SV_UNDEF, GMS_SV_EPS, GMS_...
#include "utils.h"   // for strConvCppToDelphi
#include <algorithm> // for min
#include <cmath>     // for round, abs
#include <cstdint>   // for uint8_t, uint16_t, uint32_t, int64_t
#include <cstdlib>   // for malloc, free
#include <cstring>   // for memcpy, strerror
#include <filesystem>// for exists
#include <stdexcept> // for runtime_error
#include <string>    // for string, operator+, to_string, operator""s
#include <utility>   // for move

using namespace std::literals::string_literals;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace gdx::gmsstrm
{

std::string SysErrorMessage( int errorCode )
{
#if defined( _WIN32 )
   static std::array<char, 256> errMsgBuf;
   strerror_s( errMsgBuf.data(), (int) errMsgBuf.size(), errorCode );
   char *errMsg = errMsgBuf.data();
#else
   char *errMsg = strerror( errorCode );
#endif
   if( !errMsg ) return "Unknown error " + std::to_string( errorCode );
   return errMsg;
}

enum CustomOpenAction
{
   custOpenRead,
   custOpenWrite,
   custOpenReadWrite
};

int customFileOpen( const std::string &fName, CustomOpenAction mode, std::fstream *h );
int customFileRead( std::fstream *h, char *buffer, uint32_t buflen, uint32_t &numRead );

int customFileOpen( const std::string &fName, CustomOpenAction mode, std::fstream *h )
{
   std::ios::openmode itsMode{ std::ios::binary };
   switch( mode )
   {
      case custOpenRead:
         itsMode |= std::ios::in;
         break;
      case custOpenWrite:
         itsMode |= std::ios::out;
         break;
      case custOpenReadWrite:
         itsMode |= std::ios::in | std::ios::out;
         break;
   }
   h->open( fName, itsMode );
   bool f = h->fail();
   return f && !std::filesystem::exists( fName ) ? 2 : f;
}

int customFileRead( std::fstream *h, char *buffer, uint32_t buflen, uint32_t &numRead )
{
   auto savedPos = h->tellg();
   h->seekg( 0, h->end );
   numRead = std::min<uint32_t>( static_cast<uint32_t>( h->tellg() - savedPos ), buflen );
   h->seekg( savedPos );
   h->read( buffer, numRead );
   return h->bad() ? 1 : 0;
}

union TDoubleVar
{
   double V;
   std::array<uint8_t, 8> VA;
};

// take the bytes of psrc and return them - in reverse order - in pdest
// sz should be at most 8, but we do not promise to check this!
void reverseBytesMax8( void *psrc, void *pdest, int sz )
{
   std::array<uint8_t, 8> orig{}, flip{};
   int n{ std::min( sz - 1, 7 ) };
   std::memcpy( orig.data(), static_cast<const char *>( psrc ), n + 1 );
   for( int k{}; k <= n; k++ )
      flip[k] = orig[n - k];
   std::memcpy( static_cast<char *>( pdest ), flip.data(), n + 1 );
}

void TXStreamDelphi::WriteString( const std::string_view s )
{
   static std::array<char, 256> buf{};
   utils::strConvCppToDelphi( s, buf.data() );
   Write( buf.data(), (uint32_t) s.length() + 1 );
}

void TXStreamDelphi::WriteDouble( double x )
{
   WriteValue( RWType::rw_double, x );
}

void TXStreamDelphi::WriteInteger( int n )
{
   WriteValue( RWType::rw_integer, n );
}

void TXStreamDelphi::WriteInt64( int64_t N )
{
   WriteValue( RWType::rw_int64, N );
}

void TXStreamDelphi::WriteByte( uint8_t b )
{
   WriteValue( RWType::rw_byte, b );
}

void TXStreamDelphi::WriteWord( uint16_t W )
{
   WriteValue( RWType::rw_word, W );
}

void TXStreamDelphi::WriteBool( bool B )
{
   WriteValue( RWType::rw_bool, B );
}

void TXStreamDelphi::WriteChar( char C )
{
   WriteValue( RWType::rw_char, C );
}

void TXStreamDelphi::WritePChar( const char *s, int L )
{
   WriteInteger( L );
   if( L > 0 ) Write( s, L );
}

std::string TXStreamDelphi::ReadString()
{
   uint8_t len;
   if( !Read( &len, 1 ) ) return ""s;
   std::string s;
   s.resize( len + 1 );
   Read( s.data(), len );
   s.resize( len );
   return s;
}

double TXStreamDelphi::ReadDouble()
{
   return ReadValue<double>( RWType::rw_double );
}

int TXStreamDelphi::ReadInteger()
{
   return ReadValue<int>( RWType::rw_integer );
}

uint8_t TXStreamDelphi::ReadByte()
{
   return ReadValue<uint8_t>( RWType::rw_byte );
}

uint16_t TXStreamDelphi::ReadWord()
{
   return ReadValue<uint16_t>( RWType::rw_word );
}

int64_t TXStreamDelphi::ReadInt64()
{
   return ReadValue<int64_t>( RWType::rw_int64 );
}

bool TXStreamDelphi::ReadBool()
{
   return ReadValue<bool>( RWType::rw_bool );
}

char TXStreamDelphi::ReadChar()
{
   return ReadValue<char>( RWType::rw_char );
}

void TXStreamDelphi::ReadPChar( char *P, int &L )
{
   L = ReadInteger();
   if( L <= 0 ) P = nullptr;
   else
   {
      P = new char[L];
      Read( P, L );
   }
}

void TXStreamDelphi::ActiveWriteOpTextDumping( const std::string &dumpFilename ) {}

void TXFileStreamDelphi::SetLastIOResult( int V )
{
   if( !FLastIOResult ) FLastIOResult = V;
}

void TXFileStreamDelphi::SetPassWord( const std::string &s )
{
   FPassWord.clear();
   if( s.empty() ) return;
   bool BB{};
   for( int K{}; K < (int) s.length(); K++ )
   {
      if( s[K] != ' ' ) BB = false;
      else
      {
         if( BB ) continue;
         BB = true;
      }
      char B = s[K];
      if( !( B & 1 ) ) B >>= 1;
      else
         B = (unsigned char) ( 0x80 + ( (unsigned char) B >> 1 ) );
      FPassWord += B;
   }
}

bool TXFileStreamDelphi::GetUsesPassWord()
{
   return !FPassWord.empty();
}

int64_t TXFileStreamDelphi::GetPosition()
{
   return PhysPosition;
}

void TXFileStreamDelphi::SetPosition( int64_t P )
{
   PhysPosition = P;
   FS->seekp( P );
   SetLastIOResult( FS->bad() ? 1 : 0 );
}

TXFileStreamDelphi::TXFileStreamDelphi( std::string AFileName, const FileAccessMode AMode )
    : FS{}, FFileName{ std::move( AFileName ) }, FPassWord{}, FLastIOResult{}, PhysPosition{}
{
   CustomOpenAction FMode{ custOpenRead };
   switch( AMode )
   {
      case FileAccessMode::fmCreate:
      case FileAccessMode::fmOpenWrite:
         FMode = custOpenWrite;
         break;
      case FileAccessMode::fmOpenRead:
         FMode = custOpenRead;
         break;
      case FileAccessMode::fmOpenReadWrite:
         FMode = custOpenReadWrite;
         break;
      default:
         throw std::runtime_error( "TXFileStream.Create = "s + std::to_string( (int) AMode ) );
   }
   FS = std::make_unique<std::fstream>();
   SetLastIOResult( customFileOpen( FFileName, FMode, FS.get() ) );
   FileIsOpen = !FLastIOResult;
}

TXFileStreamDelphi::~TXFileStreamDelphi()
{
   if( FileIsOpen )
   {
      FS->close();
      SetLastIOResult( !FS->good() ? 1 : 0 );
   }
}

void TXFileStreamDelphi::ApplyPassWord( const char *PR, char *PW, int Len, int64_t Offs )
{
   const int L = (int) FPassWord.length();
   int FPwNxt = (int) Offs % L;
   for( int N{}; N < Len; N++ )
   {
      FPwNxt++;
      if( FPwNxt > L ) FPwNxt = 1;
      PW[N] = (char) ( PR[N] ^ FPassWord[FPwNxt] );
   }
}

uint32_t TXFileStreamDelphi::Read( void *Buffer, uint32_t Count )
{
   uint32_t res;
   if( FPassWord.empty() )
      SetLastIOResult( customFileRead( FS.get(), static_cast<char *>( Buffer ), Count, res ) );
   else
   {
      auto PW = static_cast<char *>( Buffer );
      auto PR = std::unique_ptr<char> {new char[Count]};
      SetLastIOResult( customFileRead( FS.get(), PR.get(), Count, res ) );
      ApplyPassWord( PR.get(), PW, (int) Count, PhysPosition );
   }
   PhysPosition += res;
   return res;
}

uint32_t TXFileStreamDelphi::Write( const void *Buffer, uint32_t Count )
{
   if( FPassWord.empty() )
   {
      FS->write( static_cast<const char *>( Buffer ), Count );
   }
   else
   {
      auto PR = static_cast<const char *>( Buffer );
      auto PW = std::unique_ptr<char> { new char[Count] };
      ApplyPassWord( PR, PW.get(), (int) Count, PhysPosition );
   }
   SetLastIOResult( FS->bad() ? 1 : 0 );
   PhysPosition += Count;
   return Count;
}

int TXFileStreamDelphi::GetLastIOResult()
{
   int res{ FLastIOResult };
   FLastIOResult = 0;
   return res;
}

std::string TXFileStreamDelphi::GetFileName() const
{
   return FFileName;
}

bool TBufferedFileStreamDelphi::FillBuffer()
{
   if( !FCompress ) NrLoaded = TXFileStreamDelphi::Read( BufPtr.data(), BufSize );
   else if( !FCanCompress )
   {
      NrLoaded = 0;
      FLastIOResult = -100044;// check with gxdefs.pas
   }
   else
   {
      uint16_t RLen = TXFileStreamDelphi::Read( &CBufPtr->cxHeader, sizeof( TCompressHeader ) );
      if( RLen < sizeof( TCompressHeader ) ) NrLoaded = 0;
      else
      {
         uint16_t WLen = ( CBufPtr->cxHeader.cxB1 << 8 ) + CBufPtr->cxHeader.cxB2;
         if( !CBufPtr->cxHeader.cxTyp ) NrLoaded = TXFileStreamDelphi::Read( BufPtr.data(), WLen );
         else
         {
            TXFileStreamDelphi::Read( &CBufPtr->cxData, WLen );
            unsigned long XLen = BufSize;// we need a var parameter
            uncompress( BufPtr.data(), &XLen, &CBufPtr->cxData, WLen );
            NrLoaded = XLen;
         }
      }
   }
   NrRead = NrWritten = 0;
   return NrLoaded > 0;
}

int64_t TBufferedFileStreamDelphi::GetPosition()
{
   if( !NrWritten ) return PhysPosition - NrLoaded + NrRead;
   if( FCompress ) FlushBuffer();
   return PhysPosition + NrWritten;
}

void TBufferedFileStreamDelphi::SetPosition( int64_t p )
{
   if( NrWritten > 0 )
   {
      if( p == PhysPosition + NrWritten && !FCompress ) return;
      FlushBuffer();
   }
   if( NrLoaded > 0 && !FCompress )
   {
      int64_t StartOfBuf{ PhysPosition - NrLoaded };
      if( p >= StartOfBuf && p < PhysPosition )
      {
         NrRead = (uint32_t) ( p - StartOfBuf );
         return;
      }
   }
   TXFileStreamDelphi::SetPosition( p );
   NrLoaded = NrRead = 0;
}

TBufferedFileStreamDelphi::TBufferedFileStreamDelphi( const std::string &FileName, uint16_t Mode )
    : TXFileStreamDelphi{ FileName, (FileAccessMode) Mode },
      NrLoaded{},
      NrRead{},
      NrWritten{},
      BufSize{ BufferSize },
      CBufSize{ (uint32_t) std::round( (double) BufferSize * 12.0 / 10.0 ) + 20 },
      BufPtr( BufferSize ),
      CBufPtr{ (PCompressBuffer) malloc( sizeof( TCompressHeader ) + CBufSize ) },
      FCompress{},
      FCanCompress{ true }// no longer a fatal error
{
}

TBufferedFileStreamDelphi::~TBufferedFileStreamDelphi()
{
   if( NrWritten > 0 )
      FlushBuffer();
   free( CBufPtr );
}

bool TBufferedFileStreamDelphi::FlushBuffer()
{
   bool res{ true };
   uint32_t ActWritten;
   if( !NrWritten ) return res;
   if( !FCompress || !FCanCompress )
   {
      ActWritten = TXFileStreamDelphi::Write( BufPtr.data(), NrWritten );
      res = NrWritten == ActWritten;
   }
   else
   {
      unsigned long Len = CBufSize - sizeof( TCompressHeader );
      compress( &CBufPtr->cxData, &Len, BufPtr.data(), NrWritten );
      if( Len < NrWritten )
      {
         CBufPtr->cxHeader.cxTyp = 1;// indicates compressed
         CBufPtr->cxHeader.cxB1 = (uint8_t) ( Len >> 8 );
         CBufPtr->cxHeader.cxB2 = Len & 0xFF;
         Len += sizeof( TCompressHeader );
         ActWritten = TXFileStreamDelphi::Write( &CBufPtr->cxHeader.cxTyp, Len );
         res = Len == ActWritten;
      }
      else
      {
         CBufPtr->cxHeader.cxTyp = 0;// indicates no compression
         CBufPtr->cxHeader.cxB1 = NrWritten >> 8;
         CBufPtr->cxHeader.cxB2 = NrWritten & 0xFF;
         TXFileStreamDelphi::Write( &CBufPtr->cxHeader.cxTyp, sizeof( TCompressHeader ) );
         ActWritten = TXFileStreamDelphi::Write( BufPtr.data(), NrWritten );
         res = NrWritten == ActWritten;
      }
   }
   NrWritten = NrLoaded = NrRead = 0;
   return res;
}

uint32_t TBufferedFileStreamDelphi::Read( void *Buffer, uint32_t Count )
{
   if( NrWritten > 0 ) FlushBuffer();
   if( Count <= NrLoaded - NrRead )
   {
      std::memcpy( Buffer, &BufPtr[NrRead], Count );
      NrRead += Count;
      return Count;
   }
   else
   {
      char *UsrPtr = static_cast<char *>( Buffer );
      uint32_t UsrReadCnt = 0;
      while( Count > 0 )
      {
         if( NrRead >= NrLoaded && !FillBuffer() ) break;
         uint32_t NrBytes = std::min( Count, NrLoaded - NrRead );
         std::memcpy( &UsrPtr[UsrReadCnt], &BufPtr[NrRead], NrBytes );
         NrRead += NrBytes;
         UsrReadCnt += NrBytes;
         Count -= NrBytes;
      }
      return UsrReadCnt;
   }
}

char TBufferedFileStreamDelphi::ReadCharacter()
{
   const char substChar = 0x1A;// 26
   if( NrWritten > 0 ) FlushBuffer();
   if( NrRead >= NrLoaded && !FillBuffer() ) return substChar;
   return (char) BufPtr[NrRead++];
}

uint32_t TBufferedFileStreamDelphi::Write( const void *Buffer, uint32_t Count )
{
   if( NrLoaded > 0 )
   {// we have been reading ahead
      TXFileStreamDelphi::SetPosition( PhysPosition - NrLoaded + NrRead );
      NrLoaded = NrRead = 0;
   }
   if( Count <= BufSize - NrWritten )
   {// the simple case
      std::memcpy( &BufPtr[NrWritten], Buffer, Count );
      NrWritten += Count;
      return Count;
   }
   else
   {
      auto UsrPtr = static_cast<const char *>( Buffer );
      int UsrWriteCnt{};// total number of bytes written
      while( Count > 0 )
      {
         auto NrBytes = std::min( Count, BufSize - NrWritten );
         if( NrBytes > 0 )
            std::memcpy( &BufPtr[NrWritten], &UsrPtr[UsrWriteCnt], NrBytes );
         NrWritten += NrBytes;
         UsrWriteCnt += (int) NrBytes;
         Count -= NrBytes;
         if( NrWritten >= BufSize && !FlushBuffer() ) break;
      }
      return UsrWriteCnt;
   }
}

void TBufferedFileStreamDelphi::SetCompression( bool V )
{
   if( ( FCompress || V ) && NrWritten > 0 ) FlushBuffer();
   if( FCompress != V )
      NrLoaded = NrRead = 0;
   FCompress = V;
}

bool TBufferedFileStreamDelphi::GetCompression() const { return FCompress; }

bool TBufferedFileStreamDelphi::GetCanCompress() const { return FCanCompress; }


void TMiBufferedStreamDelphi::DetermineByteOrder()
{
   initOrderCommon<uint16_t>( order_word, size_word, PAT_WORD );
   initOrderCommon<int>( order_integer, size_integer, PAT_INTEGER );
   initOrderCommon<double>( order_double, size_double, PAT_DOUBLE );
}

TMiBufferedStreamDelphi::TMiBufferedStreamDelphi( const std::string &FileName, uint16_t Mode ) : TBufferedFileStreamDelphi{ FileName, Mode }
{
   if( FLastIOResult ) return;
   if( Mode != FileAccessMode::fmCreate ) DetermineByteOrder();// we cannot update a mixed environment file!
   else
   {// avoid using writebyte so Paranoid flag works
      uint8_t B = sizeof( uint16_t );
      Write( &B, sizeof( uint8_t ) );
      uint16_t W = PAT_WORD;
      Write( &W, sizeof( uint16_t ) );
      B = sizeof( int );
      Write( &B, sizeof( uint8_t ) );
      int I = PAT_INTEGER;
      Write( &I, sizeof( int ) );
      B = sizeof( double );
      Write( &B, sizeof( uint8_t ) );
      double D = PAT_DOUBLE;
      Write( &D, sizeof( double ) );
   }
   TDoubleVar X{};
   X.V = 1.0;
   NormalOrder = !X.VA.front();
}

//note: this only works when src and dest point to different areas
void TMiBufferedStreamDelphi::ReverseBytes( void *psrc, void *pdest, int sz )
{
   auto pdestc = static_cast<char *>( pdest );
   auto psrcc = static_cast<char *>( psrc );
   pdestc += sz - 1;
   for( int k{ 0 }; k < sz; k++ )
   {
      *pdestc = *psrcc;
      psrcc++;
      pdestc--;
   }
}

int TMiBufferedStreamDelphi::GoodByteOrder() const
{
   int res{};
   if( order_word == PAT_BAD_SIZE ) res += 1;
   if( order_word == PAT_BAD_ORDER ) res += 2;
   if( order_integer == PAT_BAD_SIZE ) res += 4;
   if( order_integer == PAT_BAD_ORDER ) res += 8;
   if( order_double == PAT_BAD_SIZE ) res += 16;
   if( order_double == PAT_BAD_ORDER ) res += 32;
   return res;
}

double TMiBufferedStreamDelphi::ReadDouble()
{
   return ReadValueOrdered<double>( RWType::rw_double, order_double );
}

int TMiBufferedStreamDelphi::ReadInteger()
{
   return ReadValueOrdered<int>( RWType::rw_integer, order_integer );
}

uint16_t TMiBufferedStreamDelphi::ReadWord()
{
   return ReadValueOrdered<uint16_t>( RWType::rw_word, order_word );
}

int64_t TMiBufferedStreamDelphi::ReadInt64()
{
   return ReadValueOrdered<int64_t>( RWType::rw_int64, order_integer );
}

bool TMiBufferedStreamDelphi::WordsNeedFlip() const
{
   return order_word;
}

bool TMiBufferedStreamDelphi::IntsNeedFlip() const
{
   return order_integer;
}

void TMiBufferedStreamDelphi::WriteGmsInteger( int N )
{
   uint8_t B;
   if( N >= 0 ) B = 0;
   else
   {
      B = 128;
      N *= -1;
   }
   B |= ( N & 15 );
   N >>= 4;
   int C{};
   std::array<uint8_t, 5> W{};
   while( N )
   {
      W[++C] = N & 255;
      N >>= 8;
   }
   W[0] = B | ( C << 4 );
   Write( W.data(), C + 1 );
}

enum tgmsvalue
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
   int k = static_cast<int>( std::round( x ) );
   if( std::abs( k - x ) > 1.0e-5 )
      return xvund;
   constexpr std::array<tgmsvalue, 5> kToRetMapping = {
           xvund, xvna, xvpin, xvmin, xveps };
   return k >= 1 && k <= (int) kToRetMapping.size() ? kToRetMapping[k - 1] : xvacr;
}

void TMiBufferedStreamDelphi::WriteGmsDouble( double D )
{
   tgmsvalue gv = mapval( D );
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
      if( gv == xvacr ) WriteGmsInteger( (int) std::round( D / GMS_SV_ACR ) );
      return;
   }
   int C{};
   TDoubleVar Z{};
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
      Write( &Z.VA[C + 1], (uint32_t) Z.VA.size() - C );
   }
   else
   {
      for( int i{ (int) Z.VA.size() - 1 }; i >= 0; i-- )
      {
         if( !Z.VA[i] ) C++;
         else
            break;
      }
      B = 128 | C;
      Write( &B, 1 );
      for( int i = 8 - C - 1; i >= 0; i-- )
         Write( &Z.VA[i], 1 );
   }
}

int TMiBufferedStreamDelphi::ReadGmsInteger()
{
   uint8_t B;
   Read( &B, 1 );
   std::array<uint8_t, 5> W{};
   W[0] = B & 15;
   bool Neg = B >= 128;
   int C{ ( B >> 4 ) & 7 };
   if( C > 0 ) Read( &W[1], C );
   int res{};
   while( C >= 1 )
   {
      res = ( res << 8 ) | W[C];
      C--;
   }
   res = ( res << 4 ) | W[0];
   if( Neg ) res *= -1;
   return res;
}

double TMiBufferedStreamDelphi::ReadGmsDouble()
{
   const static std::array<double, 9> bToRes{ GMS_SV_UNDEF, GMS_SV_NA, GMS_SV_PINF, GMS_SV_MINF, GMS_SV_EPS, GMS_SV_ACR, 0.0, 1.0, -1.0 };
   auto B{ ReadByte() };
   if( !( B & 128 ) ) return B >= 1 && B <= 9 ? ( B == 6 ? ReadGmsInteger() : 1.0 ) * bToRes[B] : 0.0;
   TDoubleVar Z{};
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
      for( int i{ (int) Z.VA.size() - 1 }; i >= 0; i-- )
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
}// namespace gdx::gmsstrm
