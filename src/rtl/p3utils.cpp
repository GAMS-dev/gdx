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

#include "p3utils.h"

#include <algorithm>               // for max, min
#include <array>                   // for array
#include <cassert>                 // for assert
#include <cmath>                   // for abs
#include <cstdlib>                 // for getenv, abs, realpath, setenv, uns...
#include <cstring>                 // for memset, strcpy, strlen, memmove
#include <filesystem>              // for path
#include <string>                  // for basic_string, operator+, string

#include "../gdlib/strutilx.h"     // for DblToStr
#include "../gdlib/utils.h"        // for in, trim, val
#include "global/modhead.h"        // for STUBWARN
#include "math_p3.h"               // for IntPower
#include "p3platform.h"            // for tOSPlatform, OSPlatform, OSFileType
#include "sysutils_p3.h"           // for ExtractFilePath, ExcludeTrailingPa...

#if defined(_WIN32)
   // Windows
   #ifndef __GNUC__
      #pragma comment( lib, "iphlpapi.lib" )
      #pragma comment( lib, "Ws2_32.lib" )
      //#define _WINSOCK2API_
      #define _WINSOCKAPI_ /* Prevent inclusion of winsock.h in windows.h */
      #include <winsock2.h>
   #endif
   #include <Windows.h>
   #include <io.h>
   #include <psapi.h> /* enough if we run on Windows 7 or later */
   #include <iphlpapi.h>
   #include <shlobj.h>
   #include <IPTypes.h>
#else
   // Unix
   #include <sys/socket.h>
   #include <sys/fcntl.h>
   #include <sys/utsname.h>
   #include <sys/stat.h>
   #if( defined( __linux__ ) || defined( __APPLE__ ) ) /* at least, maybe for others too */
      #include <net/if.h>
      #include <sys/ioctl.h>
      #if defined( __APPLE__ )
         #include <sys/proc_info.h>
         #include <sys/sysctl.h>
         #include <net/if_dl.h>
         #include <libproc.h>
      #endif
   #endif
   #include <netinet/in.h>
   #include <unistd.h>
   #include <dlfcn.h>
   #include <poll.h>
#endif

using namespace rtl::sysutils_p3;
using namespace rtl::p3platform;
using namespace std::literals::string_literals;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace rtl::p3utils
{

#if defined( _WIN32 )
#else
static bool setEnvironmentVariableUnix( const std::string &name, const std::string &value = ""s )
{
   if( name.empty() ) return false;
   if( value.empty() )
   {
      // delete name from env
      unsetenv( name.c_str() );
      return true;
   }
   return !setenv( name.c_str(), value.c_str(), 1 );
}
#endif

static std::vector<std::string> paramstr;

bool PrefixPath( const std::string &s )
{
   if( s.empty() ) return true;

   const auto prevPath = rtl::sysutils_p3::QueryEnvironmentVariable( "PATH" );
   const std::string newPath = s + rtl::sysutils_p3::PathSep + prevPath;
   return !rtl::sysutils_p3::AssignEnvironmentVariable( "PATH", newPath );
}

bool P3SetEnv( const std::string &name, const std::string &val )
{
#if defined( _WIN32 ) || defined( _WIN64 )
   return SetEnvironmentVariableA( name.c_str(), val.c_str() );
#else
   return setEnvironmentVariableUnix( name, val );
#endif
}

void P3UnSetEnv( const std::string &name )
{
#if defined( _WIN32 ) || defined( _WIN64 )
   SetEnvironmentVariableA( name.c_str(), nullptr );
#else
   setEnvironmentVariableUnix( name );
#endif
}

bool P3IsSetEnv( const std::string &name )
{
   return std::getenv( name.c_str() ) != nullptr;
}

/*static bool P3SetEnvPC( const std::string &name, const char *val )
{
   return P3SetEnv( name, val );
}*/

std::string P3GetEnv( const std::string &name )
{
#if defined( _WIN32 )
   std::array<char, 1024> buf;
   GetEnvironmentVariableA( name.c_str(), buf.data(), static_cast<DWORD>( buf.size() ) );
   return buf.data();
#else
   const char *p = getenv( name.c_str() );
   return !p ? ""s : p;
#endif
}

// semantics identical to WinAPI GetEnvironmentVariable
uint32_t P3GetEnvPC( const std::string &name, char *buf, uint32_t bufSize )
{
#if defined( _WIN32 )
   return GetEnvironmentVariableA( name.c_str(), buf, bufSize );
#else
   const char *p = getenv( name.c_str() );
   if( !p ) return 0;// no match in the env
   const auto psiz = strlen( p ) + 1;
   if( psiz <= bufSize )
   {
      // it fits: copy it over
      memmove( buf, p, psiz );
      return psiz - 1;
   }
   return psiz;
#endif
}

bool p3GetMemoryInfo( uint64_t &rss, uint64_t &vss )
{
#if defined( _WIN32 )
   PROCESS_MEMORY_COUNTERS info;
   const int ok = GetProcessMemoryInfo( GetCurrentProcess(), &info, sizeof( info ) );
   if( !ok )
      return false; /* failure */
   rss = static_cast<int64_t>( info.WorkingSetSize );
   vss = static_cast<int64_t>( info.PagefileUsage );
   return true; /* success */
#elif defined( __linux )
   size_t sz;
   FILE *fp = std::fopen( "/proc/self/statm", "r" );
   if( !fp )
      return false; /* failure */
   /* first two are VmSize, VmRSS */
   unsigned long urss, uvss;
   const int n = fscanf( fp, "%lu %lu", &uvss, &urss );
   std::fclose( fp );
   if( 2 != n )
      return false; /* failure */
   sz = sysconf( _SC_PAGESIZE );
   rss = sz * urss;
   vss = sz * uvss;
   return true; /* success */
#elif defined( __APPLE__ )
   int ret;
   struct proc_taskinfo procTaskInfo;
   ret = proc_pidinfo( (int) getpid(), PROC_PIDTASKINFO, 0,
                       (void *) &procTaskInfo, sizeof( procTaskInfo ) );
   if( ret < (int) sizeof( procTaskInfo ) )
      return false; /* failure */
   rss = (int64_t) procTaskInfo.pti_resident_size;
   vss = (int64_t) procTaskInfo.pti_virtual_size;
   return true; /* success */
#else
   throw std::runtime_error( "Unknown platform for getMemoryInfo!" );
   return false; /* fail */
#endif
}

void p3SetConsoleTitle( const std::string &s )
{
#if defined( _WIN32 )
   SetConsoleTitleA( s.c_str() );
#else
   // do nothing for now on Unix-like platforms
#endif
}

void p3NoPopups()
{
#if defined( _WIN32 )
   SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX );
#else
   // do nothing for now on Unix-like platforms
#endif
}

std::string p3GetUserName()
{
#if defined( _WIN32 )
   auto res { "unknown"s };
   DWORD n {256};
   std::array<char, 256> userName {};
   if( GetUserNameA( userName.data(), &n ) )
      res.assign( userName.data() );
   return res;
#elif defined( __APPLE__ )
   char loginName[256];
   int rc = getlogin_r( loginName, sizeof( char ) * 256 );
   return !rc ? loginName : ""s;
#else
   char loginName[256];
   char *p = cuserid( loginName );
   return p ? p : ""s;
#endif
}

