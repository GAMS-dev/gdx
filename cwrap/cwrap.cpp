// TODO: Try making most functions here one liner that delegate to an instance of TGXFileObj
// TODO: Get rid of cwrap and use TGXFileObj class in gdxcclib.cPP directly
// TODO: Optional: Maybe also add variants of some heavily used functions that directly take Delphi short strings

#include "cwrap.h"
#include "../gxfile.h"
#include <iostream>
#include <cassert>

extern "C" {

void GDX_CALLCONV doSetLoadPath(const char *s) {
    gxfile::DLLLoadPath.assign(s);
}

void GDX_CALLCONV doGetLoadPath(char *s) {
    assert(gxfile::DLLLoadPath.size() < 256);
    memcpy(s, gxfile::DLLLoadPath.c_str(), gxfile::DLLLoadPath.size());
}

gdxSetLoadPath_t gdxSetLoadPath = doSetLoadPath;
gdxGetLoadPath_t gdxGetLoadPath = doGetLoadPath;

int gdxCreate(TGXFileRec_t **TGXFile, char *errBuf, int bufSize) {
    std::string ErrMsg;
    auto *pgx = new gxfile::TGXFileObj {ErrMsg};
    if(!ErrMsg.empty())
        memcpy(errBuf, ErrMsg.c_str(), std::min<int>((int)ErrMsg.length()+1, bufSize));
    else
        errBuf[0] = '\0';
    *TGXFile = reinterpret_cast<TGXFileRec_t *>(pgx);
    return true;
}

void gdxDestroy(TGXFileRec_t **pgx) {
    delete (gxfile::TGXFileObj *)*pgx;
    *pgx = nullptr;
}

int gdxOpenWrite(TGXFileRec_t *pgx, const char *filename, const char *producer, int *ec) {
    return reinterpret_cast<gxfile::TGXFileObj *>(pgx)->gdxOpenWrite(filename, producer, *ec);
}

int gdxOpenRead(TGXFileRec_t *pgx, const char *filename, int *ec) {
    return reinterpret_cast<gxfile::TGXFileObj *>(pgx)->gdxOpenRead(filename, *ec);
}

int gdxClose(TGXFileRec_t *pgx) {
    return reinterpret_cast<gxfile::TGXFileObj *>(pgx)->gdxClose();
}

int gdxOpenWriteEx(TGXFileRec_t *TGXFile, const char *FileName, const char *Producer, int Compr, int *ErrNr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxOpenWriteEx(FileName, Producer, Compr, *ErrNr);
}

int gdxDataWriteStrStart(TGXFileRec_t *TGXFile, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataWriteStrStart(SyId, ExplTxt, Dimen, Typ, UserInfo);
}

int gdxDataWriteRaw(TGXFileRec_t *TGXFile, const int *KeyInt, const double *Values) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataWriteRaw(KeyInt, Values);
}


int gdxAcronymAdd(TGXFileRec_t *TGXFile, const char *AName, const char *Txt, int AIndx) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAcronymAdd(AName, Txt, AIndx);
}

int gdxAcronymCount(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAcronymCount();
}

int gdxAcronymGetInfo(TGXFileRec_t *TGXFile, int N, char *AName, char *Txt, int *AIndx) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAcronymGetInfo(N, AName, Txt, *AIndx);
}

int gdxAcronymGetMapping(TGXFileRec_t *TGXFile, int N, int *orgIndx, int *newIndx, int *autoIndex) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAcronymGetMapping(N, *orgIndx, *newIndx, *autoIndex);
}

int gdxAcronymIndex(TGXFileRec_t *TGXFile, double V) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAcronymIndex(V);
}

int gdxAcronymName(TGXFileRec_t *TGXFile, double V, char *AName) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAcronymName(V, AName);
}

int gdxAcronymNextNr(TGXFileRec_t *TGXFile, int NV) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAcronymNextNr(NV);
}

int gdxAcronymSetInfo(TGXFileRec_t *TGXFile, int N, const char *AName, const char *Txt, int AIndx) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAcronymSetInfo(N, AName, Txt, AIndx);
}

