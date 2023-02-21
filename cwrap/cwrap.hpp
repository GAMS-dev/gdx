#pragma once

#include "../gxfile.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
# define GDX_CALLCONV __stdcall
#else
# define GDX_CALLCONV
#endif

typedef struct TGXFileRec TGXFileRec_t;

typedef void (GDX_CALLCONV *TDataStoreProc_t) (const int Indx[], const double Vals[]);
typedef void (GDX_CALLCONV *TDataStoreProc_F_t) (const int Indx[], const double Vals[]);
typedef int (GDX_CALLCONV *TDataStoreFiltProc_t) (const int Indx[], const double Vals[], void *Uptr);
typedef void (GDX_CALLCONV *TDomainIndexProc_t) (int RawIndex, int MappedIndex, void *Uptr);
typedef int (GDX_CALLCONV *TDataStoreFiltProc_F_t) (const int Indx[], const double Vals[], long long *Uptr);
typedef void (GDX_CALLCONV *TDomainIndexProc_F_t) (int *RawIndex, int *MappedIndex, void *Uptr);

typedef void (GDX_CALLCONV *gdxSetLoadPath_t) (const char *s);
typedef void (GDX_CALLCONV *gdxGetLoadPath_t) (char *s);
extern gdxSetLoadPath_t gdxSetLoadPath;
extern gdxGetLoadPath_t gdxGetLoadPath;

typedef long long INT64;

#ifndef GDX_INLINE
#define GDX_INLINE inline
#endif