std::string p3GetComputerName()
{
#ifdef _WIN32
   std::string res { "unknown" };
   std::array<char, 256> computerName {};
   DWORD n = 256;
   if( GetComputerNameA( computerName.data(), &n ) )
      res.assign( computerName.data() );
   return res;
#else
   utsname uts {};
   const int rc = uname( &uts );
   return rc >= 0 ? uts.nodename : ""s;
#endif
}

#if defined(_WIN32)
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
   }
   return result;
}

static constexpr std::array<DWORD, 3> accessMode {
        GENERIC_READ,
        GENERIC_WRITE,
        GENERIC_READ | GENERIC_WRITE };
// this works for GDX so we do it: it is kind of silly to use the
// shareMode var then but why not?
static constexpr std::array<DWORD, 3> shareMode {
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE };
static constexpr std::array<DWORD, 3> createHow {
        OPEN_EXISTING,
        CREATE_ALWAYS,
        OPEN_ALWAYS };

typedef BOOL( WINAPI *GetFileSizeEx_t )( HANDLE h, PLARGE_INTEGER fileSize );
GetFileSizeEx_t pGetFileSizeEx {};
int triedGetFileSizeEx {};

typedef BOOL( WINAPI *SetFilePointerEx_t )( HANDLE h, LARGE_INTEGER distance, PLARGE_INTEGER newPointer, DWORD whence );
SetFilePointerEx_t pSetFilePointerEx {};
int triedSetFilePointerEx {};

#endif

static inline bool p3IsValidHandle(const Tp3FileHandle h) {
#if defined( _WIN32 )
   return h && INVALID_HANDLE_VALUE != h;
#else
   return h > 0;
#endif
}

int p3FileOpen( const std::string &fName, Tp3FileOpenAction mode, Tp3FileHandle &h )
{
#if defined(_WIN32)
   DWORD lowMode;
   HANDLE hFile;

   lowMode = mode & 3;
   if (3 == lowMode) {
      h = INVALID_HANDLE_VALUE;
      return ERROR_INVALID_PARAMETER;
   }

   if (fName.empty()) {
      if (p3OpenRead == mode)
         hFile = GetStdHandle(STD_INPUT_HANDLE);
      else if (p3OpenWrite == mode)
         hFile = GetStdHandle(STD_OUTPUT_HANDLE);
      else {
         h = INVALID_HANDLE_VALUE;
         return ERROR_INVALID_PARAMETER;
      }
   }
   else
      hFile = CreateFileA (fName.c_str(), accessMode[lowMode], shareMode[lowMode], nullptr,
                          createHow[lowMode], FILE_ATTRIBUTE_NORMAL, nullptr);
   if (INVALID_HANDLE_VALUE == hFile) {
      h = INVALID_HANDLE_VALUE;
      int result = win2c(static_cast<int>(GetLastError()));
      if (0 == result) { /* ouch: just pick a likely but non-specific code */
         result = EACCES;
      }
      return result;
   }
   else {
      h = hFile;
      return 0;
   }
#else
   if (fName.empty()) {
      if (mode == Tp3FileOpenAction::p3OpenRead)
         h = STDIN_FILENO;
      else if (mode == Tp3FileOpenAction::p3OpenWrite)
         h = STDOUT_FILENO;
      else {
         h = 0;
         return -1;
      }
      return 0;
   }
   int flags = mode & 3;
   // write-only or read-write
   if (flags > 0)
      flags |= O_CREAT;
   if (flags & 1)
      flags |= O_TRUNC;
   int fd = open(fName.c_str(), flags, 0666);
   if (-1 == fd) {
      h = 0;
      return errno;
   }
   int result = 0;
   // before calling this a success, check for directory on read-only
   struct stat statBuf {};
   if (p3OpenRead == mode) {
      int rc = fstat (fd, &statBuf);
      if (rc)
         result = errno;
      else if (S_ISDIR(statBuf.st_mode))
         result = EISDIR;
   }
   if (result) {
      close(fd);
      return result;
   }
   h = fd;
   return result;
#endif
}

int p3FileClose( Tp3FileHandle &h )
{
   if( !p3IsValidHandle( h ) ) return EBADF;
   int res{};
#if defined(_WIN32)
   if( !CloseHandle( h ) )
   {
      res = win2c( static_cast<int>(GetLastError()) );
      // ouch: just pick a likely but non-specific code
      if( !res )
         res = EIO;
   }
   h = INVALID_HANDLE_VALUE;
#else
   if( close( h ) )
      res = errno;
   h = 0;
#endif
   return res;
}

int p3FileGetSize( Tp3FileHandle fs, int64_t &fileSize )
{
   if( !p3IsValidHandle( fs ) ) return EBADF;
   int res {};
#if defined(_WIN32)
   bool frc;
   if (!triedGetFileSizeEx) {
      auto k32 { GetModuleHandleA( "kernel32" ) };
      pGetFileSizeEx = !k32 ? nullptr : (GetFileSizeEx_t) GetProcAddress(k32,"GetFileSizeEx");
      triedGetFileSizeEx = 1;
   }
   if (pGetFileSizeEx)
      frc = pGetFileSizeEx(fs, (PLARGE_INTEGER)&fileSize);
   else {
      DWORD tt;
      frc = GetFileSize(fs, &tt);
      fileSize = tt;
   }
   if(!frc) {
      res = win2c(static_cast<int>(GetLastError()));
      // ouch: just pick a likely but non-specific code
      if (!res)
         res = EACCES;
   }
#else
   struct stat statBuf {};
   if (fstat (fs, &statBuf))
      res = errno;
   else
      fileSize = statBuf.st_size;
#endif
   return res;
}

int p3FileRead( Tp3FileHandle h, char *buffer, uint32_t buflen, uint32_t &numRead )
{
   int res {};
#if defined(_WIN32)
   if( !ReadFile( h, buffer, buflen, (LPDWORD) &numRead, nullptr ) )
   {
      res = win2c( static_cast<int>(GetLastError()) );
      if( !res ) // ouch: just pick a likely but non-specific code
         res = EIO;
   }
#else
   auto rc = read( h, buffer, buflen );
   if( rc < 0 )
   {
      res = errno;
      numRead = 0;
   }
   else numRead = rc;
#endif
   return res;
}

int p3FileWrite( Tp3FileHandle h, const char *buffer, uint32_t buflen, uint32_t &numWritten )
{
   int res{};
#if defined( _WIN32 )
   if( !WriteFile( h, buffer, buflen, (LPDWORD) &numWritten, nullptr ) )
   {
      res = win2c( static_cast<int>(GetLastError()) );
      // ouch: just pick a likely but non-specific code
      if( !res )
         res = EIO;
   }
#else
   auto rc = write(h, buffer, buflen);
   if (rc < 0) {
      res = errno;
      numWritten = 0;
   }
   else
      numWritten = rc;
#endif
   return res;
}