double gdxAcronymValue(TGXFileRec_t *TGXFile, int AIndx) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAcronymValue(AIndx);
}

int gdxAddAlias(TGXFileRec_t *TGXFile, const char *Id1, const char *Id2) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAddAlias(Id1, Id2);
}

int gdxAddSetText(TGXFileRec_t *TGXFile, const char *Txt, int *TxtNr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAddSetText(Txt, *TxtNr);
}

int gdxAutoConvert(TGXFileRec_t *TGXFile, int NV) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAutoConvert(NV);
}

int gdxDataErrorCount(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataErrorCount();
}

int gdxDataErrorRecord(TGXFileRec_t *TGXFile, int RecNr, int KeyInt[], double Values[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataErrorRecord(RecNr, KeyInt, Values);
}

int gdxDataErrorRecordX(TGXFileRec_t *TGXFile, int RecNr, int KeyInt[], double Values[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataErrorRecordX(RecNr, KeyInt, Values);
}

int gdxDataReadDone(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadDone();
}

int gdxDataReadFilteredStart(TGXFileRec_t *TGXFile, int SyNr, const int FilterAction[], int *NrRecs) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadFilteredStart(SyNr, FilterAction, *NrRecs);
}

int gdxDataReadMap(TGXFileRec_t *TGXFile, int RecNr, int KeyInt[], double Values[], int *DimFrst) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadMap(RecNr, KeyInt, Values, *DimFrst);
}

int gdxDataReadMapStart(TGXFileRec_t *TGXFile, int SyNr, int *NrRecs) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadMapStart(SyNr, *NrRecs);
}

int gdxDataReadRaw(TGXFileRec_t *TGXFile, int KeyInt[], double Values[], int *DimFrst) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadRaw(KeyInt, Values, *DimFrst);
}

int gdxDataReadRawStart(TGXFileRec_t *TGXFile, int SyNr, int *NrRecs) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadRawStart(SyNr, *NrRecs);
}

int gdxDataReadSlice(TGXFileRec_t *TGXFile, const char *UelFilterStr[], int *Dimen, ::TDataStoreProc_t DP) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadSlice(UelFilterStr, *Dimen, (gxfile::TDataStoreProc_t)DP);
}

int gdxDataReadSliceStart(TGXFileRec_t *TGXFile, int SyNr, int ElemCounts[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadSliceStart(SyNr, ElemCounts);
}

int gdxDataReadStr(TGXFileRec_t *TGXFile, char *KeyStr[], double Values[], int *DimFrst) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadStr(KeyStr, Values, *DimFrst);
}

int gdxDataReadStrStart(TGXFileRec_t *TGXFile, int SyNr, int *NrRecs) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadStrStart(SyNr, *NrRecs);
}

int gdxDataSliceUELS(TGXFileRec_t *TGXFile, const int SliceKeyInt[], char *KeyStr[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataSliceUELS(SliceKeyInt, KeyStr);
}

int gdxDataWriteDone(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataWriteDone();
}

int gdxDataWriteMap(TGXFileRec_t *TGXFile, const int KeyInt[], const double Values[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataWriteMap(KeyInt, Values);
}

int gdxDataWriteMapStart(TGXFileRec_t *TGXFile, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataWriteMapStart(SyId, ExplTxt, Dimen, Typ, UserInfo);
}

int gdxDataWriteRawStart(TGXFileRec_t *TGXFile, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataWriteRawStart(SyId, ExplTxt, Dimen, Typ, UserInfo);
}

int gdxDataWriteStr(TGXFileRec_t *TGXFile, const char *KeyStr[], const double Values[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataWriteStr(KeyStr, Values);
}

int gdxGetDLLVersion(TGXFileRec_t *TGXFile, char *V) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxGetDLLVersion(V);
}

int gdxErrorCount(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxErrorCount();
}

int gdxErrorStr(TGXFileRec_t *TGXFile, int ErrNr, char *ErrMsg) {
    return TGXFile ? reinterpret_cast<gxfile::TGXFileObj*>(TGXFile)->gdxErrorStr(ErrNr, ErrMsg) : gxfile::TGXFileObj::gdxErrorStrStatic(ErrNr, ErrMsg);
}

int gdxFileInfo(TGXFileRec_t *TGXFile, int *FileVer, int *ComprLev) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxFileInfo(*FileVer, *ComprLev);
}

int gdxFileVersion(TGXFileRec_t *TGXFile, char *FileStr, char *ProduceStr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxFileVersion(FileStr, ProduceStr);
}

int gdxFilterExists(TGXFileRec_t *TGXFile, int FilterNr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxFilterExists(FilterNr);
}

int gdxFilterRegister(TGXFileRec_t *TGXFile, int UelMap) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxFilterRegister(UelMap);
}