// PROTOTYPES BEGIN
void GDX_CALLCONV doSetLoadPath(const char *s);
void GDX_CALLCONV doGetLoadPath(char *s);
int gdxFree(TGXFileRec_t **pgdx);
int gdxGetReady(char *msgBuf, int msgBufLen);
int gdxLibraryLoaded();
int gdxLibraryUnload();
int gdxCreate(TGXFileRec_t **pgdx, char *errBuf, int bufSize);
void gdxCreateD(TGXFileRec_t **pgdx, const char *sysDir, char *msgBuf, int msgBufLen);
void gdxDestroy(TGXFileRec_t **pgx);
int gdxAcronymAdd(TGXFileRec_t *pgdx, const char *AName, const char *Txt, int AIndx);
int gdxAcronymCount(TGXFileRec_t *pgdx);
int gdxAcronymGetInfo(TGXFileRec_t *pgdx, int N, char *AName, char *Txt, int *AIndx);
int gdxAcronymGetMapping(TGXFileRec_t *pgdx, int N, int *orgIndx, int *newIndx, int *autoIndex);
int gdxAcronymIndex(TGXFileRec_t *pgdx, double V);
int gdxAcronymName(TGXFileRec_t *pgdx, double V, char *AName);
int gdxAcronymNextNr(TGXFileRec_t *pgdx, int NV);
int gdxAcronymSetInfo(TGXFileRec_t *pgdx, int N, const char *AName, const char *Txt, int AIndx);
double gdxAcronymValue(TGXFileRec_t *pgdx, int AIndx);
int gdxAddAlias(TGXFileRec_t *pgdx, const char *Id1, const char *Id2);
int gdxAddSetText(TGXFileRec_t *pgdx, const char *Txt, int *TxtNr);
int gdxAutoConvert(TGXFileRec_t *pgdx, int NV);
int gdxClose(TGXFileRec_t *pgdx);
int gdxDataErrorCount(TGXFileRec_t *pgdx);
int gdxDataErrorRecord(TGXFileRec_t *pgdx, int RecNr, int KeyInt[], double Values[]);
int gdxDataErrorRecordX(TGXFileRec_t *pgdx, int RecNr, int KeyInt[], double Values[]);
int gdxDataReadDone(TGXFileRec_t *pgdx);
int gdxDataReadFilteredStart(TGXFileRec_t *pgdx, int SyNr, const int FilterAction[], int *NrRecs);
int gdxDataReadMap(TGXFileRec_t *pgdx, int RecNr, int KeyInt[], double Values[], int *DimFrst);
int gdxDataReadMapStart(TGXFileRec_t *pgdx, int SyNr, int *NrRecs);
int gdxDataReadRaw(TGXFileRec_t *pgdx, int KeyInt[], double Values[], int *DimFrst);
int gdxDataReadRawStart(TGXFileRec_t *pgdx, int SyNr, int *NrRecs);
int gdxDataReadSlice(TGXFileRec_t *pgdx, const char *UelFilterStr[], int *Dimen, TDataStoreProc_t DP);
int gdxDataReadSliceStart(TGXFileRec_t *pgdx, int SyNr, int ElemCounts[]);
int gdxDataReadStr(TGXFileRec_t *pgdx, char *KeyStr[], double Values[], int *DimFrst);
int gdxDataReadStrStart(TGXFileRec_t *pgdx, int SyNr, int *NrRecs);
int gdxDataSliceUELS(TGXFileRec_t *pgdx, const int SliceKeyInt[], char *KeyStr[]);
int gdxDataWriteDone(TGXFileRec_t *pgdx);
int gdxDataWriteMap(TGXFileRec_t *pgdx, const int KeyInt[], const double Values[]);
int gdxDataWriteMapStart(TGXFileRec_t *pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo);
int gdxDataWriteRaw(TGXFileRec_t *pgdx, const int KeyInt[], const double Values[]);
int gdxDataWriteRawStart(TGXFileRec_t *pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo);
int gdxDataWriteStr(TGXFileRec_t *pgdx, const char *KeyStr[], const double Values[]);
int gdxDataWriteStrStart(TGXFileRec_t *pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo);
int gdxGetDLLVersion(TGXFileRec_t *pgdx, char *V);
int gdxErrorCount(TGXFileRec_t *pgdx);
int gdxErrorStr(TGXFileRec_t *pgdx, int ErrNr, char *ErrMsg);
int gdxFileInfo(TGXFileRec_t *pgdx, int *FileVer, int *ComprLev);
int gdxFileVersion(TGXFileRec_t *pgdx, char *FileStr, char *ProduceStr);
int gdxFilterExists(TGXFileRec_t *pgdx, int FilterNr);
int gdxFilterRegister(TGXFileRec_t *pgdx, int UelMap);
int gdxFilterRegisterDone(TGXFileRec_t *pgdx);
int gdxFilterRegisterStart(TGXFileRec_t *pgdx, int FilterNr);
int gdxFindSymbol(TGXFileRec_t *pgdx, const char *SyId, int *SyNr);
int gdxGetElemText(TGXFileRec_t *pgdx, int TxtNr, char *Txt, int *Node);
int gdxGetLastError(TGXFileRec_t *pgdx);
int gdxGetMemoryUsed(TGXFileRec_t *pgdx);
int gdxGetSpecialValues(TGXFileRec_t *pgdx, double AVals[]);
int gdxGetUEL(TGXFileRec_t *pgdx, int UelNr, char *Uel);
int gdxMapValue(TGXFileRec_t *pgdx, double D, int *sv);
int gdxOpenAppend(TGXFileRec_t *pgdx, const char *FileName, const char *Producer, int *ErrNr);
int gdxOpenRead(TGXFileRec_t *pgdx, const char *FileName, int *ErrNr);
int gdxOpenReadEx(TGXFileRec_t *pgdx, const char *FileName, int ReadMode, int *ErrNr);
int gdxOpenWrite(TGXFileRec_t *pgdx, const char *FileName, const char *Producer, int *ErrNr);
int gdxOpenWriteEx(TGXFileRec_t *pgdx, const char *FileName, const char *Producer, int Compr, int *ErrNr);
int gdxResetSpecialValues(TGXFileRec_t *pgdx);
int gdxSetHasText(TGXFileRec_t *pgdx, int SyNr);
int gdxSetReadSpecialValues(TGXFileRec_t *pgdx, const double AVals[]);
int gdxSetSpecialValues(TGXFileRec_t *pgdx, const double AVals[]);
int gdxSetTextNodeNr(TGXFileRec_t *pgdx, int TxtNr, int Node);
int gdxSetTraceLevel(TGXFileRec_t *pgdx, int N, const char *s);
int gdxSymbIndxMaxLength(TGXFileRec_t *pgdx, int SyNr, int LengthInfo[]);
int gdxSymbMaxLength(TGXFileRec_t *pgdx);
int gdxSymbolAddComment(TGXFileRec_t *pgdx, int SyNr, const char *Txt);
int gdxSymbolGetComment(TGXFileRec_t *pgdx, int SyNr, int N, char *Txt);
int gdxSymbolGetDomain(TGXFileRec_t *pgdx, int SyNr, int DomainSyNrs[]);
int gdxSymbolGetDomainX(TGXFileRec_t *pgdx, int SyNr, char *DomainIDs[]);
int gdxSymbolDim(TGXFileRec_t *pgdx, int SyNr);
int gdxSymbolInfo(TGXFileRec_t *pgdx, int SyNr, char *SyId, int *Dimen, int *Typ);
int gdxSymbolInfoX(TGXFileRec_t *pgdx, int SyNr, int *RecCnt, int *UserInfo, char *ExplTxt);
int gdxSymbolSetDomain(TGXFileRec_t *pgdx, const char *DomainIDs[]);
int gdxSymbolSetDomainX(TGXFileRec_t *pgdx, int SyNr, const char *DomainIDs[]);
int gdxSystemInfo(TGXFileRec_t *pgdx, int *SyCnt, int *UelCnt);
int gdxUELMaxLength(TGXFileRec_t *pgdx);
int gdxUELRegisterDone(TGXFileRec_t *pgdx);
int gdxUELRegisterMap(TGXFileRec_t *pgdx, int UMap, const char *Uel);
int gdxUELRegisterMapStart(TGXFileRec_t *pgdx);
int gdxUELRegisterRaw(TGXFileRec_t *pgdx, const char *Uel);
int gdxUELRegisterRawStart(TGXFileRec_t *pgdx);
int gdxUELRegisterStr(TGXFileRec_t *pgdx, const char *Uel, int *UelNr);
int gdxUELRegisterStrStart(TGXFileRec_t *pgdx);
int gdxUMFindUEL(TGXFileRec_t *pgdx, const char *Uel, int *UelNr, int *UelMap);
int gdxUMUelGet(TGXFileRec_t *pgdx, int UelNr, char *Uel, int *UelMap);
int gdxUMUelInfo(TGXFileRec_t *pgdx, int *UelCnt, int *HighMap);
int gdxGetDomainElements(TGXFileRec_t *pgdx, int SyNr, int DimPos, int FilterNr, TDomainIndexProc_t DP, int *NrElem, void *Uptr);
int gdxCurrentDim(TGXFileRec_t *pgdx);
int gdxRenameUEL(TGXFileRec_t *pgdx, const char *OldName, const char *NewName);
int gdxStoreDomainSets(TGXFileRec_t *pgdx);
void gdxStoreDomainSetsSet(TGXFileRec_t *pgdx, int x);
int gdxDataReadRawFast(TGXFileRec_t *TGXFile, int SyNr, TDataStoreProc_t DP, int *NrRecs);
int gdxDataReadRawFastFilt(TGXFileRec_t *TGXFile, int SyNr, const char *UelFilterStr[], TDataStoreFiltProc_t DP);
void setCallByRef(const char *FuncName, int cbrValue);
// PROTOTYPES END