int p3FileSetPointer(Tp3FileHandle h, int64_t distance, int64_t &newPointer, uint32_t whence)
{
   if( !p3IsValidHandle( h ) ) return EBADF;
   int res {};
#if defined(_WIN32)
   if (!triedSetFilePointerEx) {
      const auto k32 { GetModuleHandleA( "kernel32" ) };
      pSetFilePointerEx = !k32 ? nullptr : (SetFilePointerEx_t) GetProcAddress(k32,"SetFilePointerEx");
      triedSetFilePointerEx = 1;
   }
   // declared as a union - compiler rejects a cast
   LARGE_INTEGER d;
   d.QuadPart = distance;
   bool frc;
   if (pSetFilePointerEx)
      frc = pSetFilePointerEx(h, d, (PLARGE_INTEGER) &newPointer, whence);
   else {
      DWORD trc = SetFilePointer(h, static_cast<LONG>(distance), nullptr, whence);
      if( trc == INVALID_SET_FILE_POINTER )
      {
         frc = false;
         newPointer = 0;
      }
      else {
         newPointer = trc;
         frc = true;
      }
   }
   if(!frc) {
      res = win2c( static_cast<int>(GetLastError()) );
      if (!res)
         res = EINVAL;
   }
#else
   int w;
   switch (whence) {
      case p3_FILE_BEGIN:
         w = SEEK_SET;
         break;
      case p3_FILE_CURRENT:
         w = SEEK_CUR;
         break;
      case p3_FILE_END:
         w = SEEK_END;
         break;
      default:
         return EINVAL;
   }
   // check if conversion to off_t loses info
   auto offset = (off_t)distance;
   if (offset != distance) // only can happen on 32 bit machines?
      return EOVERFLOW;
   off_t newPos = lseek (h, offset, w);
   if ((off_t)-1 == newPos)
      return errno;
   newPointer = newPos;
#endif
   return res;
}

int p3FileGetPointer(Tp3FileHandle h, int64_t &filePointer)
{
   if( !p3IsValidHandle( h ) ) return EBADF;
#if defined(_WIN32)
   if( !triedSetFilePointerEx )
   {
      const auto k32 {GetModuleHandleA( "kernel32" )};
      pSetFilePointerEx = !k32 ? nullptr : (SetFilePointerEx_t) GetProcAddress( k32, "SetFilePointerEx" );
      triedSetFilePointerEx = 1;
   }
   // declared as a union - compiler rejects a cast
   LARGE_INTEGER d;
   d.QuadPart = 0;
   bool frc;
   if( pSetFilePointerEx )
      frc = pSetFilePointerEx( h, d, (PLARGE_INTEGER) &filePointer, p3_FILE_CURRENT );
   else
   {
      DWORD trc = SetFilePointer( h, 0, nullptr, p3_FILE_CURRENT );
      if( INVALID_SET_FILE_POINTER == trc )
      {
         frc = false;
         filePointer = 0;
      }
      else
      {
         filePointer = trc;
         frc = true;
      }
   }

   if(!frc)
   {
      int res = win2c( static_cast<int>(GetLastError()) );
      if( !res )
         res = EINVAL;
      return res;
   }
#else
   off_t newPos = lseek( h, 0, SEEK_CUR );
   if( (off_t) -1 == newPos )
      return errno;
   filePointer = newPos;
#endif
   return 0;
}

/*
     * Get a list(of sorts) of directories to search for config / data / doc / etc files
     * by convention, the first element in this list is the writableLocation
     * return true if we follow this convention(i.e.p3WritableLocation succeeds), false otherwise
     *  But even on false, locCountand locNames are valid
     */
bool p3StandardLocations( Tp3Location locType, const std::string &appName, TLocNames &locNames, int &eCount )
{
   eCount = 0;
   locNames = { ""s };// { rtl::sysutils_p3::GetCurrentDir() };
   bool res = p3WritableLocation( locType, appName, locNames.front() );
   if( p3Documents == locType ) return res;

   bool isDataLoc { utils::in( locType, p3Data, p3AppData, p3AppLocalData ) };

   if( OSFileType() == OSFileWIN )
   {
      bool isConfigLoc { utils::in( locType, p3Config, p3AppConfig ) };
      if( isConfigLoc || isDataLoc )
      {
         std::string suffix = appName.empty() ? ""s : PathDelim + appName;
         locNames.emplace_back( "C:\\ProgramData" + suffix );
      }
      if( isDataLoc )
      {
         std::string execName, msg;
         if( p3GetExecName( execName, msg ) )
         {
            eCount++;
            return res;
         }
         std::string execPath { ExtractFilePath( execName ) };
         locNames.emplace_back( ExcludeTrailingPathDelimiter( execPath ) );
         locNames.emplace_back( execPath + "data"s );
         if( !appName.empty() ) locNames.emplace_back( locNames.back() + PathDelim + appName );
      }
   }
   else if( utils::in(OSPlatform(), p3platform::OSDarwin_x64, p3platform::OSDarwin_arm64) )
   {
      if( isDataLoc )
      {
         std::string msg { "/Library/Application Support"s + ( !appName.empty() ? "/" + appName : ""s ) };
         locNames.emplace_back( msg );
         std::string execName;
         if( p3GetExecName( execName, locNames.back() ) )
         {
            eCount++;
            return res;
         }
         std::string execPath { ExcludeTrailingPathDelimiter( ExtractFilePath( execName ) ) };
         if( LastDelimiter( "/", execPath ) >= 2 ) locNames.emplace_back( ExtractFilePath( execPath ) + "Resources"s );
         else
            eCount++;
      }
   }
   else
   {// neither Windows nor Mac, right now this must be Linux
      /*bool  isPlainConfigLoc { p3Config == locType }; */
      bool isAppConfigLoc { p3AppConfig == locType };
      std::array<char, 256> buf {};
      const auto bufLen = P3GetEnvPC( isDataLoc ? "XDG_DATA_DIRS" : "XDG_CONFIG_DIRS"s, buf.data(), static_cast<uint32_t>(buf.size()) );
      if( bufLen >= buf.size() )
      {// too much to handle
         eCount++;
         return res;
      }
      if( bufLen > 0 )
      {// we got something
         std::string msg = ( isAppConfigLoc || isDataLoc ) && !appName.empty() ? "/"s + appName : ""s;
         int dPos {}, k {};
         do {
            while( buf[dPos] != '\0' && buf[dPos] != ':' ) dPos++;
            const int n = dPos - k;
            if( n > 0 )
            {
               if( locNames.size() >= NLocNames ) eCount++;
               else
                  locNames.emplace_back( ""s + buf.data() + msg );
            }
            dPos++;
            k = dPos;
         } while( k <= static_cast<int>( bufLen ) );
      }
      else
      {
         std::string prefix { "/etc/xdg" },
                 suffix { ( ( isDataLoc || isAppConfigLoc ) && !appName.empty() ? "/"s + appName : ""s ) };
         if( isDataLoc )
         {
            prefix = "/usr/local/share";
            locNames.emplace_back( prefix + suffix );
            prefix = "/usr/share";
         }
         locNames.emplace_back( prefix + suffix );
      }
   }
   return res;
}

#ifndef _WIN32
// set s := ${HOME} + dd1 + dd2
// return true success, false on failure (e.g. too long or HOME not set)
static bool homePlus( const std::string &dd1, const std::string &dd2, std::string &s )
{
   std::array<char, 256> buf;
   auto len {P3GetEnvPC("HOME"s, buf.data(), buf.size())};
   if(!len || len >= buf.size()) return false;
   s.reserve(len+dd1.length()+dd2.length());
   s.assign(buf.data());
   s += dd1 + dd2;
   return true;
}
#endif

