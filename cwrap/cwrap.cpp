// TODO: Potentially replace std::string usages with PChars
// TODO: Get rid of cwrap and use TGXFileObj class in gdxcclib.cPP directly
// Optional TODO: Maybe also add variants of some heavily used functions that directly take Delphi short strings

#include "cwrap.h"
#include "../gxfile.h"
#include <iostream>
#include <cassert>

using namespace gdxinterface;
using namespace gxfile;

extern "C" {

void GDX_CALLCONV doSetLoadPath(const char *s) {
    DLLLoadPath.assign(s);
}

void GDX_CALLCONV doGetLoadPath(char *s) {
    assert(DLLLoadPath.size() < 256);
    memcpy(s, DLLLoadPath.c_str(), DLLLoadPath.size());
}

gdxSetLoadPath_t gdxSetLoadPath = doSetLoadPath;
gdxGetLoadPath_t gdxGetLoadPath = doGetLoadPath;

int gdxCreate(TGXFileRec_t **pgdx, char *errBuf, int bufSize) {
    std::string ErrMsg;
    auto *pgx = new TGXFileObj {ErrMsg};
    if(!ErrMsg.empty())
        memcpy(errBuf, ErrMsg.c_str(), std::min<int>((int)ErrMsg.length()+1, bufSize));
    else
        errBuf[0] = '\0';
    *pgdx = reinterpret_cast<TGXFileRec_t *>(pgx);
    return true;
}

void gdxDestroy(TGXFileRec_t **pgx) {
    delete (TGXFileObj *)*pgx;
    *pgx = nullptr;
}

int gdxOpenWrite(TGXFileRec_t *pgx, const char *filename, const char *producer, int *ec) {
    return reinterpret_cast<TGXFileObj *>(pgx)->gdxOpenWrite(filename, producer, *ec);
}

int gdxOpenRead(TGXFileRec_t *pgx, const char *filename, int *ec) {
    return reinterpret_cast<TGXFileObj *>(pgx)->gdxOpenRead(filename, *ec);
}

int gdxClose(TGXFileRec_t *pgx) {
    return reinterpret_cast<TGXFileObj *>(pgx)->gdxClose();
}

int gdxOpenWriteEx(TGXFileRec_t *pgdx, const char *FileName, const char *Producer, int Compr, int *ErrNr) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxOpenWriteEx(FileName, Producer, Compr, *ErrNr);
}

int gdxDataWriteStrStart(TGXFileRec_t *pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataWriteStrStart(SyId, ExplTxt, Dimen, Typ, UserInfo);
}

int gdxDataWriteRaw(TGXFileRec_t *pgdx, const int *KeyInt, const double *Values) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataWriteRaw(KeyInt, Values);
}


int gdxAcronymAdd(TGXFileRec_t *pgdx, const char *AName, const char *Txt, int AIndx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxAcronymAdd(AName, Txt, AIndx);
}

int gdxAcronymCount(TGXFileRec_t *pgdx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxAcronymCount();
}

int gdxAcronymGetInfo(TGXFileRec_t *pgdx, int N, char *AName, char *Txt, int *AIndx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxAcronymGetInfo(N, AName, Txt, *AIndx);
}

int gdxAcronymGetMapping(TGXFileRec_t *pgdx, int N, int *orgIndx, int *newIndx, int *autoIndex) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxAcronymGetMapping(N, *orgIndx, *newIndx, *autoIndex);
}

int gdxAcronymIndex(TGXFileRec_t *pgdx, double V) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxAcronymIndex(V);
}

int gdxAcronymName(TGXFileRec_t *pgdx, double V, char *AName) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxAcronymName(V, AName);
}

int gdxAcronymNextNr(TGXFileRec_t *pgdx, int NV) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxAcronymNextNr(NV);
}

int gdxAcronymSetInfo(TGXFileRec_t *pgdx, int N, const char *AName, const char *Txt, int AIndx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxAcronymSetInfo(N, AName, Txt, AIndx);
}

double gdxAcronymValue(TGXFileRec_t *pgdx, int AIndx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxAcronymValue(AIndx);
}