GDX_INLINE void GDX_CALLCONV doSetLoadPath(const char *s) {
    gxfile::DLLLoadPath.assign(s);
}

GDX_INLINE void GDX_CALLCONV doGetLoadPath(char *s) {
    assert(gxfile::DLLLoadPath.size() < 256);
    memcpy(s, gxfile::DLLLoadPath.c_str(), gxfile::DLLLoadPath.size());
}

gdxSetLoadPath_t gdxSetLoadPath = doSetLoadPath;
gdxGetLoadPath_t gdxGetLoadPath = doGetLoadPath;

GDX_INLINE int gdxCreate(TGXFileRec_t **TGXFile, char *errBuf, int bufSize) {
    std::string ErrMsg;
    auto *pgx = new gxfile::TGXFileObj {ErrMsg};
    if(!ErrMsg.empty())
        memcpy(errBuf, ErrMsg.c_str(), std::min<int>((int)ErrMsg.length()+1, bufSize));
    else
        errBuf[0] = '\0';
    *TGXFile = reinterpret_cast<TGXFileRec_t *>(pgx);
    return true;
}

GDX_INLINE void gdxDestroy(TGXFileRec_t **pgx) {
    delete (gxfile::TGXFileObj *)*pgx;
    *pgx = nullptr;
}

