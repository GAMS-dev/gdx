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

#include <string> // for string
#include <cstring> // for strerror, size_t, strcmp, strcpy

#include "sysutils_p3.h"
#include "p3platform.h" // for OSFileType, tOSFileType
#include "global/unit.h" // for UNIT_INIT_FINI

#if defined( _WIN32 )
   #include <Windows.h>
   #include <io.h>
#else
   #include <unistd.h> // for access, getcwd, unlink, F_OK
   #include <sys/stat.h> // for stat, S_ISDIR
   #include <sys/time.h> // for timeval, gettimeofday
   #include <dirent.h>
   #include <fnmatch.h>
#endif

using namespace global::delphitypes;
using namespace rtl::p3platform;
using namespace std::literals::string_literals;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace rtl::sysutils_p3
{
char PathDelim, DriveDelim, PathSep;
static std::array<char, 3> PathAndDriveDelim {'?', '?', '\0'};
std::string FileStopper, ExtStopper;

std::string ExtractShortPathName( const std::string &FileName )
{
#if defined( _WIN32 )
   std::array<char, 260> buf;
   auto rc = GetShortPathNameA( FileName.c_str(), buf.data(), static_cast<DWORD>( sizeof( char ) * buf.size() ) );
   assert(rc);
   return buf.data();
#else
   // TODO: Does this make sense?
   return ""s;
#endif
}

std::string ExtractFilePath( const std::string &FileName )
{
   const auto I {LastDelimiter( PathAndDriveDelim.data() , FileName )};
   return I == -1 ? ""s : FileName.substr(0, I+1);
}

std::string ExtractFileName( const std::string &FileName )
{
   const auto I { LastDelimiter( PathAndDriveDelim.data(), FileName ) };
   return I == -1 ? FileName : std::string {FileName.begin() + I + 1, FileName.end()};
}

std::string ExtractFileExt( const std::string &FileName )
{
   const auto I { LastDelimiter(ExtStopper, FileName)};
   return I > 0 && FileName[I] == '.' ? std::string {FileName.begin()+I, FileName.end()} : ""s;
}

bool FileExists( const std::string &FileName )
{
#if defined(_WIN32)
   return !_access(FileName.c_str(), 0);
#else
   return !access(FileName.c_str(), F_OK);
#endif
}

static TTimeStamp DateTimeToTimeStamp( tDateTime DateTime )
{
   return {
           static_cast<int>( round( std::abs( frac( DateTime ) ) * MSecsPerDay ) ),
           static_cast<int>( trunc( DateTime ) + DateDelta ) };
}

void DecodeTime( const tDateTime DateTime, uint16_t &Hour, uint16_t &Min, uint16_t &Sec, uint16_t &Msec )
{
   uint16_t MinCount, MSecCount;
   const auto tmp = DateTimeToTimeStamp( DateTime );
   DivMod( tmp.Time, SecsPerMin * MSecsPerSec, MinCount, MSecCount );
   DivMod( MinCount, MinsPerHour, Hour, Min );
   DivMod( MSecCount, MSecsPerSec, Sec, Msec );
}

void DivMod( const int Dividend, const uint16_t Divisor, uint16_t &Result, uint16_t &Remainder )
{
   const auto res = div( Dividend, Divisor );
   Result = res.quot;
   Remainder = res.rem;
}

/*static global::delphitypes::tDateTime EncodeDate( uint16_t Year, uint16_t Month, uint16_t Day )
{
   if( Year == 1600 && Month < 3 )
      return Day + ( Month == 1 ? 1 : 30 );
   if( Month > 2 ) Month -= 3;
   else
   {
      Month += 9;
      Year--;
   }
   const int Yr { Year - 1600 };
   return Yr / 100 * 146097 / 4 + Yr % 100 * 1461 / 4 + (153 * Month + 2) / 5 + Day + 59 - 109572 + 1;
}*/

#if defined(_WIN32)
static char * winErrMsg( int errNum, char *buf, int bufSiz )
{
   *buf = '\0';
   if( 0 == errNum )
      return buf;
   BOOL brc = FormatMessageA(
           FORMAT_MESSAGE_FROM_SYSTEM,
           nullptr,
           errNum,
           MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),// Default language
           buf,
           bufSiz - 1,
           nullptr );
   if( !brc )
   {
      *buf = '\0';
      return buf;
   }
   buf[bufSiz - 1] = '\0';

   /* Trim the end of the line and terminate it with a null */
   char *p = buf;
   while( ( *p > 31 ) || ( 9 == *p ) )
      ++p;
   do {
      *p-- = 0;
   } while( ( p >= buf ) && ( ( '.' == *p ) || ( 33 > *p ) ) );
   return buf;
} /* winErrMsg */
#endif

