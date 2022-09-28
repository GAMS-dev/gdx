#include "xpwrap.h"
#include "expertapi/gdxcc.h"
#include <iostream>
#include <cassert>
#include <cstring>

using namespace gdxinterface;

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

    int GDXFile::gdxDataWriteStr(const char **KeyStr, const double *Values) {
        //StrIndexBuffers keys{ &KeyStr };
        return ::gdxDataWriteStr(pgx, KeyStr, Values);
    }

    int GDXFile::gdxDataWriteDone() {
        return ::gdxDataWriteDone(pgx);
    }

    GDXFile::GDXFile(std::string &ErrMsg) {
        CharBuf msgBuf{};
        if (!::gdxLibraryLoaded()) {
            if (!::gdxGetReady(msgBuf.get(), msgBuf.size())) {
                std::cout << "Loading DLL failed!" << std::endl;
                exit(1);
            }
        }
        if (!::gdxCreate(&pgx, msgBuf.get(), msgBuf.size())) {
            std::cout << "Unable to create GDX object" << std::endl;
            exit(1);
        }
        ErrMsg = msgBuf.str();
    }

    GDXFile::~GDXFile() {
        if (pgx)
            ::gdxFree(&pgx);
        ::gdxLibraryUnload();
    }

    int GDXFile::gdxClose() {
        return ::gdxClose(pgx);
    }

    int GDXFile::gdxOpenRead(const std::string &FileName, int &ErrNr) {
        return ::gdxOpenRead(pgx, FileName.c_str(), &ErrNr);
    }

    int GDXFile::gdxFileVersion(std::string &FileStr, std::string &ProduceStr) {
        CharBuf fsBuf{}, psBuf{};
        int rc = ::gdxFileVersion(pgx, fsBuf.get(), psBuf.get());
        FileStr = fsBuf;
        ProduceStr = psBuf;
        return rc;
    }

    int GDXFile::gdxFindSymbol(const std::string &SyId, int &SyNr) {
        return ::gdxFindSymbol(pgx, SyId.c_str(), &SyNr);
    }

    int GDXFile::gdxDataReadStr(char **KeyStr, double *Values, int &DimFrst) {
        return ::gdxDataReadStr(pgx, KeyStr, Values, &DimFrst);
    }

    int GDXFile::gdxDataReadDone() {
        return ::gdxDataReadDone(pgx);
    }

    int GDXFile::gdxSymbolInfo(int SyNr, std::string &SyId, int &Dim, int &Typ) {
        CharBuf SyIdBuf{};
        int rc{::gdxSymbolInfo(pgx, SyNr, SyIdBuf.get(), &Dim, &Typ)};
        SyId = SyIdBuf;
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

    int GDXFile::gdxDataErrorRecord(int RecNr, int * KeyInt, double * Values) {
        return ::gdxDataErrorRecord(pgx, RecNr, KeyInt, Values);
    }
    int GDXFile::gdxDataErrorRecordX(int RecNr, int * KeyInt, double * Values) {
        return ::gdxDataErrorRecordX(pgx, RecNr, KeyInt, Values);
    }

    int GDXFile::gdxDataReadRaw(int *KeyInt, double *Values, int &DimFrst) {
        return ::gdxDataReadRaw(pgx, KeyInt, Values, &DimFrst);
    }

    int GDXFile::gdxDataReadRawStart(int SyNr, int &NrRecs) {
        return ::gdxDataReadRawStart(pgx, SyNr, &NrRecs);
    }

    int GDXFile::gdxDataWriteRaw(const int *KeyInt, const double *Values) {
        return ::gdxDataWriteRaw(pgx, KeyInt, Values);
    }

    int GDXFile::gdxDataWriteRawStart(const std::string &SyId, const std::string &ExplTxt, int Dimen, int Typ,
                                      int UserInfo) {
        return ::gdxDataWriteRawStart(pgx, SyId.c_str(), ExplTxt.c_str(), Dimen, Typ, UserInfo);
    }

    int GDXFile::gdxErrorCount() {
        return ::gdxErrorCount(pgx);
    }
    
    int GDXFile::gdxErrorStr(int ErrNr, std::string &ErrMsg) {
        CharBuf ErrMsgBuf{};
        int rc{ ::gdxErrorStr(pgx, ErrNr, ErrMsgBuf.get()) };
        ErrMsg = ErrMsgBuf;
        return rc;
    }

    int GDXFile::gdxGetElemText(int TxtNr, std::string &Txt, int &Node) {
        CharBuf TxtBuf{};
        int rc{ ::gdxGetElemText(pgx, TxtNr, TxtBuf.get(), &Node) };
        Txt = TxtBuf;
        return rc;

    }

    int GDXFile::gdxGetLastError() {
        return ::gdxGetLastError(pgx);
    }

    int GDXFile::gdxGetSpecialValues(gxdefs::TgdxSVals &Avals) {
        return ::gdxGetSpecialValues(pgx, Avals.data());
    }

    int GDXFile::gdxSetSpecialValues(const gxdefs::TgdxSVals &AVals) {
        return ::gdxSetSpecialValues(pgx, AVals.data());
    }

    int GDXFile::gdxSymbolGetDomain(int SyNr, int *DomainSyNrs) {
        return ::gdxSymbolGetDomain(pgx, SyNr, DomainSyNrs);
    }

    int GDXFile::gdxSymbolGetDomainX(int SyNr, char **DomainIDs) {
        return ::gdxSymbolGetDomainX(pgx, SyNr, DomainIDs);
    }

    int GDXFile::gdxSymbolDim(int SyNr) {
        return ::gdxSymbolDim(pgx, SyNr);
    }

    int GDXFile::gdxSymbolInfoX(int SyNr, int &RecCnt, int &UserInfo, std::string &ExplTxt) {
        CharBuf explTxtBuf{};
        int rc = ::gdxSymbolInfoX(pgx, SyNr, &RecCnt, &UserInfo, explTxtBuf.get());
        ExplTxt = explTxtBuf;
        return rc;
    }

    int GDXFile::gdxSymbolSetDomain(const char **DomainIDs) {
        return ::gdxSymbolSetDomain(pgx, DomainIDs);
    }

    int GDXFile::gdxSymbolSetDomainX(int SyNr, const char **DomainIDs) {
        return ::gdxSymbolSetDomainX(pgx, SyNr, DomainIDs);
    }

    int GDXFile::gdxSystemInfo(int &SyCnt, int &UelCnt) {
        return ::gdxSystemInfo(pgx, &SyCnt, &UelCnt);
    }

    int GDXFile::gdxUELRegisterDone() {
        return ::gdxUELRegisterDone(pgx);
    }

    int GDXFile::gdxUELRegisterRaw(const std::string &Uel) {
        return ::gdxUELRegisterRaw(pgx, Uel.c_str());
    }

    int GDXFile::gdxUELRegisterRawStart() {
        return ::gdxUELRegisterRawStart(pgx);
    }

    int GDXFile::gdxUELRegisterStr(const std::string &Uel, int &UelNr) {
        return ::gdxUELRegisterStr(pgx, Uel.c_str(), &UelNr);
    }

    int GDXFile::gdxUELRegisterStrStart() {
        return ::gdxUELRegisterStrStart(pgx);
    }

    int GDXFile::gdxUMUelGet(int UelNr, std::string &Uel, int &UelMap) {
        CharBuf uelBuf{};
        int rc{ ::gdxUMUelGet(pgx, UelNr, uelBuf.get(), &UelMap)};
        Uel = uelBuf;
        return rc;
    }

    int GDXFile::gdxUMUelInfo(int &UelCnt, int &HighMap) {
        return ::gdxUMUelInfo(pgx, &UelCnt, &HighMap);
    }

    int GDXFile::gdxCurrentDim() {
        return ::gdxCurrentDim(pgx);
    }

    int GDXFile::gdxRenameUEL(const std::string &OldName, const std::string &NewName) {
        return ::gdxRenameUEL(pgx, OldName.c_str(), NewName.c_str());
    }

    int GDXFile::gdxOpenReadEx(const std::string &FileName, int ReadMode, int &ErrNr) {
        return ::gdxOpenReadEx(pgx, FileName.c_str(), ReadMode, &ErrNr);
    }

    int GDXFile::gdxGetUEL(int uelNr, std::string &Uel) {
        CharBuf uelBuf{};
        int rc{::gdxGetUEL(pgx, uelNr, uelBuf.get())};
        Uel = uelBuf;
        return rc;
    }

    int GDXFile::gdxDataWriteMapStart(const std::string &SyId,
                                      const std::string &ExplTxt,
                                      int Dimen, int Typ, int UserInfo) {
        return ::gdxDataWriteMapStart(pgx, SyId.c_str(), ExplTxt.c_str(), Dimen, Typ, UserInfo);
    }

    int GDXFile::gdxDataWriteMap(const int *KeyInt, const double *Values) {
        return ::gdxDataWriteMap(pgx, KeyInt, Values);
    }

    int GDXFile::gdxUELRegisterMapStart() {
        return ::gdxUELRegisterMapStart(pgx);
    }

    int GDXFile::gdxUELRegisterMap(int UMap, const std::string &Uel) {
        return ::gdxUELRegisterMap(pgx, UMap, Uel.c_str());
    }

    int GDXFile::gdxDataReadMapStart(int SyNr, int &NrRecs) {
        return ::gdxDataReadMapStart(pgx, SyNr, &NrRecs);
    }

    int GDXFile::gdxDataReadMap(int RecNr, int *KeyInt, double *Values, int &DimFrst) {
        return ::gdxDataReadMap(pgx, RecNr, KeyInt, Values, &DimFrst);
    }

    int GDXFile::gdxAcronymCount() const {
        return ::gdxAcronymCount(pgx);
    }

    int GDXFile::gdxAcronymGetInfo(int N, std::string &AName, std::string &Txt, int &AIndx) const {
        CharBuf nameBuf{}, txtBuf{};
        int rc{::gdxAcronymGetInfo(pgx, N, nameBuf.get(), txtBuf.get(), &AIndx)};
        AName = nameBuf;
        Txt = txtBuf;
        return rc;
    }

    int GDXFile::gdxAcronymSetInfo(int N, const std::string &AName, const std::string &Txt, int AIndx) {
        return ::gdxAcronymSetInfo(pgx, N, AName.c_str(), Txt.c_str(), AIndx);
    }

    int GDXFile::gdxAcronymNextNr(int nv) {
        return ::gdxAcronymNextNr(pgx, nv);
    }

    int GDXFile::gdxAcronymGetMapping(int N, int &orgIndx, int &newIndx, int &autoIndex) {
        return ::gdxAcronymGetMapping(pgx, N, &orgIndx, &newIndx, &autoIndex);
    }

    int GDXFile::gdxFilterExists(int FilterNr) {
        return ::gdxFilterExists(pgx, FilterNr);
    }

    int GDXFile::gdxFilterRegisterStart(int FilterNr) {
        return ::gdxFilterRegisterStart(pgx, FilterNr);
    }

    int GDXFile::gdxFilterRegister(int UelMap) {
        return ::gdxFilterRegister(pgx, UelMap);
    }

    int GDXFile::gdxFilterRegisterDone() {
        return ::gdxFilterRegisterDone(pgx);
    }
}