#if defined(__MINGW32__) || defined(__MINGW64__)
bool p3WritableLocation( Tp3Location locType, const std::string &appName, std::string &locName ) { return false; }
#else
/*
     * Get the name of the directory to write config/data/doc/etc files to
     * return true on success (i.e. we can construct the name), false on failure
     */
bool p3WritableLocation( Tp3Location locType, const std::string &appName, std::string &locName )
{
#ifdef _WIN32
   locName.clear();
   if( utils::in( locType, p3Config, p3AppConfig, p3Data, p3AppLocalData ) )
      locName = QueryEnvironmentVariable( "LOCALAPPDATA" );
   else if( locType == p3AppData )
      locName = QueryEnvironmentVariable( "APPDATA" );
   else if( locType == p3Documents )
   {
      PWCHAR wideBuf;
      if( SHGetKnownFolderPath( FOLDERID_Documents, 0, nullptr, &wideBuf ) == S_OK )
      {
         std::array<char, 256> buf {};
         size_t charCount {};
         wcstombs_s( &charCount, buf.data(), buf.size(), wideBuf, _TRUNCATE );
         locName.assign( buf.data() );
      }
   }
   if( !locName.empty() && !appName.empty() && utils::in( locType, p3Config, p3AppConfig, p3Data, p3AppData, p3AppLocalData ) )
      locName += rtl::sysutils_p3::PathDelim + appName;
   return !locName.empty();
#else
   std::string dd = appName.empty() ? ""s : PathDelim + appName;

   if( utils::in(OSPlatform(), p3platform::OSDarwin_x64, p3platform::OSDarwin_arm64) )
   {
      if( p3Config == locType )
      {
         if( !homePlus( "/Library/Preferences"s, ""s, locName ) )
            return false;
      }
      else if( p3AppConfig == locType )
      {
         if( !homePlus( "/Library/Preferences"s, dd, locName ) )
            return false;
      }
      else if( utils::in( locType, p3Data, p3AppData, p3AppLocalData ) )
      {
         if( !homePlus( "/Library/Application Support"s, dd, locName ) )
            return false;
      }
      else if( p3Documents == locType )
      {
         if( !homePlus( "/Documents"s, ""s, locName ) )
            return false;
      }
      else
         return false;
      return true;
   }
   else
   {
      // everything neither Windows nor macOS: only Linux in July 2022
      if( p3Config == locType || p3AppConfig == locType )
      {
         locName = QueryEnvironmentVariable( "XDG_CONFIG_HOME" );
         if( locName.empty() && !homePlus( "/.config", dd, locName ) ) return false;
      }
      else if( utils::in( locType, p3Data, p3AppData, p3AppLocalData ) )
      {
         locName = QueryEnvironmentVariable( "XDG_DATA_HOME" );
         if( locName.empty() && !homePlus( "/.local/share", dd, locName ) ) return false;
      }
      else if( locType == p3Documents )
      {
         if( !homePlus( "/Documents", dd, locName ) ) return false;
      }
      else
         return false;
      return true;
   }
#endif
}
#endif

const std::string zeros {std::string( 54, '0' )};

int p3Chmod( const std::string &path, int mode )
{
#if !defined( _WIN32 )
   return chmod( path.c_str(), mode );
#endif
   return 0;
}

double RealTrunc( double x )
{
   return static_cast<int>( x );
}

double ReadRound( double x )
{
   return static_cast<int>( x + 0.5 * ( x >= 0 ? 1.0 : -1.0 ) );
}

//constexpr int MAXDIGITS = 17;// at most this many decimal digits from a double
/*constexpr std::array<double, MAXDIGITS + 1> tenPow {
        1e00, 1e01, 1e02, 1e03, 1e04, 1e05, 1e06, 1e07, 1e08, 1e09,
        1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17 };*/
//constexpr int MINTWOS = 4;
//constexpr int MAXTWOS = 8;
//const std::array<int, MAXTWOS - MINTWOS + 1> twoPow { 16, 32, 64, 128, 256 };
//constexpr std::array<double, MAXTWOS - MINTWOS + 1> tenTwoPow { 1e16, 1e32, 1e64, 1e128, 1e256 };

bool delphiGetDecDigits( double y, int mode, int nDigits, std::string &digits, int &decPos, int &minusCnt )
{
   // ...
   // TODO: Implement me!
   STUBWARN();
   return false;
}

bool p3GetDecDigits( double y, int mode, int nDigits, std::string &digits, int &decPos, int &minusCnt )
{
   // ...
   // TODO: Implement me!
   STUBWARN();
   return false;
}

std::string p3FloatToEfmt( double x, int width, int decimals )
{
   // ...
   // TODO: Implement me!
   STUBWARN();
   return {};
}

// return digits of input i64, with trailing zeros removed
// i64 is assumed to contain a positive int < 1e17
std::string getDigits( const int64_t i64 )
{
   constexpr int M = 100000000, LOGM = 8;
   auto i = static_cast<int>( i64 );
   if( i == i64 ) return rtl::sysutils_p3::IntToStr( i );
   i = static_cast<int>( i64 % M );
   std::string s = rtl::sysutils_p3::IntToStr( i );
   i = LOGM - static_cast<int>( s.length() );
   if( i > 0 ) s = std::string( i, '0' ) + s;
   i = static_cast<int>( i64 / M );
   std::string res = rtl::sysutils_p3::IntToStr( i ) + s;
   // strip trailing zeros
   for( i = static_cast<int>( res.length() ) - 1; i >= 1 && res.back() == '0'; i-- )
      res.pop_back();
   return res;
}

template<typename T>
T myMin( T a, T b )
{
   return a < b ? a : b;
}

// FIXME: AS: This seems slow.
std::string FloatToE( double y, int decimals )
{
   auto myRoundTo = []( const double x, const int i ) -> double {
      const double zeroFive = 0.5 * ( x > 0.0 ? 1.0 : -1.0 );
      if( !i ) return static_cast<int>( x + zeroFive );
      if( i > 0 )
      {
         // use positive power of 10 to avoid roundoff error in z
         const double z { rtl::math_p3::IntPower( 10, i ) };
         return static_cast<int>( x * z + zeroFive ) * z;
      }
      const double z { rtl::math_p3::IntPower( 10, -i ) };
      return static_cast<int>( x / z + zeroFive ) * z;
   };

   /* FloatToE */

   double x { std::abs( y ) };
   if( x != 0.0 )
   {
      int n {};
      while( x >= 1.0 )
      {
         n++;
         x /= 10.0;
      }
      while( x < 1.0 )
      {
         n--;
         x *= 10.0;
      }
      x = myRoundTo( x, decimals ) * rtl::math_p3::IntPower( 10.0, n );
   }
   std::string s { gdlib::strutilx::DblToStr( x ) };

   // edit and fix sign
   int k = LastDelimiter( "+-", s );
   auto j = s.find( '.' );
   decimals = std::min<int>( k - static_cast<int>(j) - 2, decimals );
   std::string res { "  " };
   if( y < 0 ) res[1] = '-';
   res += s.substr( j - 1, decimals + 2 ) + "E"s + s[k];
   int e, i;
   utils::val( s.substr( k, 5 ), e, i );
   e = std::abs( e );
   return res + s.substr( s.length() - 2, 2 );
}