std::string GetCurrentDir()
{
   std::array<char, 256> buf;
   buf.front() = '\0';
#if defined(_WIN32)
   const int rc = GetCurrentDirectoryA(static_cast<DWORD>(sizeof(char)*buf.size()),buf.data());
   if (!rc) {
      winErrMsg( GetLastError(), buf.data(), sizeof( buf ) );
      throw std::runtime_error( "GetCurrentDir failed"s + buf.data() );
   }
   if (rc > static_cast<int>( buf.size() * sizeof(char) ))
      throw std::runtime_error("GetCurrentDir failed: result too large for shortString"s);
#else
   if (!getcwd(buf.data(), buf.size())) {
      if (ERANGE == errno) {
         throw std::runtime_error("GetCurrentDir failed: result too large for shortString"s);
      }
      else {
         char *p = strerror (errno);
         if (p)
            throw std::runtime_error("GetCurrentDir failed"s + p);
         else
            throw std::runtime_error("GetCurrentDir failed libc failure"s);
      }
   }
   else {
      // getcwd OK, but check if we can do better
      // # if defined(__linux__) realpath() expected everywhere
      const char *sym, *ss3;
      sym = getenv("PWD");
      char absp[4096];
      if (sym) {     /* got something, check if it is really same as getcwd */
         /* realpath(p,absp) converts the relative or symlink-ish path p
       * to an absolute physical path absp */
         ss3 = realpath(sym,absp);
         if (ss3 && (!std::strcmp(buf.data(),absp)) && (std::strlen(sym) < 256)) {
            std::strcpy(buf.data(),sym);
         }
      }
   }
#endif
   return buf.data();
}

bool DirectoryExists( const std::string &Directory )
{
#if defined(_WIN32)
   int attribs = GetFileAttributesA(Directory.c_str());
   return -1 != attribs && (attribs & FILE_ATTRIBUTE_DIRECTORY);
#else
   struct stat statBuf;
   return !stat(Directory.c_str(), &statBuf) ? S_ISDIR(statBuf.st_mode) : false;
#endif
}

std::string SysErrorMessage( int errorCode )
{
   const char *errMsg = strerror( errorCode );
   if( !errMsg ) return "Unknown error " + rtl::sysutils_p3::IntToStr( errorCode );
   return errMsg;
}

// *FromDisk to avoid name collision
bool DeleteFileFromDisk( const std::string &FileName )
{
#if defined(_WIN32)
   return DeleteFileA(FileName.c_str());
#else
   return unlink(FileName.c_str()) != -1;
#endif
}

std::string QueryEnvironmentVariable( const std::string &Name )
{
#if defined( _WIN32 )
   uint32_t len = GetEnvironmentVariableA( Name.c_str(), nullptr, 0 );
   if( !len ) return ""s;
   else
   {
      std::vector<char> buf( len );
      GetEnvironmentVariableA( Name.c_str(), buf.data(), len );
      std::string val( buf.begin(), buf.end() - 1 );// no terminating zero
      if( val.length() > 255 ) val = val.substr( 0, 255 );
      return val;
   }
#else
   const char *s = std::getenv( Name.c_str() );
   std::string sout = s == nullptr ? ""s : s;
   if( sout.length() > 255 ) sout = sout.substr( 0, 255 );
   return sout;
#endif
}

// TODO: Potentially port P3SetEnv and P3UnSetEnv from portbin/rtl/p3utils
int AssignEnvironmentVariable( const std::string &sid, const std::string &setval )
{
   int rc;
#if defined( __APPLE__ ) || defined( __linux__ ) || defined( __unix__ )
   rc = setval.empty() ? unsetenv( sid.c_str() ) : setenv( sid.c_str(), setval.c_str(), 1 );
#else
   rc = _putenv( ( sid + "=" + setval ).c_str() );
#endif
   return rc;
}

// TODO: Potentially should be portbin/rtl/p3utils/P3UnSetEnv
void DropEnvironmentVariable( const std::string &name )
{
   AssignEnvironmentVariable( name, "" );
}

std::string ExcludeTrailingPathDelimiter( const std::string &S )
{
   std::string res { S };
   if( !res.empty() && PathDelim == res[res.length() - 1] )
      res.pop_back();
   return res;
}