GDX_INLINE int gdxOpenWrite(TGXFileRec_t *pgx, const char *filename, const char *producer, int *ec) {
    return reinterpret_cast<gxfile::TGXFileObj *>(pgx)->gdxOpenWrite(filename, producer, *ec);
}

GDX_INLINE int gdxOpenRead(TGXFileRec_t *pgx, const char *filename, int *ec) {
    return reinterpret_cast<gxfile::TGXFileObj *>(pgx)->gdxOpenRead(filename, *ec);
}

GDX_INLINE int gdxClose(TGXFileRec_t *pgx) {
    return reinterpret_cast<gxfile::TGXFileObj *>(pgx)->gdxClose();
}

GDX_INLINE int gdxOpenWriteEx(TGXFileRec_t *TGXFile, const char *FileName, const char *Producer, int Compr, int *ErrNr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxOpenWriteEx(FileName, Producer, Compr, *ErrNr);
}

GDX_INLINE int gdxDataWriteStrStart(TGXFileRec_t *TGXFile, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataWriteStrStart(SyId, ExplTxt, Dimen, Typ, UserInfo);
}

GDX_INLINE int gdxDataWriteRaw(TGXFileRec_t *TGXFile, const int *KeyInt, const double *Values) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataWriteRaw(KeyInt, Values);
}


GDX_INLINE int gdxAcronymAdd(TGXFileRec_t *TGXFile, const char *AName, const char *Txt, int AIndx) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAcronymAdd(AName, Txt, AIndx);
}

GDX_INLINE int gdxAcronymCount(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAcronymCount();
}

GDX_INLINE int gdxAcronymGetInfo(TGXFileRec_t *TGXFile, int N, char *AName, char *Txt, int *AIndx) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAcronymGetInfo(N, AName, Txt, *AIndx);
}

GDX_INLINE int gdxAcronymGetMapping(TGXFileRec_t *TGXFile, int N, int *orgIndx, int *newIndx, int *autoIndex) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAcronymGetMapping(N, *orgIndx, *newIndx, *autoIndex);
}

GDX_INLINE int gdxAcronymIndex(TGXFileRec_t *TGXFile, double V) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAcronymIndex(V);
}

GDX_INLINE int gdxAcronymName(TGXFileRec_t *TGXFile, double V, char *AName) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAcronymName(V, AName);
}

GDX_INLINE int gdxAcronymNextNr(TGXFileRec_t *TGXFile, int NV) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAcronymNextNr(NV);
}

GDX_INLINE int gdxAcronymSetInfo(TGXFileRec_t *TGXFile, int N, const char *AName, const char *Txt, int AIndx) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAcronymSetInfo(N, AName, Txt, AIndx);
}

GDX_INLINE double gdxAcronymValue(TGXFileRec_t *TGXFile, int AIndx) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAcronymValue(AIndx);
}

GDX_INLINE int gdxAddAlias(TGXFileRec_t *TGXFile, const char *Id1, const char *Id2) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAddAlias(Id1, Id2);
}

GDX_INLINE int gdxAddSetText(TGXFileRec_t *TGXFile, const char *Txt, int *TxtNr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAddSetText(Txt, *TxtNr);
}

GDX_INLINE int gdxAutoConvert(TGXFileRec_t *TGXFile, int NV) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxAutoConvert(NV);
}

GDX_INLINE int gdxDataErrorCount(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataErrorCount();
}

GDX_INLINE int gdxDataErrorRecord(TGXFileRec_t *TGXFile, int RecNr, int KeyInt[], double Values[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataErrorRecord(RecNr, KeyInt, Values);
}

GDX_INLINE int gdxDataErrorRecordX(TGXFileRec_t *TGXFile, int RecNr, int KeyInt[], double Values[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataErrorRecordX(RecNr, KeyInt, Values);
}

GDX_INLINE int gdxDataReadDone(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadDone();
}

GDX_INLINE int gdxDataReadFilteredStart(TGXFileRec_t *TGXFile, int SyNr, const int FilterAction[], int *NrRecs) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadFilteredStart(SyNr, FilterAction, *NrRecs);
}

GDX_INLINE int gdxDataReadMap(TGXFileRec_t *TGXFile, int RecNr, int KeyInt[], double Values[], int *DimFrst) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadMap(RecNr, KeyInt, Values, *DimFrst);
}

