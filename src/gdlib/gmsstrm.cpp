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

/*
* Important remarks by André:
* At first I did not fully port the gmsstrms but instead tried to do a simpler solution just using C++ standard library IO streams
* There I struggled with porting all features like encryption and compression.
* Then I added new classes with suffix Delphi, these should replicate the Delphi classes in full detail
* Long term I want to merge the simplified and full ports of the stream classes in order to get all functionality will still cleaning stuff up.
*/

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <cassert>
#include <sstream>
#include <fstream>

#include "gclgms.h"

#include "gmsstrm.h"
#include "utils.h"

// only supported by MSVC so far :(
//#include <format>

using namespace std::literals::string_literals;

#if defined(__IN_CPPMEX__)
#include "../gdlib/statlib.h"
#include "../rtl/p3utils.h"
using namespace rtl::p3utils;
#else
namespace statlib
{
static void gstatMessage(const std::string &s)
{
   debugStream << "*** "s << s << '\n';
}
}
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
   if( !errMsg ) return "Unknown error " + std::to_string( errorCode );
#endif
   return errMsg;
}

enum CustomOpenAction
{
   custOpenRead,
   custOpenWrite,
   custOpenReadWrite
};

static int customFileOpen( const std::string &fName, CustomOpenAction mode, std::fstream *h )
{
   std::ios::openmode itsMode { std::ios::binary };
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
   h->rdbuf()->pubsetbuf(nullptr, 0);
   h->open( fName, itsMode );
   bool f = h->fail();
   return f && !std::filesystem::exists( fName ) ? 2 : f;
}

// TODO: AS: Evaluate if this should be closer to p3utils::p3FileRead in Delphi (e.g. using WinAPI ReadFile on Windows and POSIX read on UNIX)
static int customFileRead( std::fstream *h, char *buffer, uint32_t buflen, uint32_t &numRead )
{
   auto savedPos { h->tellg() };
   h->seekg( 0, h->end );
   auto bytesRemaining {static_cast<int64_t>( h->tellg() - savedPos )};
   auto sNumRead {std::min<int64_t>( bytesRemaining, buflen )};
   assert( sNumRead >= 0 );
   numRead = static_cast<uint32_t>(sNumRead);
   h->seekg( savedPos );
   h->read( buffer, numRead );
   return h->bad() ? 1 : 0;
}

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

TBinaryTextFileIO *TBinaryTextFileIO::FromString( const std::string &contents, int &ErrNr )
{
   return new TBinaryTextFileIO { contents, ErrNr };
}

int TBinaryTextFileIO::GetLastIOResult() const
{
   return FLastIOResult;
}