int gdxAddAlias(TGXFileRec_t *pgdx, const char *Id1, const char *Id2) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxAddAlias(Id1, Id2);
}

int gdxAddSetText(TGXFileRec_t *pgdx, const char *Txt, int *TxtNr) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxAddSetText(Txt, *TxtNr);
}

int gdxAutoConvert(TGXFileRec_t *pgdx, int NV) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxAutoConvert(NV);
}

int gdxDataErrorCount(TGXFileRec_t *pgdx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataErrorCount();
}

int gdxDataErrorRecord(TGXFileRec_t *pgdx, int RecNr, int KeyInt[], double Values[]) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataErrorRecord(RecNr, KeyInt, Values);
}

int gdxDataErrorRecordX(TGXFileRec_t *pgdx, int RecNr, int KeyInt[], double Values[]) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataErrorRecordX(RecNr, KeyInt, Values);
}

int gdxDataReadDone(TGXFileRec_t *pgdx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataReadDone();
}

int gdxDataReadFilteredStart(TGXFileRec_t *pgdx, int SyNr, const int FilterAction[], int *NrRecs) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataReadFilteredStart(SyNr, FilterAction, *NrRecs);
}

int gdxDataReadMap(TGXFileRec_t *pgdx, int RecNr, int KeyInt[], double Values[], int *DimFrst) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataReadMap(RecNr, KeyInt, Values, *DimFrst);
}

int gdxDataReadMapStart(TGXFileRec_t *pgdx, int SyNr, int *NrRecs) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataReadMapStart(SyNr, *NrRecs);
}

int gdxDataReadRaw(TGXFileRec_t *pgdx, int KeyInt[], double Values[], int *DimFrst) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataReadRaw(KeyInt, Values, *DimFrst);
}

int gdxDataReadRawStart(TGXFileRec_t *pgdx, int SyNr, int *NrRecs) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataReadRawStart(SyNr, *NrRecs);
}

int gdxDataReadSlice(TGXFileRec_t *pgdx, const char *UelFilterStr[], int *Dimen, ::TDataStoreProc_t DP) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataReadSlice(UelFilterStr, *Dimen, (gxfile::TDataStoreProc_t)DP);
}

int gdxDataReadSliceStart(TGXFileRec_t *pgdx, int SyNr, int ElemCounts[]) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataReadSliceStart(SyNr, ElemCounts);
}

int gdxDataReadStr(TGXFileRec_t *pgdx, char *KeyStr[], double Values[], int *DimFrst) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataReadStr(KeyStr, Values, *DimFrst);
}

int gdxDataReadStrStart(TGXFileRec_t *pgdx, int SyNr, int *NrRecs) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataReadStrStart(SyNr, *NrRecs);
}

int gdxDataSliceUELS(TGXFileRec_t *pgdx, const int SliceKeyInt[], char *KeyStr[]) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataSliceUELS(SliceKeyInt, KeyStr);
}

int gdxDataWriteDone(TGXFileRec_t *pgdx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataWriteDone();
}

int gdxDataWriteMap(TGXFileRec_t *pgdx, const int KeyInt[], const double Values[]) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataWriteMap(KeyInt, Values);
}

int gdxDataWriteMapStart(TGXFileRec_t *pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataWriteMapStart(SyId, ExplTxt, Dimen, Typ, UserInfo);
}

int gdxDataWriteRawStart(TGXFileRec_t *pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataWriteRawStart(SyId, ExplTxt, Dimen, Typ, UserInfo);
}

int gdxDataWriteStr(TGXFileRec_t *pgdx, const char *KeyStr[], const double Values[]) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxDataWriteStr(KeyStr, Values);
}

int gdxGetDLLVersion(TGXFileRec_t *pgdx, char *V) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxGetDLLVersion(V);
}

int gdxErrorCount(TGXFileRec_t *pgdx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxErrorCount();
}

int gdxErrorStr(TGXFileRec_t *pgdx, int ErrNr, char *ErrMsg) {
    return pgdx ? reinterpret_cast<TGXFileObj*>(pgdx)->gdxErrorStr(ErrNr, ErrMsg) : TGXFileObj::gdxErrorStrStatic(ErrNr, ErrMsg);
}