GDX_INLINE int gdxDataReadMapStart(TGXFileRec_t *TGXFile, int SyNr, int *NrRecs) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadMapStart(SyNr, *NrRecs);
}

GDX_INLINE int gdxDataReadRaw(TGXFileRec_t *TGXFile, int KeyInt[], double Values[], int *DimFrst) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadRaw(KeyInt, Values, *DimFrst);
}

GDX_INLINE int gdxDataReadRawStart(TGXFileRec_t *TGXFile, int SyNr, int *NrRecs) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadRawStart(SyNr, *NrRecs);
}

GDX_INLINE int gdxDataReadSlice(TGXFileRec_t *TGXFile, const char *UelFilterStr[], int *Dimen, ::TDataStoreProc_t DP) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadSlice(UelFilterStr, *Dimen, (gdxinterface::TDataStoreProc_t)DP);
}

GDX_INLINE int gdxDataReadSliceStart(TGXFileRec_t *TGXFile, int SyNr, int ElemCounts[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadSliceStart(SyNr, ElemCounts);
}

GDX_INLINE int gdxDataReadStr(TGXFileRec_t *TGXFile, char *KeyStr[], double Values[], int *DimFrst) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadStr(KeyStr, Values, *DimFrst);
}

GDX_INLINE int gdxDataReadStrStart(TGXFileRec_t *TGXFile, int SyNr, int *NrRecs) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadStrStart(SyNr, *NrRecs);
}

GDX_INLINE int gdxDataSliceUELS(TGXFileRec_t *TGXFile, const int SliceKeyInt[], char *KeyStr[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataSliceUELS(SliceKeyInt, KeyStr);
}

GDX_INLINE int gdxDataWriteDone(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataWriteDone();
}

GDX_INLINE int gdxDataWriteMap(TGXFileRec_t *TGXFile, const int KeyInt[], const double Values[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataWriteMap(KeyInt, Values);
}

GDX_INLINE int gdxDataWriteMapStart(TGXFileRec_t *TGXFile, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataWriteMapStart(SyId, ExplTxt, Dimen, Typ, UserInfo);
}

GDX_INLINE int gdxDataWriteRawStart(TGXFileRec_t *TGXFile, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataWriteRawStart(SyId, ExplTxt, Dimen, Typ, UserInfo);
}

GDX_INLINE int gdxDataWriteStr(TGXFileRec_t *TGXFile, const char *KeyStr[], const double Values[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataWriteStr(KeyStr, Values);
}

GDX_INLINE int gdxGetDLLVersion(TGXFileRec_t *TGXFile, char *V) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxGetDLLVersion(V);
}

GDX_INLINE int gdxErrorCount(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxErrorCount();
}

GDX_INLINE int gdxErrorStr(TGXFileRec_t *TGXFile, int ErrNr, char *ErrMsg) {
    return TGXFile ? reinterpret_cast<gxfile::TGXFileObj*>(TGXFile)->gdxErrorStr(ErrNr, ErrMsg) : gxfile::TGXFileObj::gdxErrorStrStatic(ErrNr, ErrMsg);
}

GDX_INLINE int gdxFileInfo(TGXFileRec_t *TGXFile, int *FileVer, int *ComprLev) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxFileInfo(*FileVer, *ComprLev);
}

GDX_INLINE int gdxFileVersion(TGXFileRec_t *TGXFile, char *FileStr, char *ProduceStr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxFileVersion(FileStr, ProduceStr);
}

GDX_INLINE int gdxFilterExists(TGXFileRec_t *TGXFile, int FilterNr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxFilterExists(FilterNr);
}

GDX_INLINE int gdxFilterRegister(TGXFileRec_t *TGXFile, int UelMap) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxFilterRegister(UelMap);
}

GDX_INLINE int gdxFilterRegisterDone(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxFilterRegisterDone();
}

GDX_INLINE int gdxFilterRegisterStart(TGXFileRec_t *TGXFile, int FilterNr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxFilterRegisterStart(FilterNr);
}

GDX_INLINE int gdxFindSymbol(TGXFileRec_t *TGXFile, const char *SyId, int *SyNr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxFindSymbol(SyId, *SyNr);
}

