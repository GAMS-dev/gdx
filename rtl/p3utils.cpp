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

#include "p3platform.h"
#include "math_p3.h"
#include "../global/modhead.h"

#if defined(__APPLE__)
#include <sys/proc_info.h>
#include <libproc.h>
#endif

using namespace rtl::sysutils_p3;
using namespace rtl::p3platform;
using namespace std::literals::string_literals;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace rtl::p3utils {

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

    static std::vector<std::string> paramstr;

    std::string ExtractFileExt(const std::string& filename) {
        return std::filesystem::path(filename).extension().string();
    }

    std::string ChangeFileExt(const std::string& filename, const std::string& extension) {
        return std::filesystem::path(filename).replace_extension(std::filesystem::path(extension)).string();
    }

    std::string CompleteFileExt(const std::string& filename, const std::string& extension) {
        return ExtractFileExt(filename).empty() ? ChangeFileExt(filename, extension) : filename;
    }

    std::string ReplaceFileExt(const std::string& filename, const std::string& extension) {
        return ChangeFileExt(filename, extension);
    }

    bool PrefixPath(const std::string& s)
    {
        if(s.empty()) return true;

        const auto prevPath = rtl::sysutils_p3::QueryEnvironmentVariable("PATH");
        std::string newPath = s + rtl::sysutils_p3::PathSep + prevPath;
        return !rtl::sysutils_p3::AssignEnvironmentVariable("PATH", newPath);
    }

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

    bool P3IsSetEnv(const std::string& name)
    {
        return std::getenv(name.c_str()) != nullptr;
    }

    bool P3SetEnvPC(const std::string& name, char* val)
    {
        return P3SetEnv(name, val);
    }

    std::string P3GetEnv(const std::string &name) {
#if defined(_WIN32)
        std::array<char, 1024> buf;
        GetEnvironmentVariableA(name.c_str(), buf.data(), buf.size());
        return buf.data();
#else
        const char *p = getenv(name.c_str());
        return !p ? ""s : p;
#endif
    }

    // semantics identical to WinAPI GetEnvironmentVariable
    uint32_t P3GetEnvPC(const std::string& name, char* buf, uint32_t bufSize)
    {
#if defined(_WIN32)
        return GetEnvironmentVariableA(name.c_str(), buf, bufSize);
#else
        const char *p = getenv(name.c_str());
        if(!p) return 0; // no match in the env
        auto psiz = strlen(p) + 1;
        if(psiz <= bufSize) { // it fits: copy it over
            memmove(buf, p, psiz);
            return psiz-1;
        } else return psiz;
#endif
    }

    bool p3GetMemoryInfo(int64_t& rss, int64_t& vss)
    {
#if defined(_WIN32)
        PROCESS_MEMORY_COUNTERS info;
        int ok = GetProcessMemoryInfo (GetCurrentProcess( ), &info, sizeof(info));
        if (!ok)
            return false;  /* failure */
        rss = (int64_t) info.PagefileUsage;
        vss = (int64_t) info.WorkingSetSize;
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

    void p3SetConsoleTitle(const std::string& s)
    {
#if defined(_WIN32)
        SetConsoleTitleA(s.c_str());
#else
        // do nothing for now on Unix-like platforms
#endif
    }

    void p3NoPopups()
    {
#if defined(_WIN32)
        SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
#else
        // do nothing for now on Unix-like platforms
#endif
    }

    std::string p3GetUserName()
    {
#if defined(_WIN32)
        std::string res{ "unknown" };
        char userName[256];
        DWORD n = 256;
        if (GetUserNameA(userName, &n)) {
            res.assign(userName);
        }
        return res;
#elif defined(__APPLE__)
        char loginName[256];
        int rc = getlogin_r(loginName, sizeof(char)*256);
        return !rc ? loginName : ""s;
#else
        char loginName[256];
        char *p = cuserid(loginName);
        return p ? p : ""s;
#endif
    }

    std::string p3GetComputerName()
    {
#ifdef _WIN32
        std::string res{ "unknown" };
        char computerName[256];
        DWORD n = 256;
        if (GetComputerNameA(computerName, &n)) {
            res.assign(computerName);
        }
        return res;
#else
        struct utsname uts;
        int rc = uname(&uts);
        return rc >= 0 ? uts.nodename : ""s;
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

    int p3FileClose(Tp3FileHandle h)
    {
        h->close();
        return 0;
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
        numRead = std::min<int>(h->tellg()-savedPos, buflen);
        h->seekg(savedPos);
        h->read(buffer, numRead);
        return h->bad() ? 1 : 0;
    }

    int p3FileWrite(Tp3FileHandle h, const char *buffer, uint32_t buflen, uint32_t& numWritten) {
        h->write(buffer, buflen);
        numWritten = buflen;
        return h->bad() ? 1 : 0;
    }

    /*
     * Get a list(of sorts) of directories to search for config / data / doc / etc files
     * by convention, the first element in this list is the writableLocation
     * return true if we follow this convention(i.e.p3WritableLocation succeeds), false otherwise
     *  But even on false, locCountand locNames are valid
     */
    bool p3StandardLocations(Tp3Location locType, const std::string &appName, TLocNames &locNames, int &eCount) {
        eCount = 0;
        locNames = { ""s }; // { rtl::sysutils_p3::GetCurrentDir() };
        bool res = p3WritableLocation(locType, appName, locNames.front());
        if (p3Documents == locType) return res;

        bool isDataLoc {utils::in(locType,  p3Data, p3AppData, p3AppLocalData)};

        if (OSFileType() == OSFileWIN) {
            bool isConfigLoc {utils::in(locType,  p3Config, p3AppConfig)};
            if (isConfigLoc || isDataLoc) {
                std::string suffix = appName.empty() ? ""s : PathDelim + appName;
                locNames.emplace_back("C:\\ProgramData" + suffix);
            }
            if(isDataLoc) {
                std::string execName, msg;
                if(p3GetExecName(execName, msg)) {
                    eCount++;
                    return res;
                }
                std::string execPath {ExtractFilePath(execName)};
                locNames.emplace_back(ExcludeTrailingPathDelimiter(execPath));
                locNames.emplace_back(execPath + "data"s);
                if(!appName.empty()) locNames.emplace_back(locNames.back()+PathDelim+appName);
            }
        }
        else if(OSPlatform() == p3platform::OSDarwin_x64) {
            if(isDataLoc) {
                std::string msg {"/Library/Application Support"s + (!appName.empty() ? "/" + appName : ""s)};
                locNames.emplace_back(msg);
                std::string execName;
                if(p3GetExecName(execName, locNames.back())) {
                    eCount++;
                    return res;
                }
                std::string execPath {ExcludeTrailingPathDelimiter(ExtractFilePath(execName))};
                if(LastDelimiter("/", execPath) >= 2) locNames.emplace_back(ExtractFilePath(execPath) + "Resources"s);
                else eCount++;
            }
        }
        else { // neither Windows nor Mac, right now this must be Linux
            bool isPlainConfigLoc {p3Config == locType}, isAppConfigLoc {p3AppConfig == locType};
            std::array<char, 256> buf {};
            const auto bufLen = P3GetEnvPC(isDataLoc ? "XDG_DATA_DIRS" : "XDG_CONFIG_DIRS"s, buf.data(), buf.size());
            if(bufLen >= buf.size()) {// too much to handle
                eCount++;
                return res;
            } else if(bufLen > 0) { // we got something
                std::string msg = (isAppConfigLoc || isDataLoc) && !appName.empty() ? "/"s + appName : ""s;
                int dPos{}, k{};
                do {
                    while(buf[dPos] != '\0' && buf[dPos] != ':') dPos++;
                    int n = dPos - k;
                    if(n > 0) {
                        if(locNames.size() >= NLocNames) eCount++;
                        else locNames.emplace_back(""s + buf.data() + msg);
                    }
                    dPos++;
                    k = dPos;
                } while(k <= bufLen);
            } else {
                std::string prefix{"/etc/xdg"},
                            suffix {((isDataLoc || isAppConfigLoc) && !appName.empty() ? "/"s + appName : ""s)};
                if(isDataLoc) {
                    prefix = "/usr/local/share";
                    locNames.emplace_back(prefix + suffix);
                    prefix = "/usr/share";
                }
                locNames.emplace_back(prefix + suffix);
            }
        }
        return res;
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

    /*
     * Get the name of the directory to write config/data/doc/etc files to
     * return true on success (i.e. we can construct the name), false on failure
     */
    bool p3WritableLocation(Tp3Location locType, const std::string& appName, std::string& locName)
    {
#ifdef _WIN32
        locName.clear();
        if (utils::in(locType,  p3Config, p3AppConfig, p3Data, p3AppLocalData ))
            locName = QueryEnvironmentVariable("LOCALAPPDATA");
        else if (locType == p3AppData)
            locName = QueryEnvironmentVariable("APPDATA");
        else if (locType == p3Documents) {
            PWCHAR wideBuf;
            if (SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &wideBuf) == S_OK) {
                std::array<char, 256> buf{};
                size_t charCount{};
                wcstombs_s(&charCount, buf.data(), buf.size(), wideBuf, _TRUNCATE);
                locName.assign(buf.data());
            }
        }
        if (!locName.empty() && !appName.empty() && utils::in(locType,  p3Config, p3AppConfig, p3Data, p3AppData, p3AppLocalData ))
            locName += rtl::sysutils_p3::PathDelim + appName;
        return !locName.empty();
#else
        std::string dd = appName.empty() ? ""s : PathDelim + appName;

        if(OSPlatform() == p3platform::OSDarwin_x64) {
            if(p3Config == locType) {
                if(!homePlus("/Library/Preferences"s, ""s, locName))
                    return false;
            } else if(p3AppConfig == locType) {
                if(!homePlus("/Library/Preferences"s, dd, locName))
                    return false;
            } else if(utils::in(locType, p3Data, p3AppData, p3AppLocalData)) {
                if(!homePlus("/Library/Application Support"s, dd, locName))
                    return false;
            } else if(p3Documents == locType) {
                if(!homePlus("/Documents"s, ""s, locName))
                    return false;
            }
            else return false;
            return true;
        } else {
            // everything neither Windows nor macOS: only Linux in July 2022
            if (p3Config == locType || p3AppConfig == locType) {
                locName = QueryEnvironmentVariable("XDG_CONFIG_HOME");
                if (locName.empty() && !homePlus("/.config", dd, locName)) return false;
            } else if (utils::in(locType, p3Data, p3AppData, p3AppLocalData)) {
                locName = QueryEnvironmentVariable("XDG_DATA_HOME");
                if (locName.empty() && !homePlus("/.local/share", dd, locName)) return false;
            } else if (locType == p3Documents) {
                if (!homePlus("/Documents", dd, locName)) return false;
            } else return false;
            return true;
        }
#endif
        return false;
    }

    const std::string zeros = std::string(54, '0');

    int p3Chmod(const std::string &path, int mode) {
#if !defined(_WIN32)
        return chmod(path.c_str(), mode);
#endif
        return 0;
    }

    double RealTrunc(double x) {
        return static_cast<int>(x);
    }

    double ReadRound(double x) {
        return static_cast<int>(x+0.5*(x >= 0 ? 1.0 : -1.0));
    }

    const int MAXDIGITS = 17; // at most this many decimal digits from a double
    const std::array<double, MAXDIGITS+1> tenPow = {
        1e00, 1e01, 1e02, 1e03, 1e04, 1e05, 1e06, 1e07, 1e08, 1e09,
        1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17
    };
    const int MINTWOS = 4;
    const int MAXTWOS = 8;
    const std::array<int, MAXTWOS-MINTWOS+1> twoPow = {
        16, 32, 64, 128, 256
    };
    const std::array<double, MAXTWOS-MINTWOS+1> tenTwoPow = {
        1e16, 1e32, 1e64, 1e128, 1e256
    };

    bool delphiGetDecDigits(double y, int mode, int nDigits, std::string& digits, int& decPos, int& minusCnt)
    {
        // ...
        STUBWARN();
        return false;
    }

    bool p3GetDecDigits(double y, int mode, int nDigits, std::string& digits, int& decPos, int& minusCnt)
    {
        // ...
        STUBWARN();
        return false;
    }

    std::string p3FloatToEfmt(double x, int width, int decimals)
    {
        // ...
        STUBWARN();
        return std::string();
    }

    // return digits of input i64, with trailing zeros removed
    // i64 is assumed to contain a positive int < 1e17
    std::string getDigits(int64_t i64) {
        const int   M = 100000000,
                    LOGM = 8;
        std::string res;
        auto i = static_cast<int>(i64);
        if (i == i64) return std::to_string(i);
        else {
            i = int(i64 % M);
            std::string s = std::to_string(i);
            i = LOGM - static_cast<int>(s.length());
            if (i > 0) s = std::string(i, '0') + s;
            i = int(i64 / M);
            res = std::to_string(i) + s;
        };
        // strip trailing zeros
        for (i = static_cast<int>(res.length())-1; i >= 1 && res.back() == '0'; i--) {
            res.pop_back();
        }
        return res;
    }

    template<typename T>
    T myMin(T a, T b) { return a < b ? a : b; }

    std::string FloatToE(double y, int decimals) {
        auto myRoundTo = [](double x, int i) -> double {
            const double zeroFive = 0.5 * (x > 0.0 ? 1.0 : -1.0);
            if(!i) return (int)(x + zeroFive);
            else if(i > 0) {
                // use positive power of 10 to avoid roundoff error in z
                double z {rtl::math_p3::IntPower(10, i)};
                return (int)(x * z + zeroFive) * z;
            } else {
                double z {rtl::math_p3::IntPower(10, -i)};
                return (int)(x / z + zeroFive) * z;
            }
        };

        /* FloatToE */

        double x {std::abs(y)};
        if(x != 0.0) {
            int n{};
            while(x >= 1.0) {
                n++;
                x /= 10.0;
            }
            while(x < 1.0) {
                n--;
                x *= 10.0;
            }
            x = myRoundTo(x, decimals) * rtl::math_p3::IntPower(10.0, n);
        }
        std::string s = std::to_string(x);

        // edit and fix sign
        int k = LastDelimiter("+-" , s);
        auto j = s.find('.');
        decimals = utils::min<int>(k - j - 2, decimals);
        std::string res {"  "};
        if(y < 0) res[1] = '-';
        res += s.substr(j-1, decimals+2) + "E"s + s[k];
        int e, i;
        utils::val(s.substr(k, 5), e, i);
        e = std::abs(e);
        return res + s.substr(s.length()-2, 2);
    }

    std::string ParamStrZero() {
        return paramstr.front();
    }

    std::string ParamStr(int index) {
        return index >= 0 && index < paramstr.size() ? paramstr[index] : ""s;
    }

    int ParamStrCount()
    {
        return paramstr.size();
    }

    std::string loadPathVarName() {
        switch(OSPlatform()) {
            /*case OSAIX:
                return "LIBPATH"s;*/
            case OSDarwin_x64:
                return "DYLD_LIBRARY_PATH"s;
            case OSLinux86_64:
                return "LD_LIBRARY_PATH";
            default:
                return {};
        }
    }

    /*
     * Prefix the directory provided to the load path for shared objects.
     * On Windows, this routine does nothing, since there is no load path
     * if the argument is empty, we use the application directory as
     * taken from paramStr(0) returns true on success, false on failure
     */
    bool PrefixLoadPath(const std::string &dir) {
#if defined(_WIN32)
        return false;
#else
        std::string s { dir.empty() ? ExcludeTrailingPathDelimiter((ExtractFilePath(ParamStrZero()))) : dir },
                    ldPath = loadPathVarName();
        if(ldPath.empty()) return true;
        const char *tptr {getenv(ldPath.c_str())};
        return setEnvironmentVariableUnix(ldPath, s + (tptr ? ""s + rtl::sysutils_p3::PathSep + tptr : ""s));
#endif
    }

    /**
     * PrefixEnv: prefix the directory name dir to the environment variable evName,
     * if dir is not already the first directory in the list
     * returns true on success (prefixing worked or already there), false on failure
     */
    bool PrefixEnv(const std::string &dir, std::string &evName) {
        std::string trimDir {utils::trim(dir)};
        if(trimDir.empty()) return true;
#if defined(_WIN32)
        auto evSiz = GetEnvironmentVariableA(evName.c_str(), nullptr, 0);
        if(!evSiz) // not set? just set it!
            return SetEnvironmentVariableA(evName.c_str(), trimDir.c_str());
        std::vector<char> pPtr(evSiz);
        auto evLen = GetEnvironmentVariableA(evName.c_str(), pPtr.data(), evSiz);
        assert(evLen == evSiz - 1 && "PrefixEnv: strange GetEnvironmentVariable");
        char *opPtr = pPtr.data();
        while(opPtr[0] != '\0') {
            if(opPtr[0] != ' ') break;
            opPtr++;
            evLen--;
        }
        // if empty, just set it
        if('\0' == opPtr[0]) return SetEnvironmentVariableA(evName.c_str(), trimDir.c_str());
        // check if dir is the first element
        if((evLen == trimDir.length() && std::string(opPtr) == trimDir) || (evLen > trimDir.length() && PathSep == opPtr[trimDir.length()] && std::string(opPtr).substr(0, trimDir.length()) == trimDir))
            return true; // evName already starts with dir
        std::string tPtr {trimDir + PathSep + opPtr};
        return SetEnvironmentVariableA(evName.c_str(), tPtr.c_str());
#else
        std::string tptr = getenv(evName.c_str());
        if(tptr.empty()) return setEnvironmentVariableUnix(evName, dir);
        if(tptr.length() >= trimDir.length() && dir == tptr &&
           (tptr.length() == trimDir.length() || tptr[trimDir.length()] == PathSep)) return true;
        return setEnvironmentVariableUnix(evName, dir + PathSep + tptr);
#endif
    }

    void initParamStr(int argc, const char **argv) {
        paramstr.resize(argc);
        for(int i{}; i<argc; i++) {
            paramstr[i] = argv[i];
        }
    }

    bool p3GetFirstMACAddress(std::string &mac) {
#if defined(__linux__)
{
   struct ifreq ifr;
   struct ifconf ifc;
   char buf[1024];
   int success = 0, sock;

   sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
   if (sock == -1)
      return false;

   ifc.ifc_len = sizeof(buf);
   ifc.ifc_buf = buf;
   if (ioctl(sock, SIOCGIFCONF, &ifc) == -1)
      return false;

   {
      struct ifreq* it = ifc.ifc_req;
      const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

      for (; it != end; ++it) {
         strcpy(ifr.ifr_name, it->ifr_name);
         if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
            if (ifr.ifr_flags & IFF_LOOPBACK) /* don't count loopback */
               continue;
            if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
               success = 1;
               break;
            }
         }
      } /* loop over interfaces */
   }

   if (success) {
      unsigned char mb[6];
      memcpy(mb, ifr.ifr_hwaddr.sa_data, 6);
      char buf[18];
      sprintf(buf,"%02x:%02x:%02x:%02x:%02x:%02x", mb[0], mb[1], mb[2], mb[3], mb[4], mb[5]);
      mac.assign(buf);
      return true;
   }

   return false;
} /* if __linux__ */
#elif defined(__APPLE__)
{
  char prevName[IF_NAMESIZE];
  int mib[6];
  int sock;
  int halfDone = 0;  /* true if we have a MAC number for an interface that is down */
  struct ifconf ifc{};
  char buf[1024];
  char buf2[1024];
  unsigned char *mp;
  struct ifreq ifr{};
  struct ifreq *it, *end;
  size_t recLen, sz;
  struct if_msghdr *ifm;
  struct sockaddr_dl *sdl;

  sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
  if(sock < 0) return false;
  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;
  if(ioctl(sock, SIOCGIFCONF, &ifc)) return false;
  it = ifc.ifc_req;
  end = (struct ifreq *) ((unsigned char *) ifc.ifc_buf + ifc.ifc_len);

  mib[0] = CTL_NET;
  mib[1] = AF_ROUTE;
  mib[2] = 0;
  mib[3] = AF_LINK;
  mib[3] = 0;
  mib[4] = NET_RT_IFLIST;
  prevName[0] = '\0';
  for ( ;  it < end;  it = (struct ifreq *) ((unsigned char *) it + recLen)) {
    recLen = _SIZEOF_ADDR_IFREQ(*it);
    if (0==strcmp(it->ifr_name, prevName)) /* just checked it already */
      continue;
    (void) strcpy (prevName, it->ifr_name);
    (void) strcpy (ifr.ifr_name, it->ifr_name);
    if (ioctl(sock, SIOCGIFFLAGS, &ifr))  /* we should always get flags but if not skip ahead */
      continue;
    if (ifr.ifr_flags & IFF_LOOPBACK) /* always skip loopback interfaces */
      continue;
    if (halfDone && (0 == (ifr.ifr_flags & IFF_UP) ) )
      continue;  /* we already have a MAC address for a down interface */
    mib[5] = (int)if_nametoindex(it->ifr_name);
    if (0 == mib[5])
      continue;      /* no valid index found */
    sz = sizeof(buf2);
    if (sysctl(mib, 6, buf2, &sz, nullptr, 0))
      continue;     /* sysctl call failed */
    ifm = (struct if_msghdr *) buf2;
    /* printf ("msglen 0 = %d\n", ifm->ifm_msglen); */
    sdl = (struct sockaddr_dl *) (ifm +1);
    if (RTM_IFINFO != ifm->ifm_type)
      continue;     /* WTF */
    mp = (unsigned char *) LLADDR(sdl);
    char macBuf[18];
    sprintf((char*) macBuf,"%02x:%02x:%02x:%02x:%02x:%02x",
            mp[0], mp[1], mp[2], mp[3], mp[4], mp[5]);
    mac.assign(macBuf);
    if (0 != (ifr.ifr_flags & IFF_UP) )
      return true;
    else
      halfDone = 1;
  } /* loop over interfaces */
  return false;
} /* if __APPLE__ */
#elif defined(_WIN32)
  ULONG bufSiz, prevBufSiz;
  ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
  prevBufSiz = bufSiz = 4096;
  int nTries = 0, maxTries = 3;
  PIP_ADAPTER_ADDRESSES addrBuf = NULL;
  PIP_ADAPTER_ADDRESSES currAddr;
  DWORD dwrc, ifType;
  unsigned char *mp;
  int halfDone = 0; /* if we have a MAC number for an interface that is down */
  do {
    addrBuf = (IP_ADAPTER_ADDRESSES *) malloc(bufSiz);
    if (NULL == addrBuf)
      return false;
    dwrc = GetAdaptersAddresses(AF_INET, flags, NULL, addrBuf, &bufSiz);
    if (ERROR_BUFFER_OVERFLOW == dwrc) {
      prevBufSiz = bufSiz;
      free(addrBuf);
      addrBuf = NULL;
    }
    nTries++;
  } while ((ERROR_BUFFER_OVERFLOW == dwrc) && (nTries < maxTries));
  if (NO_ERROR != dwrc) {
    if (addrBuf)
      free(addrBuf);
    return false;
  }
  for (currAddr = addrBuf;  currAddr;  currAddr = currAddr->Next) {
    ifType = currAddr->IfType;
    if ( (IF_TYPE_ETHERNET_CSMACD != ifType) &&
         (IF_TYPE_IEEE80211 != ifType) )
      continue;
    if (halfDone && (IfOperStatusUp != currAddr->OperStatus))
      continue;  /* we already have a MAC address for a down interface */
    if (6 != currAddr->PhysicalAddressLength)
      continue;
    mp = (unsigned char *) currAddr->PhysicalAddress;
    char macBuf[18];
    sprintf((char*) macBuf,"%02x:%02x:%02x:%02x:%02x:%02x",
            mp[0], mp[1], mp[2], mp[3], mp[4], mp[5]);
    mac.assign(macBuf);
    if (IfOperStatusUp == currAddr->OperStatus) {
      free(addrBuf);
      return true;
    }
    else
      halfDone = 1;
    return false;
  }
  free(addrBuf);
  return false;
 /* if _WIN32 */
#else
        return false;
#endif
    }

    /* local use only: be sure to call with enough space for the sprintf */
    static void myStrError(int n, char *buf, size_t bufSiz)
    {
#if defined(_WIN32)
        if (strerror_s (buf, bufSiz, n))
            (void)sprintf(buf, "errno = %d", n);
#else
        if (strerror_r (n, buf, bufSiz))
            (void)sprintf(buf, "errno = %d", n);
#endif
    }

    static int xGetExecName(std::string &execName, std::string &msg) {
        int rc {8};
        std::array<char, 4096> execBuf {};
        std::array<char, 2048> tmpBuf{};

#if defined(__APPLE__)
        auto pid = getpid();
        int k = proc_pidpath(pid, execBuf.data(), sizeof(char)*execBuf.size());
        execName.assign(execBuf.data());
        if(k <= 0) {
            myStrError(errno, tmpBuf.data(), sizeof(char)*tmpBuf.size());
            msg = "proc_pidpath(pid="s + std::to_string(pid) + ") failed: "s + std::string(tmpBuf.begin(), tmpBuf.end());
            execName.clear();
            rc = 4;
        } else rc = 0;
#elif defined(__linux)
        auto ssz = readlink("/proc/self/exe", execBuf.data(), sizeof(char)*execBuf.size());
        execName.assign(execBuf.data());
        if(ssz < 0) {
            myStrError(errno, tmpBuf.data(), tmpBuf.size()*sizeof(char));
            msg = "readlink(/proc/self/exe,...) failure: "s + std::string(tmpBuf.begin(), tmpBuf.end());
            execName.clear();
            rc = 4;
        } else {
            ssz = std::min<int>(execBuf.size()-1, ssz);
            rc = 0;
        }
#elif defined(_WIN32)
        HMODULE h;
        int k = GetModuleFileNameA(nullptr, execBuf.data(), sizeof(char)*execBuf.size());
        if (!k) {
          msg = "GetModuleFileName() failure: rc="s + std::to_string(k);
          execName.clear();
          rc = 4;
        }
        else {
            rc = 0;
            execName.assign(execBuf.data());
        }
#else
        execName.clear();
        msg = "not implemented for this platform"s;
#endif

        return !rc && execName.length() > 255 ? 1 : rc;
    }

    // return:0 on success, 1 if truncated result, 2 if not lib, >2 o/w
    int p3GetExecName(std::string &execName, std::string &msg) {
        execName.clear();
#if defined(_WIN32)
        std::array<char, 256> buf{};
        auto rc = GetModuleFileNameA(nullptr, buf.data(), (int)buf.size());
        if(!rc) {
            msg = "GetModuleFileNameA call failed";
            return 3;
        }
        else if(rc >= 256) {
            buf.back() = '\0';
            execName.assign(buf.data());
            msg = "result truncated to 255 chars";
            return 1;
        } else {
            execName.assign(buf.data());
            msg.clear();
            return 0;
        }
#else
        msg = "P3: not yet implemented";
        int res = 9;
        res = xGetExecName(execName, msg);
        return res;
#endif
    }

    // FIXME: Do not always return false!
    bool isLibrary() {
        return false;
    }

    int xGetLibName(std::string &libName, std::string &msg)
    {
        char libBuf[4096], msgBuf[2048 + 32], tmpBuf[2048];
        int rc, k;

        *msgBuf = '\0';
        rc = 8;

#if defined(__linux) || defined(__APPLE__)
        {
            Dl_info dlInfo;

            k = dladdr((void*)(&xGetLibName), &dlInfo);
            if (k > 0) {
                strncpy(tmpBuf, dlInfo.dli_fname, sizeof(tmpBuf));
                tmpBuf[sizeof(tmpBuf) - 1] = '\0';
                if (realpath(tmpBuf, libBuf))
                    rc = 0;
                else {
                    myStrError(errno, tmpBuf, sizeof(tmpBuf));
                    sprintf(msgBuf, "realpath() failure: %s", tmpBuf);
                    *libBuf = '\0';
                    rc = 5;
                }
            }
            else {
                sprintf(msgBuf, "dladdr() failure");
                *libBuf = '\0';
                rc = 4;
            }
        }
#elif defined(_WIN32)
        {
            HMODULE h;
            k = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCTSTR)&xGetLibName, &h);
            if (k) {  /* OK: got a handle */
                k = GetModuleFileNameA(h, libBuf, sizeof(libBuf));
                if (0 == k) {
                    sprintf(msgBuf, "GetModuleFileName() failure: rc=%d", k);
                    *libBuf = '\0';
                    rc = 5;
                }
                else {
                    rc = 0;
                }
            }
            else {
                sprintf(msgBuf, "GetModuleHandleEx() failure: rc=%d", k);
                *libBuf = '\0';
                rc = 4;
            }
        }
