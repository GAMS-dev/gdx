#pragma once

#include <string>

namespace rtl::p3library {
    using TLibHandle = void*;
    TLibHandle P3LoadLibrary(const std::string& lib, std::string& loadMsg);
    void* P3GetProcAddress(TLibHandle handle, const std::string& name);
    bool P3FreeLibrary(TLibHandle handle);
}