GDX_INLINE int gdxGetElemText(TGXFileRec_t *TGXFile, int TxtNr, char *Txt, int *Node) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxGetElemText(TxtNr, Txt, *Node);
}

GDX_INLINE int gdxGetLastError(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxGetLastError();
}

GDX_INLINE int gdxGetMemoryUsed(TGXFileRec_t *TGXFile) {
    return (int)reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxGetMemoryUsed();
}

GDX_INLINE int gdxGetSpecialValues(TGXFileRec_t *TGXFile, double AVals[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxGetSpecialValues(AVals);
}

GDX_INLINE int gdxGetUEL(TGXFileRec_t *TGXFile, int UelNr, char *Uel) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxGetUEL(UelNr, Uel);
}

GDX_INLINE int gdxMapValue(TGXFileRec_t *TGXFile, double D, int *sv) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxMapValue(D, *sv);
}

GDX_INLINE int gdxOpenAppend(TGXFileRec_t *TGXFile, const char *FileName, const char *Producer, int *ErrNr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxOpenAppend(FileName, Producer, *ErrNr);
}

GDX_INLINE int gdxOpenReadEx(TGXFileRec_t *TGXFile, const char *FileName, int ReadMode, int *ErrNr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxOpenReadEx(FileName, ReadMode, *ErrNr);
}

GDX_INLINE int gdxResetSpecialValues(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxResetSpecialValues();
}

GDX_INLINE int gdxSetHasText(TGXFileRec_t *TGXFile, int SyNr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSetHasText(SyNr);
}

GDX_INLINE int gdxSetReadSpecialValues(TGXFileRec_t *TGXFile, const double AVals[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSetReadSpecialValues(AVals);
}

GDX_INLINE int gdxSetSpecialValues(TGXFileRec_t *TGXFile, const double AVals[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSetSpecialValues(AVals);
}

GDX_INLINE int gdxSetTextNodeNr(TGXFileRec_t *TGXFile, int TxtNr, int Node) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSetTextNodeNr(TxtNr, Node);
}

GDX_INLINE int gdxSetTraceLevel(TGXFileRec_t *TGXFile, int N, const char *s) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSetTraceLevel(N, s);
}

GDX_INLINE int gdxSymbIndxMaxLength(TGXFileRec_t *TGXFile, int SyNr, int LengthInfo[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbIndxMaxLength(SyNr, LengthInfo);
}

GDX_INLINE int gdxSymbMaxLength(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbMaxLength();
}

GDX_INLINE int gdxSymbolAddComment(TGXFileRec_t *TGXFile, int SyNr, const char *Txt) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbolAddComment(SyNr, Txt);
}

GDX_INLINE int gdxSymbolGetComment(TGXFileRec_t *TGXFile, int SyNr, int N, char *Txt) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbolGetComment(SyNr, N, Txt);
}

GDX_INLINE int gdxSymbolGetDomain(TGXFileRec_t *TGXFile, int SyNr, int DomainSyNrs[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbolGetDomain(SyNr, DomainSyNrs);
}

GDX_INLINE int gdxSymbolGetDomainX(TGXFileRec_t *TGXFile, int SyNr, char *DomainIDs[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbolGetDomainX(SyNr, DomainIDs);
}

GDX_INLINE int gdxSymbolDim(TGXFileRec_t *TGXFile, int SyNr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbolDim(SyNr);
}

GDX_INLINE int gdxSymbolInfo(TGXFileRec_t *TGXFile, int SyNr, char *SyId, int *Dimen, int *Typ) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbolInfo(SyNr, SyId, *Dimen, *Typ);
}

GDX_INLINE int gdxSymbolInfoX(TGXFileRec_t *TGXFile, int SyNr, int *RecCnt, int *UserInfo, char *ExplTxt) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbolInfoX(SyNr, *RecCnt, *UserInfo, ExplTxt);
}

GDX_INLINE int gdxSymbolSetDomain(TGXFileRec_t *TGXFile, const char *DomainIDs[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbolSetDomain(DomainIDs);
}

GDX_INLINE int gdxSymbolSetDomainX(TGXFileRec_t *TGXFile, int SyNr, const char *DomainIDs[]) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSymbolSetDomainX(SyNr, DomainIDs);
}

GDX_INLINE int gdxSystemInfo(TGXFileRec_t *TGXFile, int *SyCnt, int *UelCnt) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxSystemInfo(*SyCnt, *UelCnt);
}

