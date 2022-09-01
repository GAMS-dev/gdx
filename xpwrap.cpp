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

    int GDXFile::gdxDataWriteStr(const gxdefs::TgdxStrIndex &KeyStr, const gxdefs::TgdxValues &Values) {
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

    int GDXFile::gdxOpenRead(const std::string &FileName, int &ErrNr) {
        return ::gdxOpenRead(pgx, FileName.c_str(), &ErrNr);
    }

    int GDXFile::gdxFileVersion(std::string &FileStr, std::string &ProduceStr) {
        static std::array<char, 256> fsBuf{}, psBuf{};
        int rc = ::gdxFileVersion(pgx, fsBuf.data(), psBuf.data());
        FileStr.assign(fsBuf.data());
        ProduceStr.assign(psBuf.data());
        return rc;
    }

    int GDXFile::gdxFindSymbol(const std::string &SyId, int &SyNr) {
        return ::gdxFindSymbol(pgx, SyId.c_str(), &SyNr);
    }

    int GDXFile::gdxDataReadStr(gxdefs::TgdxStrIndex &KeyStr, gxdefs::TgdxValues &Values, int &DimFrst) {
        static std::array<std::array<char, 256>, 20> keyBuffers {};
        static std::array<char *, 20> keys {};
        for(int i=0; i<keys.size(); i++)
            keys[i] = &keyBuffers[i][0];
        int rc {::gdxDataReadStr(pgx, keys.data(), Values.data(), &DimFrst)};
        for(int i=0; i<keys.size(); i++)
            KeyStr[i].assign(keys[i]);
        return rc;
    }

    int GDXFile::gdxDataReadDone() {
        return ::gdxDataReadDone(pgx);
    }

    int GDXFile::gdxSymbolInfo(int SyNr, std::string &SyId, int &Dim, int &Typ) {
        static std::array<char, 256> SyIdBuf{};
        int rc{::gdxSymbolInfo(pgx, SyNr, SyIdBuf.data(), &Dim, &Typ)};
        SyId.assign(SyIdBuf.data());
        return rc;
    }

    int GDXFile::gdxDataReadStrStart(int SyNr, int &NrRecs) {
        return ::gdxDataReadStrStart(pgx, SyNr, &NrRecs);
    }

    int GDXFile::gdxAddAlias(const std::string &Id1, const std::string &Id2) {
        return ::gdxAddAlias(pgx, Id1.c_str(), Id2.c_str());
    }

    int GDXFile::gdxAddSetText(const std::string &Txt, int &TxtNr) {
        return ::gdxAddSetText(pgx, Txt.c_str(), &TxtNr);
    }

    int GDXFile::gdxDataErrorCount() {
        return ::gdxDataErrorCount(pgx);
    }

    int GDXFile::gdxDataErrorRecord(int RecNr, gxdefs::TgdxUELIndex& KeyInt, gxdefs::TgdxValues& Values) {
        return ::gdxDataErrorRecord(pgx, RecNr, KeyInt.data(), Values.data());
    }
    int GDXFile::gdxDataErrorRecordX(int RecNr, gxdefs::TgdxUELIndex& KeyInt, gxdefs::TgdxValues& Values) {
        return ::gdxDataErrorRecordX(pgx, RecNr, KeyInt.data(), Values.data());
    }

    int GDXFile::gdxDataReadRaw(gxdefs::TgdxUELIndex &KeyInt, gxdefs::TgdxValues &Values, int &DimFrst) {
        return ::gdxDataReadRaw(pgx, KeyInt.data(), Values.data(), &DimFrst);
    }

    int GDXFile::gdxDataReadRawStart(int SyNr, int &NrRecs) {
        return ::gdxDataReadRawStart(pgx, SyNr, &NrRecs);
    }

    int GDXFile::gdxDataWriteRaw(const gxdefs::TgdxUELIndex &KeyInt, const gxdefs::TgdxValues &Values) {
        return ::gdxDataWriteRaw(pgx, KeyInt.data(), Values.data());
    }

    int GDXFile::gdxDataWriteRawStart(const std::string &SyId, const std::string &ExplTxt, int Dimen, int Typ,
                                      int UserInfo) {
        return ::gdxDataWriteRawStart(pgx, SyId.c_str(), ExplTxt.c_str(), Dimen, Typ, UserInfo);
    }

    int GDXFile::gdxErrorCount() {
        return 0;
    }

    int GDXFile::gdxErrorStr(int ErrNr, std::string &ErrMsg) {
        return 0;
    }

    int GDXFile::gdxGetElemText(int TxtNr, std::string &Txt, int &Node) {
        return 0;
    }

    int GDXFile::gdxGetLastError() {
        return 0;
    }

    int GDXFile::gdxGetSpecialValues(gxdefs::TgdxSVals &Avals) {
        return 0;
    }

    int GDXFile::gdxSetSpecialValues(const gxdefs::TgdxSVals &AVals) {
        return 0;
    }

    int GDXFile::gdxSymbolGetDomain(int SyNr, gxdefs::TgdxUELIndex &DomainSyNrs) {
        return 0;
    }

    int GDXFile::gdxSymbolGetDomainX(int SyNr, gxdefs::TgdxStrIndex &DomainIDs) {
        return 0;
    }

    int GDXFile::gdxSymbolDim(int SyNr) {
        return 0;
    }

    int GDXFile::gdxSymbolInfoX(int SyNr, int &RecCnt, int &UserInfo, std::string &ExplTxt) {
        return 0;
    }

    int GDXFile::gdxSymbolSetDomain(const gxdefs::TgdxStrIndex &DomainIDs) {
        return 0;
    }

    int GDXFile::gdxSymbolSetDomainX(int SyNr, const gxdefs::TgdxStrIndex &DomainIDs) {
        return 0;
    }

    int GDXFile::gdxSystemInfo(int &SyCnt, int &UelCnt) {
        return 0;
    }

    int GDXFile::gdxUELRegisterDone() {
        return 0;
    }

    int GDXFile::gdxUELRegisterRaw(const std::string &Uel) {
        return 0;
    }

    int GDXFile::gdxUELRegisterRawStart() {
        return 0;
    }

    int GDXFile::gdxUELRegisterStr(const std::string &Uel, int &UelNr) {
        return 0;
    }

    int GDXFile::gdxUELRegisterStrStart() {
        return 0;
    }

    int GDXFile::gdxUMUelGet(int UelNr, std::string &Uel, int &UelMap) {
        return 0;
    }

    int GDXFile::gdxUMUelInfo(int &UelCnt, int &HighMap) {
        return 0;
    }

    int GDXFile::gdxCurrentDim() {
        return 0;
    }

    int GDXFile::gdxRenameUEL(const std::string &OldName, const std::string &NewName) {
        return 0;
    }
}