int gdxFilterRegisterDone(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxFilterRegisterDone();
}

int gdxFilterRegisterStart(TGXFileRec_t *TGXFile, int FilterNr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxFilterRegisterStart(FilterNr);
}

int gdxFindSymbol(TGXFileRec_t *TGXFile, const char *SyId, int *SyNr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxFindSymbol(SyId, *SyNr);
}

int gdxGetElemText(TGXFileRec_t *TGXFile, int TxtNr, char *Txt, int *Node) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxGetElemText(TxtNr, Txt, *Node);
}

int gdxGetLastError(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxGetLastError();
}

int gdxGetMemoryUsed(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxGetMemoryUsed();
}

int gdxGetSpecialValues(TGXFileRec_t *TGXFile, double AVals[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxGetSpecialValues(AVals);
}

int gdxGetUEL(TGXFileRec_t *TGXFile, int UelNr, char *Uel) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxGetUEL(UelNr, Uel);
}

int gdxMapValue(TGXFileRec_t *TGXFile, double D, int *sv) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxMapValue(D, *sv);
}

int gdxOpenAppend(TGXFileRec_t *TGXFile, const char *FileName, const char *Producer, int *ErrNr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxOpenAppend(FileName, Producer, *ErrNr);
}

int gdxOpenReadEx(TGXFileRec_t *TGXFile, const char *FileName, int ReadMode, int *ErrNr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxOpenReadEx(FileName, ReadMode, *ErrNr);
}

int gdxResetSpecialValues(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxResetSpecialValues();
}

int gdxSetHasText(TGXFileRec_t *TGXFile, int SyNr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSetHasText(SyNr);
}

int gdxSetReadSpecialValues(TGXFileRec_t *TGXFile, const double AVals[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSetReadSpecialValues(AVals);
}

int gdxSetSpecialValues(TGXFileRec_t *TGXFile, const double AVals[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSetSpecialValues(AVals);
}

int gdxSetTextNodeNr(TGXFileRec_t *TGXFile, int TxtNr, int Node) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSetTextNodeNr(TxtNr, Node);
}

int gdxSetTraceLevel(TGXFileRec_t *TGXFile, int N, const char *s) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSetTraceLevel(N, s);
}

int gdxSymbIndxMaxLength(TGXFileRec_t *TGXFile, int SyNr, int LengthInfo[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbIndxMaxLength(SyNr, LengthInfo);
}

int gdxSymbMaxLength(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbMaxLength();
}

int gdxSymbolAddComment(TGXFileRec_t *TGXFile, int SyNr, const char *Txt) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbolAddComment(SyNr, Txt);
}

int gdxSymbolGetComment(TGXFileRec_t *TGXFile, int SyNr, int N, char *Txt) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbolGetComment(SyNr, N, Txt);
}

int gdxSymbolGetDomain(TGXFileRec_t *TGXFile, int SyNr, int DomainSyNrs[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbolGetDomain(SyNr, DomainSyNrs);
}

int gdxSymbolGetDomainX(TGXFileRec_t *TGXFile, int SyNr, char *DomainIDs[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbolGetDomainX(SyNr, DomainIDs);
}

int gdxSymbolDim(TGXFileRec_t *TGXFile, int SyNr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbolDim(SyNr);
}

int gdxSymbolInfo(TGXFileRec_t *TGXFile, int SyNr, char *SyId, int *Dimen, int *Typ) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbolInfo(SyNr, SyId, *Dimen, *Typ);
}

int gdxSymbolInfoX(TGXFileRec_t *TGXFile, int SyNr, int *RecCnt, int *UserInfo, char *ExplTxt) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbolInfoX(SyNr, *RecCnt, *UserInfo, ExplTxt);
}

