#include <string>
#include <filesystem>
#include <cstring>
#include <ctime>
#include <array>

#include "sysutils_p3.h"

#if defined(_WIN32)
#include <Windows.h>
#endif

using namespace global::delphitypes;
using namespace std::literals::chrono_literals;
using namespace std::literals::string_literals;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace rtl::sysutils_p3 {
    namespace fs = std::filesystem;

    int AssignEnvironmentVariable(const std::string& sid, const std::string& setval);

#if defined(_WIN32)
    char PathDelim = '\\';
    char DriveDelim = ':';
#else
    char PathDelim = '/';
    char DriveDelim = '\0';
#endif

    std::string ExtractShortPathName(const std::string &FileName) {
#if defined(_WIN32)
        std::array<char, 260> buf;
        GetShortPathNameA(FileName.c_str(), buf.data(), (DWORD)(sizeof(char)*buf.size()));
        return buf.data();
#else
        // TODO: Does this make sense?
        return ""s;
#endif
    }

    std::string SysErrorMessage(int errorCode) {
#if defined(_WIN32)
        static std::array<char, 256> errMsgBuf;
        strerror_s(errMsgBuf.data(), (int)errMsgBuf.size(), errorCode);
        char *errMsg = errMsgBuf.data();
#else
        char *errMsg = strerror(errorCode);
#endif
        if(!errMsg) return "Unknown error " + std::to_string(errorCode);
        return errMsg;
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

    int LastDelimiter(const std::string& Delimiters, const std::string& S)
    {
        for (int i {(int)S.length() - 1}; i >= 0; i--)
            for (const char delim : Delimiters)
                if (delim != '\0' && delim == S[i])
                    return i;
        return -1;
    }
}