std::string ParamStrZero()
{
   return paramstr.front();
}

std::string ParamStr( int index )
{
   return index >= 0 && index < static_cast<int>( paramstr.size() ) ? paramstr[index] : ""s;
}

int ParamStrCount()
{
   return static_cast<int>(paramstr.size());
}

std::string loadPathVarName()
{
   switch( OSPlatform() )
   {
      case OSDarwin_x64:
      case OSDarwin_arm64:
         return "DYLD_LIBRARY_PATH"s;
      case OSLinux86_64:
         return "LD_LIBRARY_PATH";
      default:
         return {};
   }
}

/*
     * Prefix the directory provided to the load path for shared objects.
     * On Windows, this routine does nothing, since there is no load path
     * if the argument is empty, we use the application directory as
     * taken from paramStr(0) returns true on success, false on failure
     */
bool PrefixLoadPath( const std::string &dir )
{
#if defined( _WIN32 )
   return false;
#else
   std::string s { dir.empty() ? ExcludeTrailingPathDelimiter( ( ExtractFilePath( ParamStrZero() ) ) ) : dir },
           ldPath = loadPathVarName();
   if( ldPath.empty() ) return true;
   const char *tptr { getenv( ldPath.c_str() ) };
   return setEnvironmentVariableUnix( ldPath, s + ( tptr ? ""s + rtl::sysutils_p3::PathSep + tptr : ""s ) );
#endif
}

/**
     * PrefixEnv: prefix the directory name dir to the environment variable evName,
     * if dir is not already the first directory in the list
     * returns true on success (prefixing worked or already there), false on failure
     */
bool PrefixEnv( const std::string &dir, const std::string &evName )
{
   std::string trimDir { utils::trim( dir ) };
   if( trimDir.empty() ) return true;
#if defined( _WIN32 )
   auto evSiz = GetEnvironmentVariableA( evName.c_str(), nullptr, 0 );
   if( !evSiz )// not set? just set it!
      return SetEnvironmentVariableA( evName.c_str(), trimDir.c_str() );
   std::vector<char> pPtr( evSiz );
   auto evLen = GetEnvironmentVariableA( evName.c_str(), pPtr.data(), evSiz );
   assert( evLen == evSiz - 1 && "PrefixEnv: strange GetEnvironmentVariable" );
   char *opPtr = pPtr.data();
   while( opPtr[0] != '\0' )
   {
      if( opPtr[0] != ' ' ) break;
      opPtr++;
      evLen--;
   }
   // if empty, just set it
   if( '\0' == opPtr[0] ) return SetEnvironmentVariableA( evName.c_str(), trimDir.c_str() );
   // check if dir is the first element
   if( ( evLen == trimDir.length() && std::string( opPtr ) == trimDir ) || ( evLen > trimDir.length() && PathSep == opPtr[trimDir.length()] && std::string( opPtr ).substr( 0, trimDir.length() ) == trimDir ) )
      return true;// evName already starts with dir
   std::string tPtr { trimDir + PathSep + opPtr };
   return SetEnvironmentVariableA( evName.c_str(), tPtr.c_str() );
#else
   std::string tptr = getenv( evName.c_str() );
   if( tptr.empty() ) return setEnvironmentVariableUnix( evName, dir );
   if( tptr.length() >= trimDir.length() && dir == tptr &&
       ( tptr.length() == trimDir.length() || tptr[trimDir.length()] == PathSep ) ) return true;
   return setEnvironmentVariableUnix( evName, dir + PathSep + tptr );
#endif
}

void initParamStr( const int argc, const char **argv )
{
   paramstr.resize( argc );
   for( int i {}; i < argc; i++ )
   {
      paramstr[i] = argv[i];
      if( !i ) // absolute executable path
      {
#if defined(_WIN32)
         std::array<char, 261> buf {}; // length taken from Delphi's System.pas unit
         const auto slen { GetModuleFileNameA( nullptr, buf.data(), 256 ) };
         paramstr.front() = std::string { buf.data(), slen};
#else
         if( std::string buf, msg; !xGetExecName( buf, msg ) )
            paramstr.front() = buf;
#endif
      }
   }
}

#ifndef _WIN32
/* local use only: be sure to call with enough space for the snprintf */
static void myStrError( int n, char *buf, const size_t bufSiz )
{
#if defined( _WIN32 )
   if( strerror_s( buf, bufSiz, n ) )
      (void) std::snprintf( buf, bufSiz, "errno = %d", n );
#else
   if( strerror_r( n, buf, bufSiz ) )
      (void) std::snprintf( buf, bufSiz, "errno = %d", n );
#endif
}

int xGetExecName( std::string &execName, std::string &msg )
{
   int rc { 8 };
   std::array<char, 4096> execBuf {};
#if defined( __APPLE__ )
   std::array<char, 2048> tmpBuf {};
   auto pid = getpid();
   int k = proc_pidpath( pid, execBuf.data(), sizeof( char ) * execBuf.size() );
   execName.assign( execBuf.data() );
   if( k <= 0 )
   {
      myStrError( errno, tmpBuf.data(), sizeof( char ) * tmpBuf.size() );
      msg = "proc_pidpath(pid="s + rtl::sysutils_p3::IntToStr( pid ) + ") failed: "s + std::string( tmpBuf.begin(), tmpBuf.end() );
      execName.clear();
      rc = 4;
   }
   else
      rc = 0;
#elif defined( __linux )
   std::array<char, 2048> tmpBuf {};
   auto ssz = readlink( "/proc/self/exe", execBuf.data(), sizeof( char ) * execBuf.size() );
   execName.assign( execBuf.data() );
   if( ssz < 0 )
   {
      myStrError( errno, tmpBuf.data(), tmpBuf.size() * sizeof( char ) );
      msg = "readlink(/proc/self/exe,...) failure: "s + std::string( tmpBuf.begin(), tmpBuf.end() );
      execName.clear();
      rc = 4;
   }
   else
   {
      ssz = std::min<int>( execBuf.size() - 1, ssz );
      rc = 0;
   }
#elif defined( _WIN32 )
   if( const auto k = GetModuleFileNameA( nullptr, execBuf.data(), static_cast<DWORD>(sizeof( char ) * execBuf.size()) ); !k )
   {
      msg = "GetModuleFileName() failure: rc="s + rtl::sysutils_p3::IntToStr( k );
      execName.clear();
      rc = 4;
   }
   else
   {
      rc = 0;
      execName.assign( execBuf.data() );
   }
#else
   execName.clear();
   msg = "not implemented for this platform"s;
#endif
   return !rc && execName.length() > 255 ? 1 : rc;
}
#endif

// return:0 on success, 1 if truncated result, 2 if not lib, >2 o/w
int p3GetExecName( std::string &execName, std::string &msg )
{
   execName.clear();
#if defined( _WIN32 )
   std::array<char, 256> buf {};
   auto rc = GetModuleFileNameA( nullptr, buf.data(), (int) buf.size() );
   if( !rc )
   {
      msg = "GetModuleFileNameA call failed";
      return 3;
   }
   else if( rc >= 256 )
   {
      buf.back() = '\0';
      execName.assign( buf.data() );
      msg = "result truncated to 255 chars";
      return 1;
   }
   else
   {
      execName.assign( buf.data() );
      msg.clear();
      return 0;
   }
#else
   msg = "P3: not yet implemented";
   return xGetExecName( execName, msg );
#endif
}