// Corresponds to gmsstrm.TBinaryTextFileIO.OpenForRead from CMEX
// AS: Not sure if it makes sense to duplicate the whole gmsstrm type hierarchy, since std::streams already do some of the low level lifting
TBinaryTextFileIO::TBinaryTextFileIO( const std::string &fn, const std::string &PassWord, int &ErrNr, std::string &errmsg )
: FS {std::make_unique<std::fstream>( fn, std::ios::in | std::ios::binary )}
{
   FCanCompress = true;
   ErrNr = strmErrorNoError;
   if( FS->rdstate() & std::ifstream::failbit )
   {
      statlib::gstatMessage( "Unable to open " + fn + " for reading!" );
      FLastIOResult = 1;
   }
   else
      FLastIOResult = 0;
   ErrNr = FLastIOResult;
   if( ErrNr )
   {
      errmsg = SysErrorMessage( ErrNr );
      ErrNr = strmErrorIOResult;
      return;
   }
   uint8_t B1 = ReadByte(), B2 = ReadByte();
   if( B1 == 31 && B2 == 139 )
   {
      FFileSignature = fsign_gzip;
      gzFS = std::make_unique<TGZipInputStream>( fn, errmsg );
      if( !errmsg.empty() ) ErrNr = 1;
      return;
   }
   std::string srcBuf;
   srcBuf.resize( B2 );
   if( B1 == signature_header ) Read( srcBuf.data(), B2 );
   if( B1 != signature_header || srcBuf != signature_gams )
   {
      utils::tBomIndic fileStart = { B1, B2, ReadByte(), ReadByte() };
      int BOMOffset;
      if( !utils::checkBOMOffset( fileStart, BOMOffset, errmsg ) )
      {
         ErrNr = strmErrorEncoding;
         return;
      }
      // TODO: Investigate of SetPosition does additional relevant stuff apart from doing seek(0), Nr{Read,Loaded}=0
      FS->seekg( BOMOffset );
      NrRead = NrLoaded = BOMOffset;
      FRewindPoint = BOMOffset;
      FMajorVersionRead = 0;
      FMinorVersionRead = 0;
      FFileSignature = fsign_text;
      errmsg.clear();
      return;
   }
   ErrNr = strmErrorGAMSHeader;
   errmsg = "GAMS header not found";
   char b {};
   Read( &b, 1 );
   FFileSignature = static_cast<TFileSignature>( utils::ord( b ) - utils::ord( 'A' ) );
   ReadString();
   FMajorVersionRead = ReadByte();
   FMinorVersionRead = ReadByte();
   char Ch { ReadChar() };
   bool hasPswd = Ch == 'P';
   if( !hasPswd && Ch != 'p' ) return;
   Ch = ReadChar();
   bool hasComp = Ch == 'C';
   if( !hasComp && Ch != 'c' ) return;
   if( hasPswd && PassWord.empty() )
   {
      ErrNr = strmErrorNoPassWord;
      errmsg = "A Password is required";
      return;
   }
   ErrNr = strmErrorIntegrity;
   errmsg = "Integrity check failed";
   if( hasPswd )
   {
      SetPassword( PassWord );
      std::string src = ReadString(), targ;
      ApplyPassword( src, targ, verify_offset );
      if( targ != RandString( static_cast<int>( src.length() ) ) ) return;
   }
   // keep buffer logic going
   FRewindPoint = FS->tellg();
   SetCompression( true );
   FS->seekg( FRewindPoint );
   if( !hasComp ) SetCompression( false );
   if( ReadString() != signature_gams ) return;
   ErrNr = strmErrorNoError;
   errmsg.clear();
   if( !noBuffering ) offsetInBuffer = std::make_optional<uint64_t>( 0 );
}

/*
     * Structure of the file header
           header uncompressed / no password applied
           B   #255
           S   '*GAMS*'
           B   file type
           S   producer info
           B   major version
           B   minor version
           C   P/p password used
           C   C/c compression used
           S   A 'random' ShortString of the length of the password based on the password length
           S   '*GAMS*' encrypted compressed
    */
// Corresponds to gmsstrm.TBinaryTextFileIO.OpenForWrite from CMEX
TBinaryTextFileIO::TBinaryTextFileIO( const std::string &fn,
                                      const std::string &Producer,
                                      const std::string &PassWord,
                                      const TFileSignature signature,
                                      bool comp,
                                      int &ErrNr,
                                      std::string &errmsg )
: FS{std::make_unique<std::fstream>( fn, std::ios::out | std::ios::binary )}
{
   NrRead = NrWritten = 0;
   FCanCompress = true;
   FFileSignature = signature;
   frw = fm_write;
   ErrNr = strmErrorNoError;
   if( FS->rdstate() & std::ifstream::failbit )
   {
      statlib::gstatMessage( "Unable to open " + fn + " for writing!" );
      FLastIOResult = 1;
   }
   else
      FLastIOResult = 0;
   if( signature != fsign_text || !PassWord.empty() || comp )
   {
      WriteByte( signature_header );
      WriteString( signature_gams );
      WriteByte( signature + utils::ord('A') );
      WriteString( Producer );
      WriteByte( 1 );// version
      WriteByte( 1 );// sub version
      WriteByte( PassWord.empty() ? 'p' : 'P' );
      WriteByte( comp ? 'C' : 'c' );
      if( !PassWord.empty() )
      {
         FS->flush();
         SetPassword( PassWord );
         const std::string src = RandString( static_cast<int>( PassWord.length() ) );
         std::string targ;
         ApplyPassword( src, targ, verify_offset );
         SetPassword( "" );
         WriteString( targ );
      }
      if( comp ) SetCompression( true );
      else
         FS->flush();
      SetPassword( PassWord );
      // write a few bytes to be recognized later (compression / password is now active)
      WriteString( signature_gams );
   }
   ErrNr = FLastIOResult;
   if( !ErrNr )
   {
      ErrNr = strmErrorNoError;
      errmsg.clear();
   }
   else
      errmsg = SysErrorMessage( ErrNr );
}

