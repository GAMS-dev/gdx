#pragma once
#include "../global/delphitypes.h"

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace rtl::sysutils_p3 {
    // File attribute constants
    const int   faReadOnly  = 0x00000001,
                faHidden    = 0x00000002,
                faSysFile   = 0x00000004,
                faVolumeID  = 0x00000008,
                faDirectory = 0x00000010,
                faArchive   = 0x00000020,
                faSymLink   = 0x00000040,
                faAnyFile   = 0x0000003F;

    // Units of time
    const int
        HoursPerDay   = 24,
        MinsPerHour   = 60,
        SecsPerMin    = 60,
        MSecsPerSec   = 1000,
        MinsPerDay    = HoursPerDay * MinsPerHour,
        SecsPerDay    = MinsPerDay * SecsPerMin,
        MSecsPerDay   = SecsPerDay * MSecsPerSec;

    // Days between 1/1/0001 and 12/31/1899
    const int DateDelta = 693594;

    // Days between TDateTime basis (12/31/1899) and Unix time_t basis (1/1/1970)
    const int UnixDateDelta = 25569;

    //const int MAX_PATH = 260;

    struct TTimeStamp {
        int Time; // Number of milliseconds since midnight
        int Date; // One plus number of days since 1/1/0001
    };

    extern char PathDelim, DriveDelim, PathSep;
    extern std::string FileStopper, ExtStopper;

    std::string ExtractShortPathName(const std::string &FileName);

    std::string ExtractFilePath(const std::string &pathOfExecutable);
    std::string ExtractFileName(const std::string &FileName);
    std::string ExtractFileExt(const std::string& FileName);
    bool FileExists(const std::string& FileName);
    bool DirectoryExists(const std::string &Directory);

    void DivMod(int Dividend, uint16_t Divisor, uint16_t &Result, uint16_t &Remainder);

    std::string GetCurrentDir();

    std::string SysErrorMessage(int errorCore);

	bool DeleteFileFromDisk(const std::string &FileName);

    std::string QueryEnvironmentVariable(const std::string &Name);
    int AssignEnvironmentVariable(const std::string& name, const std::string& value);
    void DropEnvironmentVariable(const std::string& name);

    std::string ExcludeTrailingPathDelimiter(const std::string &S);
    std::string IncludeTrailingPathDelimiter(const std::string &S);

    bool tryEncodeDate(uint16_t year, uint16_t month, uint16_t day, double& date);
    double Now();

    void DecodeTime(const global::delphitypes::tDateTime DateTime, uint16_t& Hour, uint16_t& Min, uint16_t& Sec, uint16_t& Msec);
    void DecodeDate(double Date, uint16_t& Year, uint16_t& Month, uint16_t& Day);

    bool isLeapYear(int year);

    int LastDelimiter(const std::string& Delimiters, const std::string& S);

    using TFileName = std::string;

    struct TSearchRec {
        int Time, Size, Attr;
        TFileName Name;
        int ExcludeAttr;
        // ...
    };

    int FindFirst(const std::string& Path, int Attr, TSearchRec& F);
    int FindNext(TSearchRec& F);
    void FindClose(TSearchRec& F);
}