// FIXME: Do not always return false!
static bool isLibrary()
{
   return false;
}

static int xGetLibName( std::string &libName, std::string &msg )
{
   char libBuf[4096];
   msg.clear();
   int rc;

#if defined( __linux ) || defined( __APPLE__ )
   {
      char tmpBuf[2048];
      static_assert( sizeof( tmpBuf ) == 2048 );
      Dl_info dlInfo;
      const int k = dladdr( reinterpret_cast<void *>( &xGetLibName ), &dlInfo );
      if( k > 0 )
      {
         strncpy( tmpBuf, dlInfo.dli_fname, sizeof( tmpBuf ) - 1 );
         tmpBuf[sizeof( tmpBuf ) - 1] = '\0';
         if( realpath( tmpBuf, libBuf ) )
            rc = 0;
         else
         {
            myStrError( errno, tmpBuf, sizeof( tmpBuf ) );
            msg = "realpath() failure: "s + tmpBuf;
            *libBuf = '\0';
            rc = 5;
         }
      }
      else
      {
         msg = "dladdr() failure"s;
         *libBuf = '\0';
         rc = 4;
      }
   }
#elif defined( _WIN32 )
   {
      HMODULE h;
      int k = GetModuleHandleEx( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                     GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                             reinterpret_cast<LPCTSTR>( &xGetLibName ), &h );
      if( k )
      { /* OK: got a handle */
         k = static_cast<int>( GetModuleFileNameA( h, libBuf, sizeof( libBuf ) ) );
         if( 0 == k )
         {
            msg = "GetModuleFileName() failure: rc="s + rtl::sysutils_p3::IntToStr(k);
            *libBuf = '\0';
            rc = 5;
         }
         else
            rc = 0;
      }
      else
      {
         msg = "GetModuleHandleEx() failure: rc="s + rtl::sysutils_p3::IntToStr(k);
         *libBuf = '\0';
         rc = 4;
      }
   }
#else
   *libBuf = '\0';
   msg = "not implemented for this platform"s;
   rc = 8;
#endif
   libName.assign( libBuf );
   return 0 == rc && strlen( libBuf ) > 255 ? 1 : rc;
} /* xGetLibName */

int p3GetLibName( std::string &libName, std::string &msg )
{
   return xGetLibName( libName, msg );
#if defined( _WIN32 )
   libName.clear();
   if( !isLibrary() )
   {
      msg = "Not called from a library"s;
      return 2;
   }
   std::array<char, 256> buf;
   HMODULE hinstance;
   auto rc { GetModuleFileNameA( hinstance, buf.data(), static_cast<DWORD>(buf.size()) ) };
   if( !rc )
   {
      msg = "GetModuleFileNameA call failed"s;
      return 3;
   }
   else if( rc >= 256 )
   {
      libName.assign( buf.data() );
      msg = "Result truncated to 255 characters"s;
      return 1;
   }
   else
   {
      libName.assign( buf.data() );
      msg.clear();
      return 0;
   }
#else
   if( !isLibrary() )
   {
      libName.clear();
      msg = "Not called from a library"s;
      return 2;
   }
   libName.clear();
   msg = "P3: not yet implemented"s;
   return 9;
#endif
}

#ifdef __IN_CPPMEX__
bool p3GetFirstMACAddress( std::string &mac )
{
#if defined( __linux__ )
   {
      std::array<char, 1024> buffer {};
      int success {};

      const int sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_IP );
      if( sock == -1 )
         return false;

      ifconf ifc {};
      ifc.ifc_len = buffer.size();
      ifc.ifc_buf = buffer.data();
      if( ioctl( sock, SIOCGIFCONF, &ifc ) == -1 )
         return false;

      ifreq ifr {};

      {
         struct ifreq *it = ifc.ifc_req;
         const struct ifreq *const end = it + ( ifc.ifc_len / sizeof( struct ifreq ) );

         for( ; it != end; ++it )
         {
            strcpy( ifr.ifr_name, it->ifr_name );
            if( ioctl( sock, SIOCGIFFLAGS, &ifr ) == 0 )
            {
               if( ifr.ifr_flags & IFF_LOOPBACK ) /* don't count loopback */
                  continue;
               if( ioctl( sock, SIOCGIFHWADDR, &ifr ) == 0 )
               {
                  success = 1;
                  break;
               }
            }
         } /* loop over interfaces */
      }

      if( success )
      {
         unsigned char mb[6];
         memcpy( mb, ifr.ifr_hwaddr.sa_data, 6 );
         constexpr int bufSiz{18};
         std::array<char, bufSiz> buf {};
         std::snprintf( buf.data(), sizeof(char)*bufSiz, "%02x:%02x:%02x:%02x:%02x:%02x", mb[0], mb[1], mb[2], mb[3], mb[4], mb[5] );
         mac.assign( buf.data() );
         return true;
      }

      return false;
   } /* if __linux__ */
#elif defined( __APPLE__ )
   {
      char prevName[IF_NAMESIZE];
      int mib[6];
      int sock;
      int halfDone = 0; /* true if we have a MAC number for an interface that is down */
      struct ifconf ifc {
      };
      char buf[1024];
      char buf2[1024];
      unsigned char *mp;
      struct ifreq ifr {
      };
      struct ifreq *it, *end;
      size_t recLen, sz;
      struct if_msghdr *ifm;
      struct sockaddr_dl *sdl;

      sock = socket( PF_INET, SOCK_DGRAM, IPPROTO_IP );
      if( sock < 0 ) return false;
      ifc.ifc_len = sizeof( buf );
      ifc.ifc_buf = buf;
      if( ioctl( sock, SIOCGIFCONF, &ifc ) ) return false;
      it = ifc.ifc_req;
      end = (struct ifreq *) ( (unsigned char *) ifc.ifc_buf + ifc.ifc_len );

      mib[0] = CTL_NET;
      mib[1] = AF_ROUTE;
      mib[2] = 0;
      mib[3] = AF_LINK;
      mib[3] = 0;
      mib[4] = NET_RT_IFLIST;
      prevName[0] = '\0';
      for( ; it < end; it = (struct ifreq *) ( (unsigned char *) it + recLen ) )
      {
         recLen = _SIZEOF_ADDR_IFREQ( *it );
         if( 0 == strcmp( it->ifr_name, prevName ) ) /* just checked it already */
            continue;
         (void) strcpy( prevName, it->ifr_name );
         (void) strcpy( ifr.ifr_name, it->ifr_name );
         if( ioctl( sock, SIOCGIFFLAGS, &ifr ) ) /* we should always get flags but if not skip ahead */
            continue;
         if( ifr.ifr_flags & IFF_LOOPBACK ) /* always skip loopback interfaces */
            continue;
         if( halfDone && ( 0 == ( ifr.ifr_flags & IFF_UP ) ) )
            continue; /* we already have a MAC address for a down interface */
         mib[5] = (int) if_nametoindex( it->ifr_name );
         if( 0 == mib[5] )
            continue; /* no valid index found */
         sz = sizeof( buf2 );
         if( sysctl( mib, 6, buf2, &sz, nullptr, 0 ) )
            continue; /* sysctl call failed */
         ifm = (struct if_msghdr *) buf2;
         /* printf ("msglen 0 = %d\n", ifm->ifm_msglen); */
         sdl = (struct sockaddr_dl *) ( ifm + 1 );
         if( RTM_IFINFO != ifm->ifm_type )
            continue; /* WTF */
         mp = (unsigned char *) LLADDR( sdl );
         constexpr int macBufSiz { 18 };
         char macBuf[macBufSiz];
         std::snprintf( (char *) macBuf, macBufSiz, "%02x:%02x:%02x:%02x:%02x:%02x",
                        mp[0], mp[1], mp[2], mp[3], mp[4], mp[5] );
         mac.assign( macBuf );
         if( 0 != ( ifr.ifr_flags & IFF_UP ) )
            return true;
         else
            halfDone = 1;
      } /* loop over interfaces */
      return false;
   } /* if __APPLE__ */
