#include "p3library.h"

#if defined(WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(_WIN32) || defined(__NT__)
#include <Windows.h>
#define MY_WINPLATFORM
#else
#include <dlfcn.h>
#endif

using namespace std::literals::string_literals;

namespace rtl::p3library {
    TLibHandle P3LoadLibrary(const std::string& lib, std::string& loadMsg)
    {
#ifdef MY_WINPLATFORM
        const auto oldMode = SetErrorMode(SEM_FAILCRITICALERRORS);
        HINSTANCE hinstLib = LoadLibraryA(lib.c_str());
        const auto lastErr = GetLastError();
        SetErrorMode(oldMode);
        if (!hinstLib)
            loadMsg = "Error message with code = "s + std::to_string(lastErr);
        return hinstLib;
#else
        void *libHandle = dlopen(lib.c_str(), RTLD_NOW | RTLD_GLOBAL);
        if(!libHandle)
            loadMsg = "Unable to load dynamic library named "s + lib + " with error: "s + dlerror();
        return libHandle;
#endif
    }

    bool P3FreeLibrary(TLibHandle handle)
    {
#ifdef MY_WINPLATFORM
        return FreeLibrary((HMODULE)handle);
#else
        return !dlclose((void *)handle);
#endif
    }

    void* P3GetProcAddress(TLibHandle handle, const std::string& name)
    {
#ifdef MY_WINPLATFORM
        return GetProcAddress((HMODULE)handle, name.c_str());
#else
        char *errMsg;
        dlerror(); // clear the error state, will not happen on success
        void *res = dlsym(handle, (char *)name.c_str());
        errMsg = dlerror();
        return NULL != errMsg ? nullptr : res;
#endif
    }
}