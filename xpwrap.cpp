#include "xpwrap.h"
#include "expertapi/gdxcc.h"
#include <iostream>
#include <cassert>
#include <cstring>

using namespace gdxinterface;
using namespace std::literals::string_literals;

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

    int GDXFile::gdxFileVersion(char *FileStr, char *ProduceStr) {
        return ::gdxFileVersion(pgx, FileStr, ProduceStr);
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

    int GDXFile::gdxSymbolInfo(int SyNr, char *SyId, int &Dim, int &Typ) {
        return ::gdxSymbolInfo(pgx, SyNr, SyId, &Dim, &Typ);
    }

    int GDXFile::gdxDataReadStrStart(int SyNr, int &NrRecs) {
        return ::gdxDataReadStrStart(pgx, SyNr, &NrRecs);
    }

    int GDXFile::gdxAddAlias(const std::string &Id1, const std::string &Id2) {
        return ::gdxAddAlias(pgx, Id1.c_str(), Id2.c_str());
    }

    int GDXFile::gdxAddSetText(const char *Txt, int &TxtNr) {
        return ::gdxAddSetText(pgx, Txt, &TxtNr);
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
    
    int GDXFile::gdxErrorStr(int ErrNr, char *ErrMsg) {
        return ::gdxErrorStr(pgx, ErrNr, ErrMsg);
    }

    int GDXFile::gdxGetElemText(int TxtNr, char *Txt, int &Node) {
        return ::gdxGetElemText(pgx, TxtNr, Txt, &Node);
    }

    int GDXFile::gdxGetLastError() {
        return ::gdxGetLastError(pgx);
    }

    int GDXFile::gdxGetSpecialValues(double *Avals) {
        return ::gdxGetSpecialValues(pgx, Avals);
    }

    int GDXFile::gdxSetSpecialValues(const double *AVals) {
        return ::gdxSetSpecialValues(pgx, AVals);
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

    int GDXFile::gdxSymbolInfoX(int SyNr, int &RecCnt, int &UserInfo, char *ExplTxt) {
        return ::gdxSymbolInfoX(pgx, SyNr, &RecCnt, &UserInfo, ExplTxt);
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

    int GDXFile::gdxUELRegisterRaw(const char *Uel) {
        return ::gdxUELRegisterRaw(pgx, Uel);
    }

    int GDXFile::gdxUELRegisterRawStart() {
        return ::gdxUELRegisterRawStart(pgx);
    }

    int GDXFile::gdxUELRegisterStr(const char *Uel, int &UelNr) {
        return ::gdxUELRegisterStr(pgx, Uel, &UelNr);
    }

    int GDXFile::gdxUELRegisterStrStart() {
        return ::gdxUELRegisterStrStart(pgx);
    }

    int GDXFile::gdxUMUelGet(int UelNr, char *Uel, int &UelMap) {
        return ::gdxUMUelGet(pgx, UelNr, Uel, &UelMap);
    }

    int GDXFile::gdxUMUelInfo(int &UelCnt, int &HighMap) {
        return ::gdxUMUelInfo(pgx, &UelCnt, &HighMap);
    }

    int GDXFile::gdxUMFindUEL(const char *Uel, int& UelNr, int& UelMap) {
        return ::gdxUMFindUEL(pgx, Uel, &UelNr, &UelMap);
    }

    int GDXFile::gdxCurrentDim() {
        return ::gdxCurrentDim(pgx);
    }

    int GDXFile::gdxRenameUEL(const char *OldName, const char *NewName) {
        return ::gdxRenameUEL(pgx, OldName, NewName);
    }

    int GDXFile::gdxOpenReadEx(const std::string &FileName, int ReadMode, int &ErrNr) {
        return ::gdxOpenReadEx(pgx, FileName.c_str(), ReadMode, &ErrNr);
    }

    int GDXFile::gdxGetUEL(int uelNr, char *Uel) {
        return ::gdxGetUEL(pgx, uelNr, Uel);
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

    int GDXFile::gdxUELRegisterMap(int UMap, const char *Uel) {
        return ::gdxUELRegisterMap(pgx, UMap, Uel);
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

    int GDXFile::gdxAcronymGetInfo(int N, char *AName, char *Txt, int &AIndx) const {
        return ::gdxAcronymGetInfo(pgx, N, AName, Txt, &AIndx);
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

    int GDXFile::gdxDataReadFilteredStart(int SyNr, const int *FilterAction, int &NrRecs) {
        return ::gdxDataReadFilteredStart(pgx, SyNr, FilterAction, &NrRecs);
    }

    int GDXFile::gdxAcronymAdd(const std::string &AName, const std::string &Txt, int AIndx) {
        return ::gdxAcronymAdd(pgx, AName.c_str(), Txt.c_str(), AIndx);
    }

    int GDXFile::gdxAcronymIndex(double V) const {
        return ::gdxAcronymIndex(pgx, V);
    }

    int GDXFile::gdxAcronymName(double V, char *AName) {
        return ::gdxAcronymName(pgx, V, AName);
    }

    double GDXFile::gdxAcronymValue(int AIndx) const {
        return ::gdxAcronymValue(pgx, AIndx);
    }

    int GDXFile::gdxSymbolAddComment(int SyNr, const char *Txt) {
        return ::gdxSymbolAddComment(pgx, SyNr, Txt);
    }

    int GDXFile::gdxSymbolGetComment(int SyNr, int N, char *Txt) {
        return ::gdxSymbolGetComment(pgx, SyNr, N, Txt);
    }

    int GDXFile::gdxStoreDomainSets() {
        return ::gdxStoreDomainSets(pgx);
    }

    void GDXFile::gdxStoreDomainSetsSet(int x) {
        ::gdxStoreDomainSetsSet(pgx, x);
    }

    int GDXFile::gdxOpenAppend(const std::string &FileName, const std::string &Producer, int &ErrNr) {
        return ::gdxOpenAppend(pgx, FileName.c_str(), Producer.c_str(), &ErrNr);
    }

    int GDXFile::gdxSymbIndxMaxLength(int SyNr, int *LengthInfo) {
        return ::gdxSymbIndxMaxLength(pgx, SyNr, LengthInfo);
    }

    int GDXFile::gdxUELMaxLength() {
        return ::gdxUELMaxLength(pgx);
    }

    std::string GDXFile::getImplName() const {
        return "xpwrap"s;
    }

    int GDXFile::gdxSetTraceLevel(int N, const std::string &s) {
        return ::gdxSetTraceLevel(pgx, N, s.c_str());
    }

    int GDXFile::gdxSetTextNodeNr(int TxtNr, int Node) {
        return ::gdxSetTextNodeNr(pgx, TxtNr, Node);
    }

    int GDXFile::gdxResetSpecialValues() {
        return ::gdxResetSpecialValues(pgx);
    }

    int
    GDXFile::gdxGetDomainElements(int SyNr, int DimPos, int FilterNr, TDomainIndexProc_t DP, int &NrElem, void *UPtr) {
        return ::gdxGetDomainElements(pgx, SyNr, DimPos, FilterNr, DP, &NrElem, UPtr);
    }

    int GDXFile::gdxAutoConvert(int nv) {
        return ::gdxAutoConvert(pgx, nv);
    }

    int GDXFile::gdxGetDLLVersion(char *V) const {
        return ::gdxGetDLLVersion(pgx, V);
    }

    int GDXFile::gdxFileInfo(int &FileVer, int &ComprLev) const {
        return ::gdxFileInfo(pgx, &FileVer, &ComprLev);
    }

    int GDXFile::gdxDataReadSliceStart(int SyNr, int *ElemCounts) {
        return ::gdxDataReadSliceStart(pgx, SyNr, ElemCounts);
    }

    int GDXFile::gdxDataReadSlice(const char **UelFilterStr, int &Dimen, TDataStoreProc_t DP) {
        return ::gdxDataReadSlice(pgx, UelFilterStr, &Dimen, DP);
    }

    int GDXFile::gdxDataSliceUELS(const int *SliceKeyInt, char **KeyStr) {
        return ::gdxDataSliceUELS(pgx, SliceKeyInt, KeyStr);
    }

    int64_t GDXFile::gdxGetMemoryUsed() {
        return ::gdxGetMemoryUsed(pgx);
    }

    int GDXFile::gdxMapValue(double D, int &sv) {
        return ::gdxMapValue(pgx, D, &sv);
    }

    int GDXFile::gdxSetHasText(int SyNr) {
        return ::gdxSetHasText(pgx, SyNr);
    }

    int GDXFile::gdxSetReadSpecialValues(const double *AVals) {
        return ::gdxSetReadSpecialValues(pgx, AVals);
    }

    int GDXFile::gdxSymbMaxLength() const {
        return ::gdxSymbMaxLength(pgx);
    }

    int GDXFile::gdxDataReadRawFastFilt(int SyNr, const char **UelFilterStr, TDataStoreFiltProc_t DP) {
        return ::gdxDataReadRawFastFilt(pgx, SyNr, UelFilterStr, DP);
    }

    int GDXFile::gdxDataReadRawFast(int SyNr, TDataStoreProc_t DP, int &NrRecs) {
        return ::gdxDataReadRawFast(pgx, SyNr, DP, &NrRecs);
    }
}