int gdxFileInfo(TGXFileRec_t *pgdx, int *FileVer, int *ComprLev) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxFileInfo(*FileVer, *ComprLev);
}

int gdxFileVersion(TGXFileRec_t *pgdx, char *FileStr, char *ProduceStr) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxFileVersion(FileStr, ProduceStr);
}

int gdxFilterExists(TGXFileRec_t *pgdx, int FilterNr) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxFilterExists(FilterNr);
}

int gdxFilterRegister(TGXFileRec_t *pgdx, int UelMap) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxFilterRegister(UelMap);
}

int gdxFilterRegisterDone(TGXFileRec_t *pgdx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxFilterRegisterDone();
}

int gdxFilterRegisterStart(TGXFileRec_t *pgdx, int FilterNr) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxFilterRegisterStart(FilterNr);
}

int gdxFindSymbol(TGXFileRec_t *pgdx, const char *SyId, int *SyNr) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxFindSymbol(SyId, *SyNr);
}

int gdxGetElemText(TGXFileRec_t *pgdx, int TxtNr, char *Txt, int *Node) {
    std::string sTxt;
    int rc = reinterpret_cast<TGXFileObj *>(pgdx)->gdxGetElemText(TxtNr, sTxt, *Node);
    utils::stocp(sTxt, Txt);
    return rc;
}

int gdxGetLastError(TGXFileRec_t *pgdx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxGetLastError();
}

int gdxGetMemoryUsed(TGXFileRec_t *pgdx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxGetMemoryUsed();
}

int gdxGetSpecialValues(TGXFileRec_t *pgdx, double AVals[]) {
    std::array<double, 7> cppAVals{};
    int rc = reinterpret_cast<TGXFileObj *>(pgdx)->gdxGetSpecialValues(cppAVals);
    for(int i=0; i<cppAVals.size(); i++)
        AVals[i] = cppAVals[i];
    return rc;
}

int gdxGetUEL(TGXFileRec_t *pgdx, int UelNr, char *Uel) {
    std::string sUel;
    int rc = reinterpret_cast<TGXFileObj *>(pgdx)->gdxGetUEL(UelNr, sUel);
    utils::stocp(sUel, Uel);
    return rc;
}

int gdxMapValue(TGXFileRec_t *pgdx, double D, int *sv) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxMapValue(D, *sv);
}

int gdxOpenAppend(TGXFileRec_t *pgdx, const char *FileName, const char *Producer, int *ErrNr) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxOpenAppend(FileName, Producer, *ErrNr);
}

int gdxOpenReadEx(TGXFileRec_t *pgdx, const char *FileName, int ReadMode, int *ErrNr) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxOpenReadEx(FileName, ReadMode, *ErrNr);
}

int gdxResetSpecialValues(TGXFileRec_t *pgdx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxResetSpecialValues();
}

int gdxSetHasText(TGXFileRec_t *pgdx, int SyNr) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxSetHasText(SyNr);
}

int gdxSetReadSpecialValues(TGXFileRec_t *pgdx, const double AVals[]) {
    std::array<double, 7> cppAVals {};
    std::memcpy(cppAVals.data(), AVals, cppAVals.size()*sizeof(double));
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxSetReadSpecialValues(cppAVals);
}

int gdxSetSpecialValues(TGXFileRec_t *pgdx, const double AVals[]) {
    std::array<double, 7> cppAVals {};
    std::memcpy(cppAVals.data(), AVals, cppAVals.size()*sizeof(double));
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxSetSpecialValues(cppAVals);
}

int gdxSetTextNodeNr(TGXFileRec_t *pgdx, int TxtNr, int Node) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxSetTextNodeNr(TxtNr, Node);
}

int gdxSetTraceLevel(TGXFileRec_t *pgdx, int N, const char *s) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxSetTraceLevel(N, s);
}

int gdxSymbIndxMaxLength(TGXFileRec_t *pgdx, int SyNr, int LengthInfo[]) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxSymbIndxMaxLength(SyNr, LengthInfo);
}

int gdxSymbMaxLength(TGXFileRec_t *pgdx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxSymbMaxLength();
}