#elif defined( _WIN32 )
   ULONG bufSiz{4096}/*, prevBufSiz{4096}*/;
   int nTries = 0;
   constexpr int maxTries = 3;
   PIP_ADAPTER_ADDRESSES addrBuf {};
   DWORD dwrc;
   bool halfDone {}; // if we have a MAC number for an interface that is down
   do {
      constexpr ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
      addrBuf = static_cast<IP_ADAPTER_ADDRESSES *>( std::malloc( bufSiz ) );
      if( !addrBuf )
         return false;
      dwrc = GetAdaptersAddresses( AF_INET, flags, nullptr, addrBuf, &bufSiz );
      if( ERROR_BUFFER_OVERFLOW == dwrc )
      {
         //prevBufSiz = bufSiz;
         std::free( addrBuf );
         addrBuf = nullptr;
      }
      nTries++;
   } while( ERROR_BUFFER_OVERFLOW == dwrc && nTries < maxTries );
   if( NO_ERROR != dwrc )
   {
      if( addrBuf )
         std::free( addrBuf );
      return false;
   }
   for( PIP_ADAPTER_ADDRESSES currAddr = addrBuf; currAddr; currAddr = currAddr->Next )
   {
      DWORD ifType = currAddr->IfType;
      if( IF_TYPE_ETHERNET_CSMACD != ifType && IF_TYPE_IEEE80211 != ifType )
         continue;
      if( halfDone && IfOperStatusUp != currAddr->OperStatus )
         continue; /* we already have a MAC address for a down interface */
      if( 6 != currAddr->PhysicalAddressLength )
         continue;
      auto *mp = static_cast<unsigned char *>( currAddr->PhysicalAddress );
      std::array<char, 18> macBuf {};
      _snprintf( static_cast<char *>( macBuf.data() ), sizeof(char)*macBuf.size(), "%02x:%02x:%02x:%02x:%02x:%02x", mp[0], mp[1], mp[2], mp[3], mp[4], mp[5] );
      mac.assign( macBuf.data() );
      if( IfOperStatusUp == currAddr->OperStatus )
      {
         std::free( addrBuf );
         return true;
      }
      halfDone = true;
      return false;
   }
   std::free( addrBuf );
   return false;
   /* if _WIN32 */
#else
   return false;
#endif
}

static void mkInvalidSock(T_P3SOCKET &s)
{
#if defined(_WIN32)
   s.wsocket = INVALID_SOCKET;
#else
   s.socketfd = -1;
#endif
}

T_P3SOCKET p3SockGetInvalid()
{
   T_P3SOCKET res {};
   mkInvalidSock( res );
   return res;
}

bool p3SockIsInvalid( T_P3SOCKET s )
{
#if defined(_WIN32)
   return s.wsocket == INVALID_SOCKET;
#else
   return s.socketfd == -1;
#endif
}

int p3SockClose( T_P3SOCKET &s )
{
   int res;
#if defined( _WIN32 )
   res = closesocket( s.wsocket ) ? -1 : 0;
#else
   res = close( s.socketfd );
#endif
   mkInvalidSock( s );
   return res;
}

T_P3SOCKET p3SockCreateConnectedClient( int port )
{
   T_P3SOCKET res {};
   mkInvalidSock( res );
#if defined( _WIN32 )
   const SOCKET s { socket( AF_INET, SOCK_STREAM, 0 ) };
   if( s == INVALID_SOCKET ) return res;
   SOCKADDR_IN addr {};
   std::memset( &addr, 0, sizeof( addr ) );
   addr.sin_family = AF_INET;
   addr.sin_port = htons( port );
   addr.sin_addr.s_addr = inet_addr( "127.0.0.1" );
   if( connect( s, reinterpret_cast<SOCKADDR *>( &addr ), sizeof( addr ) ) == SOCKET_ERROR )
      return res;
   res.wsocket = s;
   return res;
#else
   sockaddr_in servaddr {};
   const int sockfd { socket( AF_INET, SOCK_STREAM, 0 ) };
   if( -1 == sockfd ) return res;
   (void) std::memset( &servaddr, 0, sizeof( servaddr ) );
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
   servaddr.sin_port = htons( port );
   if( connect( sockfd, reinterpret_cast<sockaddr *>( &servaddr ), sizeof( servaddr ) ) )
      return res;
   res.socketfd = sockfd;
   return res;
#endif
}

bool p3SockSendEx(T_P3SOCKET s, const char *buf, int count, int &res, bool pollFirst, int timeOut);

bool p3SockSend( const T_P3SOCKET s, const char *buf, int count, int &res )
{
   return p3SockSendEx( s, buf, count, res, false, 0 );
}

bool p3SockSendTimeout( T_P3SOCKET s, const char *buf, const int count, int &res, const int timeOut )
{
   return p3SockSendEx( s, buf, count, res, true, timeOut );
}

bool p3SockSendEx(T_P3SOCKET s, const char *buf, int count, int &res, bool pollFirst, const int timeOut)
{
   // do not call it this way!!
   if(count <= 0) return true;

   int timeOutLocal = timeOut > 0 ? timeOut : 0;    // no wait / nonblocking

#if defined( _WIN32 )
   if (pollFirst) {
      pollfd fds[1];
      fds->fd = (SOCKET) s.wsocket;
      fds->events = POLLOUT;
      int rc = WSAPoll(fds, 1, timeOutLocal);
      if (rc < 0) {
         res = WSAGetLastError();
         return false;
      }
      if (0 == rc) { // indicate that we hit the timeout with no way to write
         res = -1;
         return true;
      }
      if (! (fds->revents & POLLOUT))
         return false;
   }
   int rc = send(s.wsocket, buf, count, 0);
   if (SOCKET_ERROR == rc) { // should never happen
      res = WSAGetLastError();
      if (WSAEWOULDBLOCK == res) {
         res = -1; // no space or availability to write
         return true;
      }
      return false;
   }
   res = rc;
   return true;
#else
   if (pollFirst) {
      pollfd fds[1];
      fds->fd = s.socketfd;
      fds->events = POLLOUT;
      int rc = poll(fds, 1, timeOutLocal);
      if (rc < 0) {
         res = errno;
         return false;
      }
      if (0 == rc) {  // indicate that we hit the timeout with no way to write
         res = -1;
         return true;
      }
      if (! (fds->revents & POLLOUT))
         return false;
   }
   const ssize_t ssz = send (s.socketfd, reinterpret_cast<const void *>(buf), count, 0);
   if (ssz < 0) { // should never happen
      if ((EAGAIN == errno) || (EWOULDBLOCK == errno)) {
         res = -1; // no space or availability to write
         return true;
      }
      res = errno;
      return false;
   }
   res = static_cast<int>( ssz );
   return true;
#endif
}

