#pragma once

#include <string>

namespace rtl::p3library {

    using TLibHandle = void*;

    TLibHandle P3LoadLibrary(const std::string& lib, std::string& loadMsg);
    void* P3GetProcAddress(TLibHandle handle, const std::string& name);
    bool P3FreeLibrary(TLibHandle handle);
    bool P3LibHandleIsNil(const TLibHandle handle);
    std::string P3MakeLibName(const std::string& path, const std::string& base);
    TLibHandle P3NilLibHandle();
    std::string P3LibraryExt();
    std::string P3LibraryPrefix();

}