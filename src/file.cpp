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

#include "file.h"

#if defined( _WIN32 )
#include <windows.h>
#else
#include <sys/stat.h>
#include <fcntl.h>
#include <csignal>
#include <unistd.h>
#endif

#include <filesystem>
#include <array>

#include <cassert>

namespace gdx::file
{

int FStreamFile::open( const std::string &fileName, FileOpenAction mode )
{
   std::ios::openmode itsMode { std::ios::binary };
   switch( mode )
   {
      case fileOpenRead:
         itsMode |= std::ios::in;
         break;
      case fileOpenWrite:
         itsMode |= std::ios::out;
         break;
      case fileOpenReadWrite:
         itsMode |= std::ios::in | std::ios::out;
         break;
   }
   fs.rdbuf()->pubsetbuf( nullptr, 0 );
   fs.open( fileName, itsMode );
   bool f = fs.fail();
   return f && !std::filesystem::exists( fileName ) ? 2 : f;
}

int FStreamFile::close()
{
   if(!fs.is_open()) return 1;
   fs.close();
   return 0;
}

int FStreamFile::read( char *buffer, uint32_t buflen, uint32_t &numRead )
{
   if(!fs.is_open()) return 1;
   auto savedPos { fs.tellg() };
   fs.seekg( 0, fs.end );
   auto bytesRemaining { static_cast<int64_t>( fs.tellg() - savedPos ) };
   auto sNumRead { std::min<int64_t>( bytesRemaining, buflen ) };
   assert( sNumRead >= 0 );
   numRead = static_cast<uint32_t>( sNumRead );
   fs.seekg( savedPos );
   fs.read( buffer, numRead );
   return fs.bad() ? 1 : 0;
}

int FStreamFile::seek( int64_t p )
{
   assert(fs.is_open());
   fs.seekp( p );
   return fs.good() ? 0 : 1;
}

int FStreamFile::write( const char *buffer, uint32_t count )
{
   if(!fs.is_open()) return 1;
   fs.write( buffer, count );
   return fs.good() ? 0 : 1;
}

FStreamFile::~FStreamFile() = default;

//===================================================================================================================

#if defined( _WIN32 )

static int win2c( int rc );

static const std::array<DWORD, 3> accessMode {
        GENERIC_READ,
        GENERIC_WRITE,
        GENERIC_READ | GENERIC_WRITE };

// this works for GDX, so we do it:
// it is kind of silly to use the shareMode var then but why not?
static const std::array<DWORD, 3> shareMode {
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE };

static const std::array<DWORD, 3> createHow { OPEN_EXISTING, CREATE_ALWAYS, OPEN_ALWAYS };

static inline bool handleIsValid( const void *handle )
{
   return handle && handle != INVALID_HANDLE_VALUE;
}

int WinAPIFile::open( const std::string &fileName, FileOpenAction mode )
{
   DWORD lowMode { (DWORD) mode & 3 };
   if( 3 == lowMode )
   {
      handle = INVALID_HANDLE_VALUE;
      return ERROR_INVALID_PARAMETER;
   }

   HANDLE hFile;
   if( fileName.empty() )
   {
      if( FileOpenAction::fileOpenRead == mode )
         hFile = GetStdHandle( STD_INPUT_HANDLE );
      else if( FileOpenAction::fileOpenWrite == mode )
         hFile = GetStdHandle( STD_OUTPUT_HANDLE );
      else
      {
         handle = INVALID_HANDLE_VALUE;
         return ERROR_INVALID_PARAMETER;
      }
   }
   else
      hFile = CreateFileA( fileName.c_str(), accessMode[lowMode], shareMode[lowMode], nullptr, createHow[lowMode], FILE_ATTRIBUTE_NORMAL, nullptr );
   if( INVALID_HANDLE_VALUE == hFile )
   {
      handle = INVALID_HANDLE_VALUE;
      auto rc = GetLastError();
      auto res = win2c( static_cast<int>( rc ) );
      // !res? ouch: just pick a likely but non-specific code
      return !res ? EACCES : res;
   }
   else
   {
      handle = hFile;
      return 0;
   }
}

// map the Windows codes returned by GetLastError to libc codes
// use when making Windows API calls with P3, since P3 ioresult-ish codes
// are expected to be libc codes on all platforms
static int win2c( int rc )
{
   int result;
   switch( rc )
   {
      case ERROR_FILE_NOT_FOUND:
      case ERROR_PATH_NOT_FOUND:
         result = ENOENT;
         break;
      case ERROR_TOO_MANY_OPEN_FILES:
         result = EMFILE;
         break;
      case ERROR_ACCESS_DENIED:
         result = EACCES;
         break;
      case ERROR_INVALID_HANDLE:
         result = EBADF;
         break;
      case ERROR_NOT_ENOUGH_MEMORY:
         result = ENOMEM;
         break;
      case ERROR_INVALID_ACCESS:
         result = EACCES;
         break;
      case ERROR_NO_MORE_FILES:
         result = ENFILE;
         break;
      case ERROR_SEEK_ON_DEVICE:
         result = ESPIPE;
         break;
      case ERROR_INVALID_PARAMETER:
      case ERROR_NEGATIVE_SEEK:
         result = EINVAL;
         break;
      default:
         result = 0; /* no guessing */
   }                 /* case */
   return result;
}

int WinAPIFile::read( char *buffer, uint32_t buflen, uint32_t &numRead )
{
   if( !handleIsValid( handle ) ) return EIO;
   if( ReadFile( handle, buffer, buflen, (LPDWORD) &numRead, nullptr ) ) return 0;
   auto rc { GetLastError() };
   auto res { win2c( static_cast<int>( rc ) ) };
   if( !res ) res = EIO;
   return res;
}

int WinAPIFile::write( const char *buffer, uint32_t count )
{
   if( !handleIsValid( handle ) ) return EIO;
   uint32_t numwritten;
   if( WriteFile( handle, buffer, count, (LPDWORD) &numwritten, nullptr ) )
      return 0;// success
   auto res = win2c( static_cast<int>( GetLastError() ) );
   // !res -> ouch: just pick a likely but non-specific code
   return !res ? EIO : res;
}

int WinAPIFile::close()
{
   assert( handle && handle != INVALID_HANDLE_VALUE );
   if( CloseHandle( handle ) ) return 0;// success
   auto res { win2c( static_cast<int>( GetLastError() ) ) };
   handle = INVALID_HANDLE_VALUE;
   // !res? -> ouch: just pick a likely but non-specific code
   return !res ? EIO : res;
}

typedef BOOL( WINAPI *SetFilePointerEx_t )( HANDLE h, LARGE_INTEGER distance, PLARGE_INTEGER newPointer, DWORD whence );

int WinAPIFile::seek( int64_t p )
{
   if( !handleIsValid( handle ) ) return EIO;
   static bool triedSetFilePointerEx {};
   static SetFilePointerEx_t pSetFilePointerEx {};
   // declared as a union - compiler rejects a cast
   LARGE_INTEGER d;
   if( !triedSetFilePointerEx )
   {
      pSetFilePointerEx = (SetFilePointerEx_t) GetProcAddress( GetModuleHandleA( "kernel32" ), "SetFilePointerEx" );
      triedSetFilePointerEx = true;
   }
   d.QuadPart = p;
   BOOL frc;
   if( pSetFilePointerEx )
   {
      int64_t newPos;
      frc = pSetFilePointerEx( (HANDLE) handle, d, (PLARGE_INTEGER) &newPos, 0 );
   }
   else
   {
      DWORD trc = SetFilePointer( (HANDLE) handle, (int) p, nullptr, 0 );
      if( INVALID_SET_FILE_POINTER == trc )
      {
         frc = 0;
      }
      else
      {
         frc = 1;
      }
   }
   if( frc ) return 0;
   else
   {
      auto res { win2c( static_cast<int>( GetLastError() ) ) };
      return !res ? EINVAL : res;
   }
}

WinAPIFile::~WinAPIFile()
{
   if( handleIsValid( handle ) )
      close();
}

#endif

//===================================================================================================================

#if !defined( _WIN32 )
int POSIXFile::open( const std::string &fileName, FileOpenAction mode )
{
   if( fileName.empty() )
   {
      if( FileOpenAction::fileOpenRead == mode )
         h = STDIN_FILENO;
      else if( FileOpenAction::fileOpenWrite == mode )
         h = STDOUT_FILENO;
      else
      {
         h = 0;
         return -1;
      }
      return 0;
   }
   int flags = mode & 3;
   if( flags > 0 )// write-only or read-write
      flags |= O_CREAT;
   if( flags & 1 )
      flags |= O_TRUNC;
   int fd = ::open( fileName.c_str(), flags, 0666 );
   if( -1 == fd )
   {
      h = 0;
      return errno;
   }
   int res {};
   // before calling this a success, check for directory on read-only
   if( FileOpenAction::fileOpenRead == mode )
   {
      struct stat statBuf {
      };
      int rc = fstat( fd, &statBuf );
      if( rc )
         res = errno;
      else if( S_ISDIR( statBuf.st_mode ) )
         res = EISDIR;
   }
   if( res )
   {
      ::close( fd );
      return res;
   }
   h = fd;
   return res;
}

int POSIXFile::read( char *buffer, uint32_t buflen, uint32_t &numRead )
{
   numRead = 0;
   if(!h) return errno;
   auto rc {::read( h, buffer, buflen )};
   if( rc < 0 )
      return errno;
   else
   {
      numRead = rc;
      return 0;
   }
}

int POSIXFile::write( const char *buffer, uint32_t count )
{
   return !h || ::write( h, buffer, count ) < 0 ? errno : 0;
}

int POSIXFile::close()
{
   auto res { !h || ::close( h ) ? errno : 0 };
   h = 0;
   return res;
}

int POSIXFile::seek( int64_t p )
{
   return h && ::lseek( h, p, SEEK_SET ) == p ? 0 : 1;
}

POSIXFile::~POSIXFile()
{
   if( h ) close();
}
#endif

}// namespace gdx::file