std::string IncludeTrailingPathDelimiter( const std::string &S )
{
   if( !S.empty() && PathDelim == S.back() ) return S;
   return S + PathDelim;
}

const std::array<int, 12>
        daysPerMonthRegularYear = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
        daysPerMonthLeapYear = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

bool isLeapYear( const int year )
{
   // FIXME: Redo this for removing chrono stuff from C++20
   //return std::chrono::year{ year }.is_leap();
   return (year % 4 == 0 && year % 4000 != 0 && year % 100 != 0) || year % 400 == 0;
}

int LastDelimiter(const char* Delimiters, const std::string& S)
{
   for( int i { static_cast<int>( S.length() ) - 1 }; i >= 0; i-- )
      for( const char *c=Delimiters; *c != '\0'; c++ )
         if( *c == S[i] )
            return i;
   return -1;
}

int LastDelimiter( const std::string &Delimiters, const std::string &S )
{
   for( int i { static_cast<int>( S.length() ) - 1 }; i >= 0; i-- )
      for( const char delim: Delimiters )
         if( delim != '\0' && delim == S[i] )
            return i;
   return -1;
}

bool CreateDir( const std::string &Dir )
{
#if defined(_WIN32)
   return CreateDirectoryA( Dir.c_str(), nullptr );
#else
   return !mkdir( Dir.c_str(), 0777 );
#endif
}

bool RemoveDir( const std::string &Dir )
{
#if defined(_WIN32)
   return RemoveDirectoryA(Dir.c_str());
#else
   return !rmdir(Dir.c_str());
#endif
}

std::string ChangeFileExt( const std::string &filename, const std::string &extension )
{
   auto I {LastDelimiter( ExtStopper, filename )};
   if(I == -1 || filename[I] != '.') I = static_cast<int>(filename.length());
   return filename.substr(0, I) + extension;

}

std::string CompleteFileExt( const std::string &filename, const std::string &extension )
{
   return ExtractFileExt( filename ).empty() ? ChangeFileExt( filename, extension ) : filename;
}

std::string ReplaceFileExt( const std::string &filename, const std::string &extension )
{
   return ChangeFileExt( filename, extension );
}

static int FindMatchingFile(TSearchRec &f)
{
   int result;
#if defined(_WIN32)
   while( f.FindData->dwFileAttributes & f.ExcludeAttr )
      if( !FindNextFile( f.FindHandle, reinterpret_cast<PWIN32_FIND_DATA>( &( f.FindData ) ) ) )
         return static_cast<int>(GetLastError());
   FILETIME lastWriteTime;
   FILETIME localFileTime;
   std::memcpy( &lastWriteTime, &( f.FindData->ftLastWriteTime ), sizeof( FILETIME ) );
   FileTimeToLocalFileTime( &lastWriteTime, &localFileTime );
   auto *wPtr = reinterpret_cast<WORD *>( &f.Time );
   FileTimeToDosDateTime( &localFileTime, wPtr + 1, wPtr );
   f.Size = static_cast<int>(f.FindData->nFileSizeLow);
   f.Attr = static_cast<int>(f.FindData->dwFileAttributes);
   f.Name = f.FindData->cFileName;
   return 0;
#else
   struct stat statbuf {};
   struct stat linkstatbuf {};
   std::string pattern;
   result = -1;
   auto *dp = f.FindHandle;
   const dirent *dirEntry = readdir( dp );
   if( dirEntry )
      pattern = f.Pattern;
   while( dirEntry )
   {
      if( int rc = fnmatch( pattern.c_str(), dirEntry->d_name, 0 ); 0 == rc )
      {
         // F.PathOnly must include trailing backslash
         std::string fname = f.PathOnly + dirEntry->d_name;
         rc = lstat( fname.c_str(), &statbuf );
         if( !rc )
         {
            int attr = 0;
            if( const mode_t mode = statbuf.st_mode; S_ISDIR( mode ) )
               attr |= faDirectory;
            else if( !S_ISREG( mode ) )
            {
               // directories should not be treated as system files
               if( S_ISLNK( mode ) )
               {
                  attr |= faSymLink;
                  if( 0 == lstat( fname.c_str(), &linkstatbuf ) && S_ISDIR( linkstatbuf.st_mode ) )
                     attr |= faDirectory;
               }
               attr |= faSysFile;
            }
            if( dirEntry->d_name[0] == '.' && dirEntry->d_name[1] != '\0' )
               if( !( dirEntry->d_name[1] == '.' && dirEntry->d_name[2] == '\0' ) )
                  attr |= faHidden;
            // if (euidaccess(fname, W_OK) != 0)
            if( access( fname.c_str(), W_OK ) != 0 )
               attr |= faReadOnly;
            if( 0 == ( attr & f.ExcludeAttr ) )
            {
               f.Size = static_cast<int>(statbuf.st_size);
               f.Attr = attr;
               f.mode = statbuf.st_mode;
               f.Name = dirEntry->d_name;
               f.Time = static_cast<int>(statbuf.st_mtime);
               result = 0;
               break;
            }// matching file found
         }   // lstat returns OK
      }      // matches desired pattern
      dirEntry = readdir( dp );
      result = -1;
   }// readdir loop
#endif // defined(_WIN32)
  return result;
}