int p3SockStartUp()
{
#if defined(_WIN32)
   WSADATA wsa;
   return WSAStartup(MAKEWORD(2, 0), &wsa);
#else
   return 0;
#endif
}

void p3SockCleanUp()
{
#if defined( _WIN32 )
   WSACleanup();
#endif
}

static bool p3SockRecvEx( const T_P3SOCKET s, char *buf, int count, int &res, bool pollFirst, const int timeOut )
{
   if( count <= 0 )// do not call it this way!!
      return true;

   int timeOutLocal { std::max<int>( 0, timeOut ) };// no wait / nonblocking, milliseconds
#if defined( _WIN32 )
   if( pollFirst )
   {
      pollfd fds[1];
      fds->fd = static_cast<SOCKET>( s.wsocket );
      fds->events = POLLIN;
      int rc = WSAPoll( fds, 1, timeOutLocal );
      if( rc < 0 )
      {
         res = WSAGetLastError();
         return false;
      }
      if( 0 == rc )
      {// indicate that we hit the timeout with nothing to receive
         res = -1;
         return true;
      }
      if( !( fds->revents & POLLIN ) )
         return false;
   }
   int rc = recv( (SOCKET) s.wsocket, (char *) buf, count, 0 );
   if( SOCKET_ERROR == rc )
   {// should never happen
      res = WSAGetLastError();
      if( WSAEWOULDBLOCK == res )
      {
         res = -1;// nothing is available
         return true;
      }
      return false;
   }
   res = rc;
#else
   if( pollFirst )
   {
      pollfd fds[1];
      fds->fd = s.socketfd;
      fds->events = POLLIN;
      int rc = poll( fds, 1, timeOutLocal );
      if( rc < 0 )
      {
         res = errno;
         return false;
      }
      if( 0 == rc )
      {
         // indicate that we hit the timeout with nothing to receive
         res = -1;
         return true;
      }
      if( !( fds->revents & POLLIN ) )
         return false;
   }
   const ssize_t ssz = recv( s.socketfd, reinterpret_cast<void *>( buf ), count, 0 );
   if( ssz < 0 )
   {
      if( EAGAIN == errno || EWOULDBLOCK == errno )
      {
         res = -1;// nothing is available
         return true;
      }
      res = errno;// unusual case, but possible: interrupted by signal??
      return false;
   }
   res = static_cast<int>( ssz );
#endif
   return true;
}

bool p3SockRecv( const T_P3SOCKET s, char *buf, const int count, int &res )
{
   return p3SockRecvEx( s, buf, count, res, false, 0 );
}

bool p3SockRecvTimeout( const T_P3SOCKET s, char *buf, const int count, int &res, const int timeOut )
{
   return p3SockRecvEx( s, buf, count, res, true, timeOut );
}

// wait for / accept a client connection to the server socket
// returns the connected socket on success, invalid sockwet on failure
T_P3SOCKET p3SockAcceptClientConn( T_P3SOCKET srvSock )
{
   T_P3SOCKET res {};
   mkInvalidSock( res );
#if defined( _WIN32 )
   res.wsocket = accept( (SOCKET) srvSock.wsocket, nullptr, nullptr );
#else
   sockaddr_in cli {};
   // Accept the client connection request
   socklen_t len { sizeof( cli ) };
   const int connfd {accept(srvSock.socketfd, reinterpret_cast<sockaddr *>( &cli ), &len)};
   if(connfd < 0)
      return res;
   assert(len <= sizeof(cli));
   res.socketfd = connfd;
#endif
   return res;
}

// create a server socket listening on localhost
// if resuse is true,
//    set the SO_REUSEADDR option on the socket.  This prevents the socket from
//    entering the TIME_WAIT state when it is closed
// returns the socket on success, invalid socket on failure
T_P3SOCKET p3SockCreateServerSocket( int port, bool reuse )
{
   T_P3SOCKET res {};
   mkInvalidSock( res );
#if defined( _WIN32 )
   const auto acceptSocket { socket( AF_INET, SOCK_STREAM, 0 ) };
   if( INVALID_SOCKET == acceptSocket ) return res;
   if( reuse )
   {
      int enable { 1 };
      // without this magic Windows will bar connections to this port for a small time window
      // after the socket is closed to avoid delayed packets from a previous connection going to
      // any new connections
      if( setsockopt( acceptSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &enable, sizeof( int ) ) )
         return res;
   }
   SOCKADDR_IN addr;
   (void) std::memset( &addr, 0, sizeof( addr ) );
   addr.sin_family = AF_INET;
   addr.sin_port = htons( port );
   addr.sin_addr.s_addr = inet_addr( "127.0.0.1" );
   int rc { bind( acceptSocket, (SOCKADDR *) &addr, sizeof( addr ) ) };
   if( rc == SOCKET_ERROR )
   {
      closesocket( acceptSocket );
      return res;
   }
   rc = listen( acceptSocket, 5 );
   if( rc == SOCKET_ERROR )
   {
      closesocket( acceptSocket );
      return res;
   }
   res.wsocket = acceptSocket;
#else
   sockaddr_in servaddr {};
   const int sockfd { socket( AF_INET, SOCK_STREAM, 0 ) };
   if( sockfd == -1 ) return res;
   if( reuse )
   {
      int enable { 1 };
      // without this magic Linux/Mac will bar connections to this port for a small time window
      // after the socket is closed to avoid delayed packets from a previous connection going to
      // any new connections
      if( setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof( enable ) ) )
         return res;
   }
   (void) std::memset( &servaddr, 0, sizeof( servaddr ) );
   // assign IP, PORT
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
   servaddr.sin_port = htons( port );

   if( bind( sockfd, reinterpret_cast<sockaddr *>( &servaddr ), sizeof( servaddr ) ) )
   {
      (void) close( sockfd );
      return res;
   }
   if( listen( sockfd, 5 ) )
   {
      (void) close( sockfd );
      return res;
   }
   res.socketfd = sockfd;
#endif
   return res;
}

// determine port assigned to socket from OS
// useful in case bind was called with port=0
// RETURNS
//   port number on success.
//   -1 on failure: e.g. incorrect inputs or networking subsystem not initialized.
// if the function returns a port number >=0 res is not set.
// if the function returns -1:
//   *res: errno or WSAGetLastError value
int p3SockGetPort( T_P3SOCKET s, int &res )
{
   int result{-1};
   sockaddr_in addr {};
   int rc;
#if defined(_WIN32)
   int addrLen = sizeof(sockaddr_in);
   rc = getsockname(s.wsocket, (SOCKADDR*)&addr, &addrLen);
   if(rc)
   {
      res = WSAGetLastError();
      return -1;
   }
#else
   socklen_t addrLen = sizeof(sockaddr_in);
   rc = getsockname(s.socketfd, (sockaddr*)&addr, &addrLen);
   if(rc)
   {
      res = errno;
      return -1;
   }
#endif
   result = ntohs(addr.sin_port);
   return result;
}
#endif // __IN_CPPMEX__

}// namespace rtl::p3utils
