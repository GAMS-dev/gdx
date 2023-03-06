#include <string>
#include <filesystem>
#include <cstring>
#include <array>

#include "sysutils_p3.h"

#if defined(_WIN32)
#include <Windows.h>
#endif

using namespace std::literals::chrono_literals;
using namespace std::literals::string_literals;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace rtl::sysutils_p3 {
    namespace fs = std::filesystem;

#if defined(_WIN32)
    char PathDelim = '\\';
    char DriveDelim = ':';
#else
    char PathDelim = '/';
    char DriveDelim = '\0';
#endif

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
}