GDX_INLINE int gdxUELMaxLength(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUELMaxLength();
}

GDX_INLINE int gdxUELRegisterDone(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUELRegisterDone();
}

GDX_INLINE int gdxUELRegisterMap(TGXFileRec_t *TGXFile, int UMap, const char *Uel) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUELRegisterMap(UMap, Uel);
}

GDX_INLINE int gdxUELRegisterMapStart(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUELRegisterMapStart();
}

GDX_INLINE int gdxUELRegisterRaw(TGXFileRec_t *TGXFile, const char *Uel) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUELRegisterRaw(Uel);
}

GDX_INLINE int gdxUELRegisterRawStart(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUELRegisterRawStart();
}

GDX_INLINE int gdxUELRegisterStr(TGXFileRec_t *TGXFile, const char *Uel, int *UelNr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUELRegisterStr(Uel, *UelNr);
}

GDX_INLINE int gdxUELRegisterStrStart(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUELRegisterStrStart();
}

GDX_INLINE int gdxUMFindUEL(TGXFileRec_t *TGXFile, const char *Uel, int *UelNr, int *UelMap) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUMFindUEL(Uel, *UelNr, *UelMap);
}

GDX_INLINE int gdxUMUelGet(TGXFileRec_t *TGXFile, int UelNr, char *Uel, int *UelMap) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUMUelGet(UelNr, Uel, *UelMap);
}

GDX_INLINE int gdxUMUelInfo(TGXFileRec_t *TGXFile, int *UelCnt, int *HighMap) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxUMUelInfo(*UelCnt, *HighMap);
}

GDX_INLINE int gdxGetDomainElements(TGXFileRec_t *TGXFile, int SyNr, int DimPos, int FilterNr, ::TDomainIndexProc_t DP, int *NrElem, void *Uptr) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxGetDomainElements(SyNr, DimPos, FilterNr, (gdxinterface::TDomainIndexProc_t)DP, *NrElem, Uptr);
}

GDX_INLINE int gdxCurrentDim(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxCurrentDim();
}

GDX_INLINE int gdxRenameUEL(TGXFileRec_t *TGXFile, const char *OldName, const char *NewName) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxRenameUEL(OldName, NewName);
}

GDX_INLINE int gdxStoreDomainSets(TGXFileRec_t *TGXFile) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxStoreDomainSets();
}

GDX_INLINE void gdxStoreDomainSetsSet(TGXFileRec_t *TGXFile, int x) {
    reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxStoreDomainSetsSet(x);
}

GDX_INLINE int gdxFree(TGXFileRec_t **TGXFile) {
    gdxDestroy(TGXFile);
    return 1;
}

GDX_INLINE int gdxGetReady(char *msgBuf, int msgBufLen) {
    // FIXME: Is this enough?
    assert(msgBufLen > 0);
    msgBuf[0] = '\0';
    return 1;
}

GDX_INLINE int gdxLibraryLoaded() {
    // FIXME: Is this enough?
    return 1;
}

GDX_INLINE int gdxLibraryUnload() {
    // FIXME: Is this enough?
    return 1;
}

GDX_INLINE void gdxCreateD(TGXFileRec_t **TGXFile, const char *sysDir, char *msgBuf, int msgBufLen) {
    // FIXME: Is this correct?
    doSetLoadPath(sysDir);
    gdxCreate(TGXFile, msgBuf, msgBufLen);
}

GDX_INLINE int gdxDataReadRawFast(TGXFileRec_t *TGXFile, int SyNr, ::TDataStoreProc_t DP, int *NrRecs) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadRawFast(SyNr, (gdxinterface::TDataStoreProc_t)DP, *NrRecs);
}

GDX_INLINE int gdxDataReadRawFastFilt(TGXFileRec_t *TGXFile, int SyNr, const char **UelFilterStr, ::TDataStoreFiltProc_t DP) {
    return reinterpret_cast<gxfile::TGXFileObj *>(TGXFile)->gdxDataReadRawFastFilt(SyNr, UelFilterStr, (gdxinterface::TDataStoreFiltProc_t)DP);
}

void setCallByRef(const char *FuncName, int cbrValue) {
    // FIXME: Actually do something!
    // ...
}

#ifdef __cplusplus
}
#endif