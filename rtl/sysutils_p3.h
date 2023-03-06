#pragma once

#include <string>

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace rtl::sysutils_p3 {
    extern char PathDelim, DriveDelim;
    std::string SysErrorMessage(int errorCore);
    std::string QueryEnvironmentVariable(const std::string &Name);
}