int gdxSymbolSetDomain(TGXFileRec_t *TGXFile, const char *DomainIDs[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbolSetDomain(DomainIDs);
}

int gdxSymbolSetDomainX(TGXFileRec_t *TGXFile, int SyNr, const char *DomainIDs[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbolSetDomainX(SyNr, DomainIDs);
}

int gdxSystemInfo(TGXFileRec_t *TGXFile, int *SyCnt, int *UelCnt) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSystemInfo(*SyCnt, *UelCnt);
}

int gdxUELMaxLength(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUELMaxLength();
}

int gdxUELRegisterDone(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUELRegisterDone();
}

int gdxUELRegisterMap(TGXFileRec_t *TGXFile, int UMap, const char *Uel) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUELRegisterMap(UMap, Uel);
}

int gdxUELRegisterMapStart(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUELRegisterMapStart();
}

int gdxUELRegisterRaw(TGXFileRec_t *TGXFile, const char *Uel) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUELRegisterRaw(Uel);
}

int gdxUELRegisterRawStart(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUELRegisterRawStart();
}

int gdxUELRegisterStr(TGXFileRec_t *TGXFile, const char *Uel, int *UelNr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUELRegisterStr(Uel, *UelNr);
}

int gdxUELRegisterStrStart(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUELRegisterStrStart();
}

int gdxUMFindUEL(TGXFileRec_t *TGXFile, const char *Uel, int *UelNr, int *UelMap) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUMFindUEL(Uel, *UelNr, *UelMap);
}

int gdxUMUelGet(TGXFileRec_t *TGXFile, int UelNr, char *Uel, int *UelMap) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUMUelGet(UelNr, Uel, *UelMap);
}

int gdxUMUelInfo(TGXFileRec_t *TGXFile, int *UelCnt, int *HighMap) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUMUelInfo(*UelCnt, *HighMap);
}

int gdxGetDomainElements(TGXFileRec_t *TGXFile, int SyNr, int DimPos, int FilterNr, ::TDomainIndexProc_t DP, int *NrElem, void *Uptr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxGetDomainElements(SyNr, DimPos, FilterNr, (gxfile::TDomainIndexProc_t)DP, *NrElem, Uptr);
}

int gdxCurrentDim(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxCurrentDim();
}

int gdxRenameUEL(TGXFileRec_t *TGXFile, const char *OldName, const char *NewName) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxRenameUEL(OldName, NewName);
}

int gdxStoreDomainSets(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxStoreDomainSets();
}

void gdxStoreDomainSetsSet(TGXFileRec_t *TGXFile, int x) {
    reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxStoreDomainSetsSet(x);
}

int gdxFree(TGXFileRec_t **TGXFile) {
    gdxDestroy(TGXFile);
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

// FIXME: For some reason using the 32-bit DLL (gdxcclib.dll) directly from Delphi fails close to here
void gdxCreateD(TGXFileRec_t **TGXFile, const char *sysDir, char *msgBuf, int msgBufLen) {
    // FIXME: Is this correct?
    doSetLoadPath(sysDir);
    gdxCreate(TGXFile, msgBuf, msgBufLen);
}

int gdxDataReadRawFast(TGXFileRec_t *TGXFile, int SyNr, ::TDataStoreProc_t DP, int *NrRecs) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadRawFast(SyNr, (gxfile::TDataStoreProc_t)DP, *NrRecs);
}

int gdxDataReadRawFastFilt(TGXFileRec_t *TGXFile, int SyNr, const char **UelFilterStr, ::TDataStoreFiltProc_t DP) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadRawFastFilt(SyNr, UelFilterStr, (gxfile::TDataStoreFiltProc_t)DP);
}

void setCallByRef(const char *FuncName, int cbrValue) {
    // FIXME: Actually do something!
    // ...
}

#ifdef PYGDX_EXPERIMENT
int gdx_set1d(TGXFileRec_t *pgx, const char *name, const char **elems) {
    auto obj = reinterpret_cast<gxfile::TGXFileObj *>(pgx);
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