TBinaryTextFileIO::~TBinaryTextFileIO() = default;

uint32_t TBinaryTextFileIO::Read( char *Buffer, uint32_t Count )
{
   if( FFileSignature == fsign_gzip )
      return static_cast<int>(gzFS->Read( Buffer, Count ));

   uint32_t numBytesRetrieved { Count };

   // offsetInBuffer < 0 skips buffering
   if( !noBuffering && offsetInBuffer )
   {
      maybeFillReadBuffer();
      const auto bytesRemaining = static_cast<uint32_t>( readBuffer.size() - *offsetInBuffer );
      if( bytesRemaining < Count ) numBytesRetrieved = bytesRemaining;
      std::memcpy( Buffer, &readBuffer[*offsetInBuffer], numBytesRetrieved );
      *offsetInBuffer += numBytesRetrieved;
      NrRead += numBytesRetrieved;
   }
   else
   {
      FS->read( Buffer, Count );
      numBytesRetrieved = static_cast<int>( FS->gcount() );
      NrRead += Count;
   }
   return numBytesRetrieved;
}

char TBinaryTextFileIO::ReadCharacter()
{
   char ch {};
   Read( &ch, 1 );
   return ch;
}

bool TBinaryTextFileIO::UsesPassWord() const
{
   return !FPassword.empty();
}

// FIXME: The behavior of this ReadLine differs from the corresponding Delphi method
// This should be replaced by TBinaryTextFileIODelphi, which more closely resembles the Delphi code soon!
void TBinaryTextFileIO::ReadLine( std::string &Buffer, int &Len, char &LastChar )
{
   if( FFileSignature == fsign_gzip )
   {
      gzFS->ReadLine( Buffer, std::numeric_limits<int>::max(), LastChar );
      Len = static_cast<int>(Buffer.size());
      return;
   }

   for( Buffer.clear(), Len = 0; !utils::in<char>( LastChar, '\r', '\n', static_cast<char>( std::ifstream::traits_type::eof() ) ) && Len < 80001; Len++ )
   {
      Buffer += LastChar;
      if( Read( &LastChar, 1 ) <= 0 )
         LastChar = std::ifstream::traits_type::eof();
   }
}

uint32_t TBinaryTextFileIO::Write( const char *Buffer, uint32_t Count ) const
{
   assert( frw == fm_write && "TBinaryTextFileIO.Write" );
   if( !FS ) return -1;
   FS->write( Buffer, Count );
   return Count;
}

void TBinaryTextFileIO::ReWind()
{
   FS->clear();
   FS->seekg( 0 );
   if( offsetInBuffer && lastReadCount > 0 ) lastReadCount = 0;
}

uint8_t TBinaryTextFileIO::ReadByte()
{
   if( Paranoid ) ParCheck( RWType::rw_byte );
   char buf {};
   Read( &buf, 1 );
   return buf;
}

char TBinaryTextFileIO::ReadChar()
{
   if( Paranoid ) ParCheck( RWType::rw_byte );
   char buf {};
   Read( &buf, 1 );
   return buf;
}

void TBinaryTextFileIO::ParCheck( RWType T )
{
   uint8_t B {};
   Read( reinterpret_cast<char *>(&B), 1 );
   if( B != static_cast<int>( T ) )
   {
      const std::string suffix {
         B < static_cast<int>( RWType::rw_count ) ? RWTypeText[B] : "???"s + std::to_string( B )
      };
      throw std::runtime_error( "Stream check failed: Expected = " + RWTypeText[static_cast<int>( T )] + " Read = " + suffix );
   }
}

std::string TBinaryTextFileIO::ReadString()
{
   if( Paranoid ) ParCheck( RWType::rw_string );
   char len { 0 };
   if( !Read( &len, 1 ) || !len ) return ""s;
   std::string res;
   res.resize( len );
   Read( res.data(), len );
   return res;
}

TBinaryTextFileIO::TBinaryTextFileIO( const std::string &contents, int &ErrNr )
   : FS {std::make_unique<std::stringstream>( contents, std::fstream::in )}
{
   ErrNr = strmErrorNoError;
}