int FindFirst(const std::string &Path, const int Attr, TSearchRec &F )
{
   F.ExcludeAttr = ~Attr & 30;
   F.PathOnly = ExtractFilePath( Path );
   if(F.PathOnly.empty())
      F.PathOnly = IncludeTrailingPathDelimiter( GetCurrentDir() );
   F.Pattern = ExtractFileName( Path );
#if defined(_WIN32)
   {
      HANDLE fHandle;
      auto len { Path.length() };
      F.FindHandle = fHandle = FindFirstFile(Path.c_str(), reinterpret_cast<PWIN32_FIND_DATA>( &F.FindData ) );
      if (INVALID_HANDLE_VALUE != fHandle) {
         auto res = FindMatchingFile(F);
         if (res != 0)
            FindClose(F);
         return res;
      }
      return GetLastError();
   }
#else
{
   DIR *dp;
   F.FindHandle = dp = opendir(F.PathOnly.c_str());
   if( dp )
   {
      auto res = FindMatchingFile( F );
      if( res )
         FindClose( F );
      return res;
   }
   return errno; // what should this be??
}
#endif
}

int FindNext( TSearchRec &F )
{
   // ...
   STUBWARN();
   return 0;
}

void FindClose( TSearchRec &F )
{
   // ...
   STUBWARN();
}

bool tryEncodeDate( uint16_t year, uint16_t month, uint16_t day, double &date )
{
   const std::array<int, 12> &daysPerMonth = isLeapYear( year ) ? daysPerMonthLeapYear : daysPerMonthRegularYear;

   if( year >= 1 && year <= 9999 && month >= 1 && month <= 12 && day >= 1 && day <= daysPerMonth[month - 1] )
   {
      const int stop = month - 1;
      int i { 1 };
      if( i <= stop )
      {
         do {
            day += daysPerMonth[i - 1];
         } while( i++ != stop );
      }
      i = year - 1;
      date = trunc( i * 365.0 + i / 4.0 - i / 100.0 + i / 400.0 + day - DateDelta /* + 1*/ );
      return true;
   }

   return false;
}

static bool tryEncodeTime( uint16_t hour, uint16_t minute, uint16_t sec, uint16_t msec, double &curt )
{
   if( hour < HoursPerDay && minute < MinsPerHour && /*sec < SecsPerDay &&*/ msec < MSecsPerSec )
   {
      curt = ( hour * 36e5 + minute * 6e4 + sec * MSecsPerSec + msec ) / MSecsPerDay;
      return true;
   }

   return false;
}

double Now()
{
#if defined( _WIN32 )
   SYSTEMTIME st;
   GetLocalTime( &st );
   double dnow, tnow;
   int rc { tryEncodeDate( st.wYear, st.wMonth, st.wDay, dnow ) };
   rc += tryEncodeTime( st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, tnow );
   return rc == 2 ? dnow + tnow : 0;
#else
   timeval tv;
   tm lt;
   if(gettimeofday(&tv, nullptr) || !localtime_r(&tv.tv_sec, &lt))
      return 0.0;
   double dnow, tnow;
   const bool rc1 = tryEncodeDate( lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, dnow );
   const bool rc2 = tryEncodeTime (lt.tm_hour, lt.tm_min, lt.tm_sec, tv.tv_usec/1000, tnow);
   return rc1 && rc2 ? dnow + tnow : 0.0;
#endif
}

/*static double EncodeDateTime( uint16_t Year, uint16_t Month, uint16_t Day, uint16_t Hour, uint16_t Minute, uint16_t Second, uint16_t Millisecond )
{
   const double integerPart = EncodeDate( Year, Month, Day );
   const double fractionalHoursInDay = static_cast<double>( Millisecond ) / 36e5 + static_cast<double>( Second ) / 3600.0 + static_cast<double>( Minute ) / 60.0 + Hour;
   return integerPart + fractionalHoursInDay / 24.0;
}*/