int gdxSymbolAddComment(TGXFileRec_t *pgdx, int SyNr, const char *Txt) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxSymbolAddComment(SyNr, Txt);
}

int gdxSymbolGetComment(TGXFileRec_t *pgdx, int SyNr, int N, char *Txt) {
    std::string sTxt;
    int rc{ reinterpret_cast<TGXFileObj *>(pgdx)->gdxSymbolGetComment(SyNr, N, sTxt) };
    utils::stocp(sTxt, Txt);
    return rc;
}

int gdxSymbolGetDomain(TGXFileRec_t *pgdx, int SyNr, int DomainSyNrs[]) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxSymbolGetDomain(SyNr, DomainSyNrs);
}

int gdxSymbolGetDomainX(TGXFileRec_t *pgdx, int SyNr, char *DomainIDs[]) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxSymbolGetDomainX(SyNr, DomainIDs);
}

int gdxSymbolDim(TGXFileRec_t *pgdx, int SyNr) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxSymbolDim(SyNr);
}

int gdxSymbolInfo(TGXFileRec_t *pgdx, int SyNr, char *SyId, int *Dimen, int *Typ) {
    std::string sSyId;
    int rc = reinterpret_cast<TGXFileObj *>(pgdx)->gdxSymbolInfo(SyNr, sSyId, *Dimen, *Typ);
    utils::stocp(sSyId, SyId);
    return rc;
}

int gdxSymbolInfoX(TGXFileRec_t *pgdx, int SyNr, int *RecCnt, int *UserInfo, char *ExplTxt) {
    std::string sExplTxt;
    int rc = reinterpret_cast<TGXFileObj *>(pgdx)->gdxSymbolInfoX(SyNr, *RecCnt, *UserInfo, sExplTxt);
    utils::stocp(sExplTxt, ExplTxt);
    return rc;
}

int gdxSymbolSetDomain(TGXFileRec_t *pgdx, const char *DomainIDs[]) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxSymbolSetDomain(DomainIDs);
}

int gdxSymbolSetDomainX(TGXFileRec_t *pgdx, int SyNr, const char *DomainIDs[]) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxSymbolSetDomainX(SyNr, DomainIDs);
}

int gdxSystemInfo(TGXFileRec_t *pgdx, int *SyCnt, int *UelCnt) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxSystemInfo(*SyCnt, *UelCnt);
}

int gdxUELMaxLength(TGXFileRec_t *pgdx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxUELMaxLength();
}

int gdxUELRegisterDone(TGXFileRec_t *pgdx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxUELRegisterDone();
}

int gdxUELRegisterMap(TGXFileRec_t *pgdx, int UMap, const char *Uel) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxUELRegisterMap(UMap, Uel);
}

int gdxUELRegisterMapStart(TGXFileRec_t *pgdx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxUELRegisterMapStart();
}

int gdxUELRegisterRaw(TGXFileRec_t *pgdx, const char *Uel) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxUELRegisterRaw(Uel);
}

int gdxUELRegisterRawStart(TGXFileRec_t *pgdx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxUELRegisterRawStart();
}

int gdxUELRegisterStr(TGXFileRec_t *pgdx, const char *Uel, int *UelNr) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxUELRegisterStr(Uel, *UelNr);
}

int gdxUELRegisterStrStart(TGXFileRec_t *pgdx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxUELRegisterStrStart();
}

int gdxUMFindUEL(TGXFileRec_t *pgdx, const char *Uel, int *UelNr, int *UelMap) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxUMFindUEL(Uel, *UelNr, *UelMap);
}

int gdxUMUelGet(TGXFileRec_t *pgdx, int UelNr, char *Uel, int *UelMap) {
    std::string sUel;
    int rc = reinterpret_cast<TGXFileObj *>(pgdx)->gdxUMUelGet(UelNr, sUel, *UelMap);
    utils::stocp(sUel, Uel);
    return rc;
}

int gdxUMUelInfo(TGXFileRec_t *pgdx, int *UelCnt, int *HighMap) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxUMUelInfo(*UelCnt, *HighMap);
}

