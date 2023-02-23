#include "p3utils.h"

#include <string>
#include <cstring>
#include <cassert>
#include <filesystem>
#include <fstream>

#include "sysutils_p3.h"
#include "../utils.h"

#ifdef _WIN32
#pragma comment(lib, "iphlpapi.lib")
//#define _WINSOCK2API_
#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#include <Windows.h>
#include <winsock2.h>
#include <io.h>
#include <psapi.h>  /* enough if we run on Windows 7 or later */
#include <iphlpapi.h>
#include <shlobj.h>
#include <IPTypes.h>
#else

# include <sys/socket.h>
# if (defined(__linux__) || defined(__APPLE__)) /* at least, maybe for others too */
#  if defined(__linux__)
#   include <sys/ioctl.h>
#   include <net/if.h>
#  elif defined(__APPLE__)
#   include <sys/ioctl.h>
#   include <sys/sysctl.h>
#   include <net/if.h>
#   include <net/if_dl.h>
#  endif
# endif
# include <netinet/in.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <dlfcn.h>
#endif

#if defined(__APPLE__)
#include <sys/proc_info.h>
#include <libproc.h>
#endif

using namespace rtl::sysutils_p3;
using namespace std::literals::string_literals;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace rtl::p3utils {

#if !defined(_WIN32)
    bool homePlus(const std::string &dd1, const std::string &dd2, std::string &s);
#endif

#if defined(_WIN32)
#else
    static bool setEnvironmentVariableUnix(const std::string &name, const std::string &value = ""s) {
        if(name.empty()) return false;
        if(value.empty()) { // delete name from env
            unsetenv(name.c_str());
            return true;
        }
        return !setenv(name.c_str(), value.c_str(), 1);
    }
#endif

    bool P3SetEnv(const std::string& name, const std::string &val)
    {
#if defined(_WIN32) || defined(_WIN64)
        return SetEnvironmentVariableA(name.c_str(), val.c_str());
#else
        return setEnvironmentVariableUnix(name, val);
#endif
    }

    void P3UnSetEnv(const std::string& name)
    {
#if defined(_WIN32) || defined(_WIN64)
        SetEnvironmentVariableA(name.c_str(), nullptr);
#else
        setEnvironmentVariableUnix(name);
#endif
    }

    int p3FileOpen(const std::string& fName, Tp3FileOpenAction mode, Tp3FileHandle h)
    {
        std::ios::openmode itsMode{ std::ios::binary };
        switch (mode) {
        case p3OpenRead:
            itsMode |= std::ios::in;
            break;
        case p3OpenWrite:
            itsMode |= std::ios::out;
            break;
        case p3OpenReadWrite:
            itsMode |= std::ios::in | std::ios::out;
            break;
        }
        h->open(fName, itsMode);
        bool f = h->fail();
        return f && !std::filesystem::exists(fName) ? 2 : f;
    }

    int p3FileGetSize(Tp3FileHandle fs, int64_t& fileSize)
    {
        if (!fs->is_open() || fs->bad()) return 1;
        std::streampos oldpos = fs->tellg();
        fs->seekg(0, std::ios::beg);
        std::streampos start = fs->tellg();
        fs->seekg(0, std::ios::end);
        fileSize = fs->tellg() - start;
        fs->seekg(oldpos);
        return 0;
    }

    int p3FileRead(Tp3FileHandle h, char* buffer, uint32_t buflen, uint32_t& numRead)
    {
        auto savedPos = h->tellg();
        h->seekg(0, h->end);
        numRead = std::min<uint32_t>(static_cast<uint32_t>(h->tellg()-savedPos), buflen);
        h->seekg(savedPos);
        h->read(buffer, numRead);
        return h->bad() ? 1 : 0;
    }

#ifndef _WIN32
    bool homePlus(const std::string &dd1, const std::string &dd2, std::string &s) {
        struct passwd *pw = getpwuid(getuid());
        const char *homePath = pw->pw_dir;
        if(!homePath || !strlen(homePath)) return false;
        s = ""s + homePath + dd1 + dd2;
        return true;
    }
#endif

    bool p3GetMemoryInfo(int64_t& rss, int64_t& vss)
    {
#if defined(_WIN32)
        PROCESS_MEMORY_COUNTERS info;
        int ok = GetProcessMemoryInfo (GetCurrentProcess( ), &info, sizeof(info));
        if (!ok)
            return false;  /* failure */
        rss = (int64_t) info.WorkingSetSize;
        vss = (int64_t) info.PagefileUsage;
        return true; /* success */
#elif defined(__linux)
        size_t sz;
        FILE *fp = fopen("/proc/self/statm", "r");
        if (!fp)
            return false;  /* failure */
        /* first two are VmSize, VmRSS */
        unsigned long urss, uvss;
        int n = fscanf (fp, "%lu %lu", &uvss, &urss);
        fclose(fp);
        if (2 != n)
            return false;  /* failure */
        sz = sysconf(_SC_PAGESIZE);
        rss = sz * urss;
        vss = sz * uvss;
        return true; /* success */
#elif defined(__APPLE__)
        int ret;
        struct proc_taskinfo procTaskInfo;
        ret = proc_pidinfo ((int) getpid(), PROC_PIDTASKINFO, 0,
                            (void *) &procTaskInfo, sizeof(procTaskInfo));
        if (ret < (int)sizeof(procTaskInfo))
            return false;  /* failure */
        rss = (int64_t) procTaskInfo.pti_resident_size;
        vss = (int64_t) procTaskInfo.pti_virtual_size;
        return true; /* success */
#else
        throw std::runtime_error("Unknown platform for getMemoryInfo!");
        return false; /* fail */
#endif
    }

}