static bool DecodeDateFully(const double DateTime, uint16_t &Year, uint16_t &Month, uint16_t &Day, uint16_t &DOW)
{
   constexpr int  D1 {365},
                  D4 {D1*4+1},
                  D100 {D4*25-1},
                  D400{D100*4+1};
   int T { DateTimeToTimeStamp( DateTime ).Date };
   if( T <= 0 )
   {
      Year = Month = Day = DOW = 0;
      return false;
   }
   DOW = T % 7 + 1;
   T--;
   uint16_t Y = 1;
   while( T >= D400 )
   {
      T -= D400;
      Y += 400;
   }
   uint16_t I, D;
   DivMod( T, D100, I, D );
   if( I == 4 )
   {
      I--;
      D += D100;
   }
   Y += I * 100;
   DivMod( D, D4, I, D );
   Y += I * 4;
   DivMod( D, D1, I, D );
   if( I == 4 )
   {
      I--;
      D += D1;
   }
   Y += I;
   const auto res { isLeapYear( Y ) };
   const TDayTable *DayTable = &MonthDays[res];
   uint16_t M;
   for( M=1; true; M++ )
   {
      I = ( *DayTable )[M-1];
      if( D < I ) break;
      D -= I;
   }
   Year = Y;
   Month = M;
   Day = D + 1;
   return res;
}

void DecodeDate( const tDateTime DateTime, uint16_t &Year, uint16_t &Month, uint16_t &Day )
{
   uint16_t Dummy {};
   DecodeDateFully(DateTime, Year, Month, Day, Dummy);
}

bool RenameFile( const std::string &OldName, const std::string &NewName )
{
#if defined(_WIN32)
   return MoveFileA( OldName.c_str(), NewName.c_str() );
#else
   return !rename( OldName.c_str(), NewName.c_str() );
#endif
}

void Sleep( uint32_t milliseconds )
{
#ifdef _WIN32
   ::Sleep( milliseconds );
#else
   long nano;
   struct timespec req {}, rem {};
   req.tv_sec = milliseconds / 1000; /* whole seconds */
   nano = milliseconds % 1000;
   nano *= 1000000;
   req.tv_nsec = nano;
   (void) nanosleep( &req, &rem );
#endif
}

// SSN changed this to accept int64 arg. 27 Apr 03.
std::string IntToStr( int64_t n )
{
   /*
    * instead of reflecting negative vals to positive,
    * we reflect positive to negative.  This addresses the two's complement
    * issue (we have one more negative integer than positive
    */
   std::array<char, 256> res;
   int64_t w2{};
   if(n < 0)
   {
      res[0] = '-';
      w2 = 1;
   }
   else n *= -1;
   int64_t w {255};
   do {
      res[w-- - 1] = '0' - (char)(n % 10);
      n /= 10;
   } while(n);
   while(w < 255)
      res[w2++] = res[w++];
   return {res.data(), (size_t)w2};
}

// Buffer res must be at least 256 bytes wide!
void IntToStr( int64_t n, char *res, size_t &len )
{
   /*
    * instead of reflecting negative vals to positive,
    * we reflect positive to negative.  This addresses the two's complement
    * issue (we have one more negative integer than positive
    */
   int64_t w2 {};
   if( n < 0 )
   {
      res[0] = '-';
      w2 = 1;
   }
   else
      n *= -1;
   int64_t w { 255 };
   do {
      res[w-- - 1] = '0' - (char) ( n % 10 );
      n /= 10;
   } while( n );
   while( w < 255 )
      res[w2++] = res[w++];
   len = (size_t) w2;
   res[len] = '\0';
}

static void initialization()
{
   switch( OSFileType() )
   {
      case OSFileWIN:
         PathAndDriveDelim[0] = PathDelim = '\\';
         PathAndDriveDelim[1] = DriveDelim = ':';
         PathSep = ';';
         FileStopper = "\\:";
         ExtStopper = "\\:.";
         break;

      case OSFileUNIX:
         PathAndDriveDelim[0] = PathDelim = '/';
         PathAndDriveDelim[1] = DriveDelim = '\0';
         PathSep = ':';
         FileStopper = "/";
         ExtStopper = "/.";
         break;

      default:
         PathDelim = DriveDelim = PathSep = '?';
         FileStopper = ExtStopper = "?";
         break;
   }
}

static void finalization()
{
}

UNIT_INIT_FINI();
}// namespace rtl::sysutils_p3