int gdxGetDomainElements(TGXFileRec_t *pgdx, int SyNr, int DimPos, int FilterNr, ::TDomainIndexProc_t DP, int *NrElem, void *Uptr) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxGetDomainElements(SyNr, DimPos, FilterNr, (gxfile::TDomainIndexProc_t)DP, *NrElem, Uptr);
}

int gdxCurrentDim(TGXFileRec_t *pgdx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxCurrentDim();
}

int gdxRenameUEL(TGXFileRec_t *pgdx, const char *OldName, const char *NewName) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxRenameUEL(OldName, NewName);
}

int gdxStoreDomainSets(TGXFileRec_t *pgdx) {
    return reinterpret_cast<TGXFileObj *>(pgdx)->gdxStoreDomainSets();
}

void gdxStoreDomainSetsSet(TGXFileRec_t *pgdx, int x) {
    reinterpret_cast<TGXFileObj *>(pgdx)->gdxStoreDomainSetsSet(x);
}

int gdxFree(TGXFileRec_t **pgdx) {
    gdxDestroy(pgdx);
    return 1;
}

int gdxGetReady(char *msgBuf, int msgBufLen) {
    // FIXME: Is this enough?
    assert(msgBufLen > 0);
    msgBuf[0] = '\0';
    return 1;
}

int gdxLibraryLoaded() {
    // FIXME: Is this enough?
    return 1;
}

int gdxLibraryUnload() {
    // FIXME: Is this enough?
    return 1;
}

void gdxCreateD(TGXFileRec_t **pgdx, const char *sysDir, char *msgBuf, int msgBufLen) {
    // FIXME: Is this correct?
    doSetLoadPath(sysDir);
    gdxCreate(pgdx, msgBuf, msgBufLen);
}

int gdxDataReadRawFast(TGXFileRec_t *TGXFile, int SyNr, ::TDataStoreProc_t DP, int *NrRecs) {
    return reinterpret_cast<TGXFileObj *>(TGXFile)->gdxDataReadRawFast(SyNr, (gxfile::TDataStoreProc_t)DP, *NrRecs);
}

int gdxDataReadRawFastFilt(TGXFileRec_t *TGXFile, int SyNr, const char **UelFilterStr, ::TDataStoreFiltProc_t DP) {
    return reinterpret_cast<TGXFileObj *>(TGXFile)->gdxDataReadRawFastFilt(SyNr, UelFilterStr, (gxfile::TDataStoreFiltProc_t)DP);
}

void setCallByRef(const char *FuncName, int cbrValue) {
    // FIXME: Actually do something!
    // ...
}

#ifdef PYGDX_EXPERIMENT
int gdx_set1d(TGXFileRec_t *pgx, const char *name, const char **elems) {
    auto obj = reinterpret_cast<TGXFileObj *>(pgx);
    obj->gdxDataWriteStrStart(name, "A 1D set", 1, dt_set, 0);
    TgdxStrIndex keyStrs {};
    TgdxValues values {};
    int i;
    for(i=0; elems[i]; i++) {
        keyStrs[0].assign(elems[i]);
        obj->gdxDataWriteStr(&elems[i], values.data());
    }
    obj->gdxDataWriteDone();
    return i;
}

int create_gdx_file(const char *filename) {
    std::string ErrMsg;
    gxfile::TGXFileObj gdx{ErrMsg};
    if (!ErrMsg.empty()) {
        std::cout << "Unable to create GDX object. Error message:\n" << ErrMsg << '\n';
        return 1;
    }
    int ErrNr;
    if (!gdx.gdxOpenWrite(filename, "cwrap", ErrNr)) {
        std::cout << "Error opening " << filename << " for writing. Error code = " << ErrNr << '\n';
        return 1;
    }
    gdx.gdxDataWriteStrStart("i", "A simple set", 1, dt_set, 0);
    TgdxValues vals{};
    TgdxStrIndex keys{};
    for (int i{1}; i <= 5; i++) {
        keys[0] = "uel_" + std::to_string(i);
        const char *keyptrs[1];
        keyptrs[0] = keys[0].c_str();
        gdx.gdxDataWriteStr(keyptrs, vals.data());
    }
    gdx.gdxDataWriteDone();
    gdx.gdxClose();
    return 0;
}
#endif

}
