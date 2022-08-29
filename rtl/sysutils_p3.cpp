#include <string>
#include <filesystem>
#include <cstring>
#include <ctime>

#include "sysutils_p3.h"
#include "p3platform.h"

#if defined(_WIN32)
#include <Windows.h>
#endif
#include "../global/modhead.h"
#include "../global/unit.h"

using namespace global::delphitypes;
using namespace rtl::p3platform;
using namespace std::literals::chrono_literals;
using namespace std::literals::string_literals;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace rtl::sysutils_p3 {
    namespace fs = std::filesystem;

    char PathDelim, DriveDelim, PathSep;
    std::string FileStopper, ExtStopper;

    std::string ExtractShortPathName(const std::string &FileName) {
#if defined(_WIN32)
        std::array<char, 260> buf;
        GetShortPathNameA(FileName.c_str(), buf.data(), sizeof(char)*buf.size());
        return buf.data();
#else
        // TODO: Does this make sense?
        return ""s;
#endif
    }

    std::string ExtractFilePath(const std::string& pathOfExecutable) {
        return fs::path(pathOfExecutable).parent_path().string();
    }

    std::string ExtractFileName(const std::string& FileName) {
        return fs::path(FileName).filename().string();
    }

    std::string ExtractFileExt(const std::string& FileName)
    {
        const auto p = fs::path(FileName);
        return p.has_extension() ? p.extension().string() : ""s;
    }

    bool FileExists(const std::string& FileName) {
        return std::filesystem::exists(FileName);
    }

    TTimeStamp DateTimeToTimeStamp(tDateTime DateTime) {
        return {
            (int)round(std::abs(frac(DateTime)) * MSecsPerDay),
            (int)(trunc(DateTime) + DateDelta)
        };
    }

    void DecodeTime(const tDateTime DateTime, uint16_t& Hour, uint16_t& Min, uint16_t& Sec, uint16_t& Msec) {
        uint16_t MinCount, MSecCount;
        const auto tmp = DateTimeToTimeStamp(DateTime);
        DivMod(tmp.Time, SecsPerMin * MSecsPerSec, MinCount, MSecCount);
        DivMod(MinCount, MinsPerHour, Hour, Min);
        DivMod(MSecCount, MSecsPerSec, Sec, Msec);
    }

    void DivMod(const int Dividend, const uint16_t Divisor, uint16_t& Result, uint16_t& Remainder) {
        const auto res = div(Dividend, Divisor);
        Result = res.quot;
        Remainder = res.rem;
    }

    global::delphitypes::tDateTime EncodeDate(uint16_t Year, uint16_t Month, uint16_t Day)
    {
        if (Year == 1600 && Month < 3)
            return Day + (Month == 1 ? 1 : 30);
        else {
            if (Month > 2) Month -= 3;
            else {
                Month += 9;
                Year--;
            }
            int Yr{ Year - 1600 };
            return Yr / 100 * 146097/ 4 + (Yr % 100) * 1461 / 4 + 153 * Month + 2 / 5 + Day + 59 - 109572 + 1;
        }
    }

    std::string GetCurrentDir() {
        return std::filesystem::current_path().string();
    }

    bool DirectoryExists(const std::string &Directory) {
        const auto dp = std::filesystem::path(Directory);
        return std::filesystem::exists(dp) && std::filesystem::is_directory(dp);
    }

    std::string SysErrorMessage(int errorCode) {
        char *errMsg = strerror(errorCode);
        if(!errMsg) return "Unknown error " + std::to_string(errorCode);
        return errMsg;
    }

	bool DeleteFileFromDisk(const std::string &FileName) {
		return std::filesystem::remove(std::filesystem::path(FileName));
	}

    std::string QueryEnvironmentVariable(const std::string &Name) {
#if defined(_WIN32)
        int len = GetEnvironmentVariableA(Name.c_str(), nullptr, 0);
        if (!len) return ""s;
        else {
            std::vector<char> buf(len);
            GetEnvironmentVariableA(Name.c_str(), buf.data(), len);
            std::string val(buf.begin(), buf.end()-1); // no terminating zero
            if (val.length() > 255) val = val.substr(0, 255);
            return val;
        }
#else
        const char* s = std::getenv(Name.c_str());
        std::string sout = s == nullptr ? ""s : s;
        if (sout.length() > 255) sout = sout.substr(0, 255);
        return sout;
#endif
    }

    // TODO: Potentially port P3SetEnv and P3UnSetEnv from portbin/rtl/p3utils
    int AssignEnvironmentVariable(const std::string& sid, const std::string& setval)
    {
        int rc;
#if defined(__APPLE__) || defined(__linux__) || defined(__unix__)
        rc = setval.empty() ? unsetenv(sid.c_str()) : setenv(sid.c_str(), setval.c_str(), 1);
#else
        rc = _putenv((sid + "=" + setval).c_str());
#endif
        return rc;
    }

    // TODO: Potentially should be portbin/rtl/p3utils/P3UnSetEnv
    void DropEnvironmentVariable(const std::string& name)
    {
        AssignEnvironmentVariable(name, "");
    }

    std::string ExcludeTrailingPathDelimiter(const std::string &S) {
        std::string res {S};
        if(!res.empty() && PathDelim == res[res.length()-1])
            res = res.substr(0, res.length()-1);
        return res;
    }

    std::string IncludeTrailingPathDelimiter(const std::string &S) {
        if(!S.empty() && PathDelim == S.back()) return S;
        return S + PathDelim;
    }

    const std::array<int, 12>
        daysPerMonthRegularYear = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
        daysPerMonthLeapYear = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    bool isLeapYear(int year) {
        // FIXME: Redo this for removing chrono stuff from C++20
        //return std::chrono::year{ year }.is_leap();
        return year % 4 == 0 && year % 4000 != 0 && year % 100 != 0
            || year % 400 == 0;
    }

    int LastDelimiter(const std::string& Delimiters, const std::string& S)
    {
        for (int i {(int)S.length() - 1}; i >= 0; i--)
            for (const char delim : Delimiters)
                if (delim != '\0' && delim == S[i])
                    return i;
        return -1;
    }

    int FindFirst(const std::string& Path, int Attr, TSearchRec& F)
    {
        // ...
        STUBWARN();
        return 0;
    }

    int FindNext(TSearchRec& F)
    {
        // ...
        STUBWARN();
        return 0;
    }

    void FindClose(TSearchRec& F)
    {
        // ...
        STUBWARN();
    }

    bool tryEncodeDate(uint16_t year, uint16_t month, uint16_t day, double& date) {
        const std::array<int, 12>& daysPerMonth = isLeapYear(year) ? daysPerMonthLeapYear : daysPerMonthRegularYear;

        if (year >= 1 && year <= 9999 && month >= 1 && month <= 12 && day >= 1 && day <= daysPerMonth[month - 1]) {
            int stop = month - 1;
            int i{ 1 };
            if (i <= stop) {
                do {
                    day += daysPerMonth[i - 1];
                } while (i++ != stop);
            }
            i = year - 1;
            date = trunc(i * 365.0 + i / 4.0 - i / 100.0 + i / 400.0 + day - DateDelta/* + 1*/);
            return true;
        }

        return false;
    }

    bool tryEncodeTime(uint16_t hour, uint16_t minute, uint16_t sec, uint16_t msec, double &curt) {
        if (hour < HoursPerDay && minute < MinsPerHour && /*sec < SecsPerDay &&*/ msec < MSecsPerSec) {
            curt = (hour * 36e5 + minute * 6e4 + sec * MSecsPerSec + msec) / MSecsPerDay;
            return true;
        }
        
        return false;
    }

    double Now()
    {
#if defined(_WIN32)
        SYSTEMTIME st;
        GetLocalTime(&st);
        double dnow, tnow;
        int rc{ tryEncodeDate(st.wYear, st.wMonth, st.wDay, dnow) };
        rc += tryEncodeTime(st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, tnow);
        return rc == 2 ? dnow + tnow : 0;
#else
        std::time_t t = std::time(0);
        std::tm* loctime = std::localtime(&t);
        
        const auto current_time_since_epoch{ std::chrono::system_clock::now().time_since_epoch() };
        // FIXME: Redo this for C++20 to C++17 switch!!!
        const auto current_milliseconds{ std::chrono::duration_cast<std::chrono::milliseconds> (current_time_since_epoch).count() % 1000 };

        double dnow, tnow;
        bool rc1 = tryEncodeDate(loctime->tm_year+1900, loctime->tm_mon+1, loctime->tm_mday, dnow);
        
        bool rc2 = tryEncodeTime(loctime->tm_hour, loctime->tm_min, loctime->tm_sec, current_milliseconds, tnow);
        return rc1 && rc2 ? dnow + tnow : 0.0;
#endif
    }

    double EncodeDateTime(uint16_t Year, uint16_t Month, uint16_t Day, uint16_t Hour, uint16_t Minute, uint16_t Second, uint16_t Millisecond) {
        double integerPart = EncodeDate(Year, Month, Day);
        double fractionalHoursInDay = (static_cast<double>(Millisecond) / 36e5) + static_cast<double>(Second) / 3600.0 + static_cast<double>(Minute) / 60.0 + Hour;
        return integerPart + fractionalHoursInDay / 24.0;
    }

    void DecodeDate(double Date, uint16_t& Year, uint16_t& Month, uint16_t& Day) {
        const int daysBetweenDelphiAndUnixRefdays = 25569;
        uint64_t days = static_cast<uint64_t>(std::trunc(Date));
        double fractionalHours = (Date - days) * 24;
        time_t later = static_cast<time_t>(((days - daysBetweenDelphiAndUnixRefdays) * 24 + fractionalHours) * 60 * 60);
        std::tm *later_tm = gmtime(&later);
        // FIXME: This should be treated correctly! Why does this even happen?
        if (!later_tm) {
            printf("\n*** WARNING: This should not happen. gmtime returned nullptr in DecodeDate (file=" __FILE__ ")\n");
            fflush(stdout);
            // ...
            Year = Month = Day = 0;
            return;
        }
        Year = later_tm->tm_year+1900;
        Month = later_tm->tm_mon+1;
        Day = later_tm->tm_mday;
    }

    void initialization() {
        switch(OSFileType()) {
            case OSFileWIN:
                PathDelim = '\\';
                DriveDelim = ':';
                PathSep = ';';
                FileStopper = "\\:";
                ExtStopper = "\\:.";
                break;

            case OSFileUNIX:
                PathDelim = '/';
                DriveDelim = '\0';
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

    void finalization() {

    }

    UNIT_INIT_FINI();
}