void TBinaryTextFileIO::SetPassword( const std::string &s )
{
   FPassword.clear();
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
      auto B = static_cast<unsigned char>(s[K]);
      if( !( B & 1 ) ) B >>= 1;
      else B = static_cast<unsigned char>( 0x80 + ( B >> 1 ) );
      FPassword += static_cast<char>(B);
   }
}

void TBinaryTextFileIO::ApplyPassword( const std::string &PR, std::string &PW, int64_t Offs ) const
{
   const auto L = FPassword.length();
   int FPwNxt = static_cast<int>( Offs % L );
   std::transform( std::cbegin( PR ), std::cend( PR ), std::begin( PW ), [&]( const char c ) {
      return static_cast<char>( utils::excl_or( c, FPassword[FPwNxt++ % L] ) );
   } );
}

std::string TBinaryTextFileIO::RandString( int L )
{
   int Seed {};
   auto RandCh = [&]() {
      Seed = ( Seed * 12347 + 1023 ) & 0x7FFFFFF;
      return static_cast<char>( Seed & 0xFF );
   };
   Seed = 1234 * L;
   return utils::constructStr( L, [&]( int _ ) { return RandCh(); } );
}

void TBinaryTextFileIO::WriteByte( uint8_t B ) const
{
   if( Paranoid ) ParWrite( RWType::rw_byte );
   FS->write( reinterpret_cast<const char *>( &B ), 1 );
}

void TBinaryTextFileIO::ParWrite( RWType T ) const
{
   const auto B { static_cast<uint8_t>( T ) };
   FS->write( reinterpret_cast<const char *>( &B ), 1 );
}

void TBinaryTextFileIO::WriteString( const std::string_view s ) const
{
   static std::array<char, 256> buf {};
   if( Paranoid ) ParWrite( RWType::rw_string );
   utils::strConvCppToDelphi( s, buf.data() );
   FS->write( buf.data(), static_cast<int>(s.length() + 1) );
}

void TBinaryTextFileIO::SetCompression( const bool V )
{
   if( ( FCompress || V ) && NrWritten > 0 )
      FS->flush();
   if( FCompress != V )
      NrLoaded = NrRead = 0;
   FCompress = V;
}

void TBinaryTextFileIO::maybeFillReadBuffer()
{
   if( !lastReadCount || *offsetInBuffer >= readBuffer.size() )
   {
      FS->read( readBuffer.data(), BufferSize );
      lastReadCount = FS->gcount();
      *offsetInBuffer = 0;
   }
}

void CompressTextFile( const std::string &fn, const std::string &fo, const std::string &PassWord, bool Comp, int &ErrNr, std::string &ErrMsg )
{
   TBinaryTextFileIODelphi Fin { fn, "", ErrNr, ErrMsg };
   if( !ErrMsg.empty() ) return;

   TBinaryTextFileIODelphi Fout { fo, "CompressTextFile", PassWord, fsign_text, Comp, ErrNr, ErrMsg };
   if( !ErrMsg.empty() ) return;

   std::array<char, 4096> Buffer {};
   uint32_t NrRead;
   do {
      NrRead = Fin.Read( Buffer.data(), (int) Buffer.size() );
      if( !NrRead ) break;
      Fout.Write( Buffer.data(), NrRead );
   } while( NrRead >= static_cast<int>( Buffer.size() ) );
}

