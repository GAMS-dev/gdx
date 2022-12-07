#pragma once
#include "../global/delphitypes.h"

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace rtl::sysutils_p3 {
    extern char PathDelim, DriveDelim;
    std::string ExtractShortPathName(const std::string &FileName);
    std::string SysErrorMessage(int errorCore);
    std::string QueryEnvironmentVariable(const std::string &Name);
    int LastDelimiter(const std::string& Delimiters, const std::string& S);
}
