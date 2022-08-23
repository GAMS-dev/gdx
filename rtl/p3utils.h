#pragma once

#include <string>
#include <vector>

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace rtl::p3utils {

    void initParamStr(int argc, const char **argv);

    int p3Chmod(const std::string &path, int mode);

    double RealTrunc(double x);
    double ReadRound(double x);

    // delphiGetDecDigits is exposed so we can test it again p3GetDecDigits
    bool delphiGetDecDigits(double y, int mode, int nDigits, std::string &digits, int &decPos, int &minusCnt);

    bool p3GetDecDigits(double y, int mode, int nDigits, std::string &digits, int &decPos, int &minusCnt);
    std::string p3FloatToEfmt(double x, int width, int decimals);
    std::string getDigits(int64_t i64);

    std::string FloatToE(double y, int decimals);

    std::string ParamStrZero();
    std::string ParamStr(int index);
    int ParamStrCount();
    std::string loadPathVarName();
    bool PrefixLoadPath(const std::string &dir);
    bool PrefixEnv(const std::string &dir, std::string &evName);

    const int NLocNames = 8;
    using TLocNames = std::vector<std::string>;

    enum Tp3Location { p3Config, p3AppConfig, p3Data, p3AppData, p3AppLocalData, p3Documents };

    bool p3StandardLocations(Tp3Location locType, const std::string &appName, TLocNames &locNames, int &eCount);
    bool p3WritableLocation(Tp3Location locType, const std::string& appName, std::string& locName);

    std::string ExtractFileExt(const std::string &filename);
    std::string ChangeFileExt(const std::string &filename, const std::string &extension);
    std::string CompleteFileExt(const std::string &filename, const std::string &extension);
    std::string ReplaceFileExt(const std::string &filename, const std::string &extension);

    bool PrefixPath(const std::string& s);

    bool P3SetEnv(const std::string& name, const std::string &val);
    std::string P3GetEnv(const std::string &name);
    void P3UnSetEnv(const std::string &name);
    bool P3IsSetEnv(const std::string &name);

    bool P3SetEnvPC(const std::string &name, char *val);
    uint32_t P3GetEnvPC(const std::string &name, char *buf, uint32_t bufSize);

    int p3GetExecName(std::string &execName, std::string &msg);
    int p3GetLibName(std::string &libName, std::string &msg);

    bool p3GetMemoryInfo(int64_t& rss, int64_t& vss);

    void p3SetConsoleTitle(const std::string &s);
    void p3NoPopups();

    std::string p3GetUserName();
    std::string p3GetComputerName();

    using Tp3File = std::fstream;
    using Tp3FileHandle = std::fstream*;
    enum Tp3FileOpenAction {
        p3OpenRead,
        p3OpenWrite,
        p3OpenReadWrite
    };

    int p3FileOpen(const std::string& fName, Tp3FileOpenAction mode, Tp3FileHandle h);
    int p3FileClose(Tp3FileHandle h);
    int p3FileRead(Tp3FileHandle h, char* buffer, uint32_t buflen, uint32_t& numRead);
    int p3FileWrite(Tp3FileHandle h, const char *buffer, uint32_t buflen, uint32_t& numWritten);
    int p3FileGetSize(Tp3FileHandle h, int64_t& fileSize);

    bool p3GetFirstMACAddress(std::string &mac);

    // ...

}