void UnCompressTextFile( const std::string &fn, const std::string &fo, const std::string &PassWord, int &ErrNr, std::string &ErrMsg )
{
   TBinaryTextFileIODelphi Fin { fn, PassWord, ErrNr, ErrMsg };
   if( !ErrMsg.empty() ) return;

   TBinaryTextFileIODelphi Fout { fo, "", "", fsign_text, false, ErrNr, ErrMsg };
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
   const std::function<bool()> FillBuffer = [&]() {
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

void TXStreamDelphi::ParWrite( RWType T )
{
   Write( &T, 1 );
}

void TXStreamDelphi::ParCheck( RWType T )
{
   uint8_t B;
   Read( &B, 1 );
   if( B != static_cast<int>(T) )
   {
      const std::string msg {( B >= static_cast<int>(RWType::rw_count) ? "???"s + std::to_string( B ) : RWTypeText[B] )};
      throw std::runtime_error( "Stream check failed: Expected = "s + RWTypeText[static_cast<int>(T)] + " Read = "s + msg );
   }
}

void TXStreamDelphi::WriteString( const std::string_view s )
{
   static std::array<char, 256> buf {};
   if( fstext )
   {
      static int cnt {};
      *fstext << "WriteString@" << GetPosition() << "#" << ++cnt << ": " << s << "\n";
   }
   if( Paranoid ) ParWrite( RWType::rw_string );
   utils::strConvCppToDelphi( s, buf.data() );
   Write( buf.data(), static_cast<uint32_t>( s.length() ) + 1 );
}

void TXStreamDelphi::WriteDouble( double x )
{
   if( fstext )
   {
      static int cnt {};
      *fstext << "WriteDouble@" << GetPosition() << "#" << ++cnt << ": " << utils::asdelphifmt( x, 12 ) << '\n';
   }
   WriteValue( RWType::rw_double, x );
}

void TXStreamDelphi::WriteInteger( int n )
{
   if( fstext )
   {
      static int cnt {};
      *fstext << "WriteInteger@" << GetPosition() << "#" << ++cnt << ": " << n << '\n';
   }
   WriteValue( RWType::rw_integer, n );
}

void TXStreamDelphi::WriteInt64( int64_t N )
{
   if( fstext )
   {
      static int cnt {};
      *fstext << "WriteInt64@" << GetPosition() << "#" << ++cnt << ": " << N << '\n';
   }
   WriteValue( RWType::rw_int64, N );
}

void TXStreamDelphi::WriteByte( uint8_t b )
{
   if( fstext )
   {
      static int cnt {};
      *fstext << "WriteByte@" << GetPosition() << "#" << ++cnt << ": " << std::to_string( b ) << '\n';
   }
   WriteValue( RWType::rw_byte, b );
}

void TXStreamDelphi::WriteWord( uint16_t W )
{
   if( fstext )
   {
      static int cnt {};
      *fstext << "WriteWord@" << GetPosition() << "#" << ++cnt << ": " << W << '\n';
   }
   WriteValue( RWType::rw_word, W );
}

void TXStreamDelphi::WriteBool( bool B )
{
   if( fstext )
   {
      static int cnt {};
      *fstext << "WriteBool@" << GetPosition() << "#" << ++cnt << ": " << ( B ? "True" : "False" ) << '\n';
   }
   WriteValue( RWType::rw_bool, B );
}

void TXStreamDelphi::WriteChar( char C )
{
   if( fstext )
   {
      static int cnt {};
      *fstext << "WriteChar@" << GetPosition() << "#" << ++cnt << ": " << C << '\n';
   }
   WriteValue( RWType::rw_char, C );
}

void TXStreamDelphi::WritePChar( const char *s, int L )
{
   if( Paranoid ) ParWrite( RWType::rw_pchar );
   WriteInteger( L );
   if( L > 0 ) Write( s, L );
}

std::string TXStreamDelphi::ReadString()
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

std::string TXStreamDelphi::ReadPChar( int &L )
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

std::string TXStreamDelphi::ReadPChar()
{
   int dummyLen;
   return ReadPChar(dummyLen);
}

void TXStreamDelphi::ReadPChar( char *P, int &L )
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

void TXStreamDelphi::ActiveWriteOpTextDumping( const std::string &dumpFilename )
{
   fstext = std::make_unique<std::ofstream>( dumpFilename );
}

void TXFileStreamDelphi::SetLastIOResult( int V )
{
   if( !FLastIOResult ) FLastIOResult = V;
}

void TXFileStreamDelphi::SetPassWord( const std::string &s )
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

bool TXFileStreamDelphi::GetUsesPassWord() const
{
   return !FPassWord.empty();
}

std::string TXFileStreamDelphi::RandString( int L )
{
   int Seed {};
   auto RandCh = [&]() {
      Seed = ( Seed * 12347 + 1023 ) & 0x7FFFFFF;
      return static_cast<char>( Seed & 0xFF );
   };
   Seed = 1234 * L;
   return utils::constructStr( L, [&]( int i ) { return RandCh(); } );
}

int64_t TXFileStreamDelphi::GetSize()
{
#ifdef __IN_CPPMEX__
   int64_t res;
   SetLastIOResult( p3FileGetSize( FS.get(), res ) );
   return res;
#else
   return 0;
#endif
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
   //SetLastIOResult(P3FileSetPointer(FS, P, NP, P3_FILE_BEGIN));
}

TXFileStreamDelphi::TXFileStreamDelphi( std::string AFileName, const FileAccessMode AMode )
    : FFileName { std::move( AFileName ) }
{
   CustomOpenAction FMode { custOpenRead };
   switch( AMode )
   {
      case fmCreate:
      case fmOpenWrite:
         FMode = custOpenWrite;
         break;
      case fmOpenRead:
         FMode = custOpenRead;
         break;
      case fmOpenReadWrite:
         FMode = custOpenReadWrite;
         break;
      default:
         throw std::runtime_error( "TXFileStream.Create = "s + std::to_string( AMode ) );
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

void TXFileStreamDelphi::ApplyPassWord( const char *PR, char *PW, int Len, int64_t Offs ) const
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

uint32_t TXFileStreamDelphi::Read( void *Buffer, uint32_t Count )
{
   uint32_t res;
   if( FPassWord.empty() )
      SetLastIOResult( customFileRead( FS.get(), static_cast<char *>( Buffer ), Count, res ) );
   else
   {
      const auto PW = static_cast<char *>( Buffer );
      std::vector<char> PR( Count );
      SetLastIOResult( customFileRead( FS.get(), PR.data(), Count, res ) );
      ApplyPassWord( PR.data(), PW, static_cast<int>( Count ), PhysPosition );
   }
   PhysPosition += res;
   return res;
}

uint32_t TXFileStreamDelphi::Write( const void *Buffer, uint32_t Count )
{
   if( FPassWord.empty() )
      FS->write( static_cast<const char *>( Buffer ), Count );
   else
   {
      const auto PR = static_cast<const char *>( Buffer );
      std::vector<char> PW( Count );
      ApplyPassWord( PR, PW.data(), static_cast<int>( Count ), PhysPosition );
   }
   SetLastIOResult( FS->bad() ? 1 : 0 );
   PhysPosition += Count;
   return Count;
}

int TXFileStreamDelphi::GetLastIOResult()
{
   const int res { FLastIOResult };
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
      if( const uint16_t RLen = TXFileStreamDelphi::Read( &CBufPtr->cxHeader, sizeof( TCompressHeader ) );
         RLen < sizeof( TCompressHeader ) )
         NrLoaded = 0;
      else
      {
         const uint16_t WLen = ( CBufPtr->cxHeader.cxB1 << 8 ) + CBufPtr->cxHeader.cxB2;
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
      const int64_t StartOfBuf { PhysPosition - NrLoaded };
      if( p >= StartOfBuf && p < PhysPosition )
      {
         NrRead = static_cast<uint32_t>( p - StartOfBuf );
         return;
      }
   }
   TXFileStreamDelphi::SetPosition( p );
   NrLoaded = NrRead = 0;
}

int64_t TBufferedFileStreamDelphi::GetSize()
{
   int64_t res { TXFileStreamDelphi::GetSize() };
   if( NrWritten > 0 ) res = std::max( res, PhysPosition + NrWritten );
   return res;
}

TBufferedFileStreamDelphi::TBufferedFileStreamDelphi( const std::string &FileName, uint16_t Mode )
    : TXFileStreamDelphi { FileName, static_cast<FileAccessMode>( Mode ) },
      NrLoaded {},
      NrRead {},
      NrWritten {},
      BufSize { BufferSize },
      CBufSize { static_cast<uint32_t>( std::round( static_cast<double>( BufferSize ) * 12.0 / 10.0 ) ) + 20 },
      BufPtr( BufferSize ),
      CBufPtr { static_cast<PCompressBuffer>( malloc( sizeof( TCompressHeader ) + CBufSize ) ) },
      FCompress {},
      FCanCompress { true }// no longer a fatal error
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
   bool res { true };
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
         CBufPtr->cxHeader.cxB1 = static_cast<uint8_t>( Len >> 8 );
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

char TBufferedFileStreamDelphi::ReadCharacter()
{
   if( NrWritten > 0 ) FlushBuffer();
   if( NrRead >= NrLoaded && !FillBuffer() ) return substChar;
   return static_cast<char>( BufPtr[NrRead++] );
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

bool TBufferedFileStreamDelphi::IsEof()
{
   return NrRead >= NrLoaded && GetPosition() >= GetSize();
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

TMiBufferedStreamDelphi::TMiBufferedStreamDelphi( const std::string &FileName, uint16_t Mode ) : TBufferedFileStreamDelphi { FileName, Mode }
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
void TMiBufferedStreamDelphi::ReverseBytes( void *psrc, void *pdest, int sz )
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

int TMiBufferedStreamDelphi::GoodByteOrder() const
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
   static int cnt {};
   if( fstext )
      *fstext << "WriteGmsInteger@" << GetPosition() << "#" << ++cnt << ": " << N << '\n';

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
   const int k = static_cast<int>( std::round( x ) );
   if( std::abs( k - x ) > 1.0e-5 )
      return xvund;
   constexpr std::array<tgmsvalue, 5> kToRetMapping = {
           xvund, xvna, xvpin, xvmin, xveps };
   return k >= 1 && k <= static_cast<int>( kToRetMapping.size() ) ? kToRetMapping[k - 1] : xvacr;
}

void TMiBufferedStreamDelphi::WriteGmsDouble( double D )
{
   static int cnt {};
   if( fstext )
      *fstext << "WriteGmsDouble@" << GetPosition() << "#" << ++cnt << ": " << utils::asdelphifmt( D ) << '\n';

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
      if( gv == xvacr ) WriteGmsInteger( static_cast<int>( std::round( D / GMS_SV_ACR ) ) );
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

int TMiBufferedStreamDelphi::ReadGmsInteger()
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

double TMiBufferedStreamDelphi::ReadGmsDouble()
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

TBinaryTextFileIODelphi::TBinaryTextFileIODelphi( const std::string &fn, const std::string &PassWord, int &ErrNr, std::string &errMsg )
: FS{std::make_unique<TBufferedFileStreamDelphi>( fn, fmOpenRead )}
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
      auto FSPtr { FS.get() };
      assert( FSPtr );
      if( FSPtr->RandString( static_cast<int>( src.length() ) ) != std::string(targBuf.data()) ) return;
   }

   FRewindPoint = FS->GetPosition();
   FS->SetCompression( true );
   FS->SetPosition( FRewindPoint );
   if( !hasComp ) FS->SetCompression( false );
   if( FS->ReadString() != signature_gams ) return;
   ErrNr = strmErrorNoError;
   errMsg.clear();
}

TBinaryTextFileIODelphi::TBinaryTextFileIODelphi( const std::string &fn, const std::string &Producer, const std::string &PassWord, TFileSignature signature, bool comp, int &ErrNr, std::string &errMsg )
    : FS {std::make_unique<TBufferedFileStreamDelphi>( fn, fmCreate )}, frw{fm_write}, FFileSignature{signature}
{
   auto FSptr { FS.get() };
   assert( FSptr );
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
         std::string src = FSptr->RandString( (int) PassWord.length() );
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

uint32_t TBinaryTextFileIODelphi::Read( char *Buffer, uint32_t Count )
{
   return FFileSignature == fsign_gzip ? static_cast<int>( gzFS->Read( Buffer, Count ) ) : static_cast<int>( FS->Read( Buffer, Count ) );
}

char TBinaryTextFileIODelphi::ReadCharacter()
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

void TBinaryTextFileIODelphi::ReadLine( std::vector<uint8_t> &Buffer, int &Len, int MaxInp, char &LastChar )
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

void TBinaryTextFileIODelphi::ReadLine( std::string &StrBuffer, int &Len, const int MaxInp, char &LastChar ) const
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

uint32_t TBinaryTextFileIODelphi::Write( const char *Buffer, const uint32_t Count ) const
{
   assert( frw == fm_write && "TBinaryTextFileIO.Read" );
   return static_cast<uint32_t>( !FS ? -1 : FS->Write( Buffer, Count ) );
}

bool TBinaryTextFileIODelphi::UsesPassWord()
{
   return FS && FS->GetUsesPassWord();
}

void TBinaryTextFileIODelphi::ReWind()
{
   assert( frw == fm_read && "TBinaryTextFileIO.ReWind1" );
   assert( FS && "TBinaryTextFileIO.ReWind2" );
   FS->SetPosition( FRewindPoint );
   if( FS->GetCompression() ) FS->ReadString();// skip verification string
}

int TBinaryTextFileIODelphi::GetLastIOResult()
{
   return FS->GetLastIOResult();
}
}// namespace gdlib::gmsstrm
