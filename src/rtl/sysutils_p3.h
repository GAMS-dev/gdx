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

#if defined(_WIN32)
// Forward declarations of Windows header stuff
typedef void *HANDLE;
struct _WIN32_FIND_DATAA;
#else
#include <dirent.h>
#endif

#include <cstdint>                 // for uint16_t, int64_t, uint32_t
#include <array>                    // for array
#include <string>                   // for string, basic_string
#include "../global/delphitypes.h"  // for tDateTime

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace rtl::sysutils_p3
{
// File attribute constants
constexpr int faReadOnly = 0x00000001,
          faHidden = 0x00000002,
          faSysFile = 0x00000004,
          faVolumeID = 0x00000008,
          faDirectory = 0x00000010,
          faArchive = 0x00000020,
          faSymLink = 0x00000040,
          faAnyFile = 0x0000003F;

// Units of time
constexpr int
        HoursPerDay = 24,
        MinsPerHour = 60,
        SecsPerMin = 60,
        MSecsPerSec = 1000,
        MinsPerDay = HoursPerDay * MinsPerHour,
        SecsPerDay = MinsPerDay * SecsPerMin,
        MSecsPerDay = SecsPerDay * MSecsPerSec;

// Days between 1/1/0001 and 12/31/1899
constexpr int DateDelta = 693594;

// Days between TDateTime basis (12/31/1899) and Unix time_t basis (1/1/1970)
constexpr int UnixDateDelta = 25569;

//const int MAX_PATH = 260;

struct TTimeStamp {
   int Time;// Number of milliseconds since midnight
   int Date;// One plus number of days since 1/1/0001
};

extern char PathDelim, DriveDelim, PathSep;
extern std::string FileStopper, ExtStopper;

std::string ExtractShortPathName( const std::string &FileName );

std::string ExtractFilePath( const std::string &FileName );
std::string ExtractFileName( const std::string &FileName );
std::string ExtractFileExt( const std::string &FileName );
std::string ChangeFileExt( const std::string &filename, const std::string &extension );
std::string CompleteFileExt( const std::string &filename, const std::string &extension );
std::string ReplaceFileExt( const std::string &filename, const std::string &extension );

bool FileExists( const std::string &FileName );
bool DirectoryExists( const std::string &Directory );

void DivMod( int Dividend, uint16_t Divisor, uint16_t &Result, uint16_t &Remainder );

std::string GetCurrentDir();

std::string SysErrorMessage( int errorCore );

bool DeleteFileFromDisk( const std::string &FileName );
bool RenameFile( const std::string &OldName, const std::string &NewName );

std::string QueryEnvironmentVariable( const std::string &Name );
int AssignEnvironmentVariable( const std::string &name, const std::string &value );
void DropEnvironmentVariable( const std::string &name );

std::string ExcludeTrailingPathDelimiter( const std::string &S );
std::string IncludeTrailingPathDelimiter( const std::string &S );

bool tryEncodeDate( uint16_t year, uint16_t month, uint16_t day, double &date );
double EncodeTime( uint16_t hour, uint16_t min, uint16_t sec, uint16_t msec);
double EncodeDate( uint16_t Year, uint16_t Month, uint16_t Day );
double EncodeDateTime( uint16_t Year, uint16_t Month, uint16_t Day, uint16_t Hour, uint16_t Minute, uint16_t Second, uint16_t Millisecond );
double Now();

void DecodeTime( global::delphitypes::tDateTime DateTime, uint16_t &Hour, uint16_t &Min, uint16_t &Sec, uint16_t &Msec );
void DecodeDate( global::delphitypes::tDateTime DateTime, uint16_t &Year, uint16_t &Month, uint16_t &Day );

bool isLeapYear( int year );

using TDayTable = std::array<uint16_t, 12>;
using PDayTable = TDayTable *;

constexpr std::array<TDayTable, 2> MonthDays {{
   {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
   {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
}};

int LastDelimiter( const char *Delimiters, const std::string &S );
int LastDelimiter( const std::string &Delimiters, const std::string &S );

bool CreateDir(const std::string &Dir);
bool RemoveDir(const std::string &Dir);

using TFileName = std::string;

struct TSearchRec {
   int Time {}, Size {}, Attr {};
   TFileName Name;
   int ExcludeAttr {};
#if defined(_WIN32)
   HANDLE FindHandle {};
   _WIN32_FIND_DATAA * FindData {};
#else
   DIR *FindHandle {};
#endif
   std::string PathOnly, Pattern;
   uint32_t mode {};

   ~TSearchRec();
};

int FindFirst( const std::string &Path, int Attr, TSearchRec &F );
int FindNext( TSearchRec &F );
void FindClose( TSearchRec &F );

void Sleep( uint32_t milliseconds );

std::string IntToStr(int64_t N);
void IntToStr(int64_t N, char *res, size_t &len );

double FileDateToDateTime( int fd );
int DateTimeToFileDate( double dt );

union LongRec {
   struct {
      uint16_t lo, hi;
   };
   uint8_t bytes[4];
};

}// namespace rtl::sysutils_p3
