#pragma once

#include <string>
#include <vector>

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace rtl::p3utils {
    bool P3SetEnv(const std::string& name, const std::string &val);
    void P3UnSetEnv(const std::string &name);

    using Tp3File = std::fstream;
    using Tp3FileHandle = std::fstream*;
    enum Tp3FileOpenAction {
        p3OpenRead,
        p3OpenWrite,
        p3OpenReadWrite
    };

    int p3FileOpen(const std::string& fName, Tp3FileOpenAction mode, Tp3FileHandle h);
    int p3FileRead(Tp3FileHandle h, char* buffer, uint32_t buflen, uint32_t& numRead);
    int p3FileGetSize(Tp3FileHandle h, int64_t& fileSize);
}