#else
        *libBuf = '\0';
        (void)strcpy(msgBuf, "not implemented for this platform");
        rc = 8;
#endif

        libName.assign(libBuf);
        msg.assign(msgBuf);

        return ((0 == rc) && (strlen(libBuf) > 255)) ? 1 : rc;
    } /* xGetLibName */

    int p3GetLibName(std::string &libName, std::string &msg) {
        return xGetLibName(libName, msg);

#if defined(_WIN32)
        int res {9};
        libName.clear();
        if(!isLibrary()) {
            msg = "Not called from a library"s;
            return 2;
        }
        std::array<char, 256> buf;
        HMODULE hinstance;
        auto rc{ GetModuleFileNameA(hinstance, buf.data(), buf.size()) };
        if (!rc) {
            msg = "GetModuleFileNameA call failed"s;
            return 3;
        }
        else if (rc >= 256) {
            libName.assign(buf.data());
            msg = "Result truncated to 255 characters"s;
            return 1;
        }
        else {
            libName.assign(buf.data());
            msg.clear();
            return 0;
        }
#else
        if (!isLibrary()) {
            libName.clear();
            msg = "Not called from a library"s;
            return 2;
        }
        libName.clear();
        msg = "P3: not yet implemented"s;
        return 9;
#endif
    }
}
