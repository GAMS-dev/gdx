#include "xpwrap.h"
#include "expertapi/gdxcc.h"
#include <cassert>

namespace xpwrap {
    int GDXFile::gdxOpenWrite(const std::string &FileName, const std::string &Producer, int &ErrNr) {
        return ::gdxOpenWrite(pgx, FileName.c_str(), Producer.c_str(), &ErrNr);
    }

    int GDXFile::gdxOpenWriteEx(const std::string &FileName, const std::string &Producer, int Compr, int &ErrNr) {
        return ::gdxOpenWriteEx(pgx, FileName.c_str(), Producer.c_str(), Compr, &ErrNr);
    }

    int GDXFile::gdxDataWriteStrStart(const std::string &SyId, const std::string &ExplTxt, int Dim, int Typ, int UserInfo) {
        return ::gdxDataWriteStrStart(pgx, SyId.c_str(), ExplTxt.c_str(), Dim, Typ, UserInfo);
    }

    int GDXFile::gdxDataWriteStr(const gdxinterface::TgdxStrIndex &KeyStr, const gdxinterface::TgdxValues &Values) {
        static std::array<const char *, 20> KeyStrCstrs {};
        for(int i{}; i<KeyStrCstrs.size(); i++)
            KeyStrCstrs[i] = KeyStr[i].c_str();
        return ::gdxDataWriteStr(pgx, KeyStrCstrs.data(), Values.data());
    }

    int GDXFile::gdxDataWriteDone() {
        return ::gdxDataWriteDone(pgx);
    }

    GDXFile::GDXFile() {
        std::array<char, 256> msgBuf {};
        assert(::gdxCreate(&pgx, msgBuf.data(), msgBuf.size()));
    }

    GDXFile::~GDXFile() {
        ::gdxFree(&pgx);
    }

    int GDXFile::gdxClose() {
        return ::gdxClose(pgx);
    }
}