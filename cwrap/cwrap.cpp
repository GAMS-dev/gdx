#include "cwrap.h"
#include "../gxfile.h"
#include <iostream>
#include "../utils.h"
#include <cassert>

using namespace gxfile;

extern "C" {

int gdxCreate(void **pgdx, char *errBuf, int bufSize) {
    std::string ErrMsg;
    auto *pgx = new TGXFileObj {ErrMsg};
    if(!ErrMsg.empty())
        memcpy(errBuf, ErrMsg.c_str(), std::min<int>((int)ErrMsg.length()+1, bufSize));
    else
        errBuf[0] = '\0';
    *pgdx = pgx;
    return true;
}

void gdxDestroy(void **pgx) {
    delete (TGXFileObj *)*pgx;
    *pgx = nullptr;
}

int gdxOpenWrite(void *pgx, const char *filename, const char *producer, int *ec) {
    return static_cast<TGXFileObj *>(pgx)->gdxOpenWrite(filename, producer, *ec);
}

int gdxOpenRead(void *pgx, const char *filename, int *ec) {
    return static_cast<TGXFileObj *>(pgx)->gdxOpenRead(filename, *ec);
}

int gdxClose(void *pgx) {
    return static_cast<TGXFileObj *>(pgx)->gdxClose();
}

int gdx_set1d(void *pgx, const char *name, const char **elems) {
    auto obj = static_cast<TGXFileObj *>(pgx);
    obj->gdxDataWriteStrStart(name, "A 1D set", 1, global::gmsspecs::dt_set, 0);
    gxdefs::TgdxStrIndex keyStrs {};
    gxdefs::TgdxValues values {};
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
    gdx.gdxDataWriteStrStart("i", "A simple set", 1, global::gmsspecs::dt_set, 0);
    gxdefs::TgdxValues vals{};
    gxdefs::TgdxStrIndex keys{};
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

int gdxOpenWriteEx(void *pgdx, const char *FileName, const char *Producer, int Compr, int *ErrNr) {
    return static_cast<TGXFileObj *>(pgdx)->gdxOpenWriteEx(FileName, Producer, Compr, *ErrNr);
}

int gdxDataWriteStrStart(void *pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo) {
    return static_cast<TGXFileObj *>(pgdx)->gdxDataWriteStrStart(SyId, ExplTxt, Dimen, Typ, UserInfo);
}

int gdxDataWriteRaw(void *pgdx, const int *KeyInt, const double *Values) {
    return static_cast<TGXFileObj *>(pgdx)->gdxDataWriteRaw(KeyInt, Values);
}


int gdxAcronymAdd(void *pgdx, const char *AName, const char *Txt, int AIndx) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxAcronymAdd(AName, Txt, AIndx);
    return 0;
}

int gdxAcronymCount(void *pgdx) {
    return static_cast<TGXFileObj *>(pgdx)->gdxAcronymCount();
}

int gdxAcronymGetInfo(void *pgdx, int N, char *AName, char *Txt, int *AIndx) {
    std::string sAName, sTxt;
    int rc{ static_cast<TGXFileObj *>(pgdx)->gdxAcronymGetInfo(N, sAName, sTxt, *AIndx) };
    utils::stocp(sAName, AName);
    utils::stocp(sTxt, Txt);
    return rc;
}

int gdxAcronymGetMapping(void *pgdx, int N, int *orgIndx, int *newIndx, int *autoIndex) {
    return static_cast<TGXFileObj *>(pgdx)->gdxAcronymGetMapping(N, *orgIndx, *newIndx, *autoIndex);
}

int gdxAcronymIndex(void *pgdx, double V) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxAcronymIndex(V);
    return 0;
}

int gdxAcronymName(void *pgdx, double V, char *AName) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxAcronymName(V, AName);
    return 0;
}

int gdxAcronymNextNr(void *pgdx, int NV) {
    return static_cast<TGXFileObj *>(pgdx)->gdxAcronymNextNr(NV);
}

int gdxAcronymSetInfo(void *pgdx, int N, const char *AName, const char *Txt, int AIndx) {
    return static_cast<TGXFileObj *>(pgdx)->gdxAcronymSetInfo(N, AName, Txt, AIndx);
}

double gdxAcronymValue(void *pgdx, int AIndx) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxAcronymValue(AIndx);
    return 0.0;
}

int gdxAddAlias(void *pgdx, const char *Id1, const char *Id2) {
    return static_cast<TGXFileObj *>(pgdx)->gdxAddAlias(Id1, Id2);
}

int gdxAddSetText(void *pgdx, const char *Txt, int *TxtNr) {
    return static_cast<TGXFileObj *>(pgdx)->gdxAddSetText(Txt, *TxtNr);
}

int gdxAutoConvert(void *pgdx, int NV) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxAutoConvert(NV);
    return 0;
}

int gdxDataErrorCount(void *pgdx) {
    return static_cast<TGXFileObj *>(pgdx)->gdxDataErrorCount();
}

int gdxDataErrorRecord(void *pgdx, int RecNr, int KeyInt[], double Values[]) {
    return static_cast<TGXFileObj *>(pgdx)->gdxDataErrorRecord(RecNr, KeyInt, Values);
}

int gdxDataErrorRecordX(void *pgdx, int RecNr, int KeyInt[], double Values[]) {
    return static_cast<TGXFileObj *>(pgdx)->gdxDataErrorRecordX(RecNr, KeyInt, Values);
}

int gdxDataReadDone(void *pgdx) {
    return static_cast<TGXFileObj *>(pgdx)->gdxDataReadDone();
}

int gdxDataReadFilteredStart(void *pgdx, int SyNr, const int FilterAction[], int *NrRecs) {
    return static_cast<TGXFileObj *>(pgdx)->gdxDataReadFilteredStart(SyNr, FilterAction, *NrRecs);
}

int gdxDataReadMap(void *pgdx, int RecNr, int KeyInt[], double Values[], int *DimFrst) {
    return static_cast<TGXFileObj *>(pgdx)->gdxDataReadMap(RecNr, KeyInt, Values, *DimFrst);
}

int gdxDataReadMapStart(void *pgdx, int SyNr, int *NrRecs) {
    return static_cast<TGXFileObj *>(pgdx)->gdxDataReadMapStart(SyNr, *NrRecs);
}

int gdxDataReadRaw(void *pgdx, int KeyInt[], double Values[], int *DimFrst) {
    return static_cast<TGXFileObj *>(pgdx)->gdxDataReadRaw(KeyInt, Values, *DimFrst);
}

int gdxDataReadRawStart(void *pgdx, int SyNr, int *NrRecs) {
    return static_cast<TGXFileObj *>(pgdx)->gdxDataReadRawStart(SyNr, *NrRecs);
}

int gdxDataReadSlice(void *pgdx, const char *UelFilterStr[], int *Dimen, TDataStoreProc_t DP) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxDataReadSlice(UelFilterStr, Dimen, DP);
    return 0;
}

int gdxDataReadSliceStart(void *pgdx, int SyNr, int ElemCounts[]) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxDataReadSliceStart(SyNr, ElemCounts);
    return 0;
}

int gdxDataReadStr(void *pgdx, char *KeyStr[], double Values[], int *DimFrst) {
    return static_cast<TGXFileObj *>(pgdx)->gdxDataReadStr(KeyStr, Values, *DimFrst);
}

int gdxDataReadStrStart(void *pgdx, int SyNr, int *NrRecs) {
    return static_cast<TGXFileObj *>(pgdx)->gdxDataReadStrStart(SyNr, *NrRecs);
}

int gdxDataSliceUELS(void *pgdx, const int SliceKeyInt[], char *KeyStr[]) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxDataSliceUELS(SliceKeyInt, KeyStr);
    return 0;
}

int gdxDataWriteDone(void *pgdx) {
    return static_cast<TGXFileObj *>(pgdx)->gdxDataWriteDone();
}

int gdxDataWriteMap(void *pgdx, const int KeyInt[], const double Values[]) {
    return static_cast<TGXFileObj *>(pgdx)->gdxDataWriteMap(KeyInt, Values);
}

int gdxDataWriteMapStart(void *pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo) {
    return static_cast<TGXFileObj *>(pgdx)->gdxDataWriteMapStart(SyId, ExplTxt, Dimen, Typ, UserInfo);
}

int gdxDataWriteRawStart(void *pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo) {
    return static_cast<TGXFileObj *>(pgdx)->gdxDataWriteRawStart(SyId, ExplTxt, Dimen, Typ, UserInfo);
}

int gdxDataWriteStr(void *pgdx, const char *KeyStr[], const double Values[]) {
    return static_cast<TGXFileObj *>(pgdx)->gdxDataWriteStr(KeyStr, Values);
}

int gdxGetDLLVersion(void *pgdx, char *V) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxGetDLLVersion(V);
    return 0;
}

int gdxErrorCount(void *pgdx) {
    return static_cast<TGXFileObj *>(pgdx)->gdxErrorCount();
}

int gdxErrorStr(void *pgdx, int ErrNr, char *ErrMsg) {
    std::string sErrMsg;
    int rc;
    if (pgdx) rc = static_cast<TGXFileObj*>(pgdx)->gdxErrorStr(ErrNr, sErrMsg);
    else rc = TGXFileObj::gdxErrorStrStatic(ErrNr, sErrMsg);
    utils::stocp(sErrMsg, ErrMsg);
    return rc;
}

int gdxFileInfo(void *pgdx, int *FileVer, int *ComprLev) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxFileInfo(FileVer, ComprLev);
    return 0;
}

int gdxFileVersion(void *pgdx, char *FileStr, char *ProduceStr) {
    std::string sFileStr, sProduceStr;
    int rc{ static_cast<TGXFileObj *>(pgdx)->gdxFileVersion(sFileStr, sProduceStr) };
    utils::stocp(sFileStr, FileStr);
    utils::stocp(sProduceStr, ProduceStr);
    return rc;
}

int gdxFilterExists(void *pgdx, int FilterNr) {
    return static_cast<TGXFileObj *>(pgdx)->gdxFilterExists(FilterNr);
}

int gdxFilterRegister(void *pgdx, int UelMap) {
    return static_cast<TGXFileObj *>(pgdx)->gdxFilterRegister(UelMap);
}

int gdxFilterRegisterDone(void *pgdx) {
    return static_cast<TGXFileObj *>(pgdx)->gdxFilterRegisterDone();
}

int gdxFilterRegisterStart(void *pgdx, int FilterNr) {
    return static_cast<TGXFileObj *>(pgdx)->gdxFilterRegisterStart(FilterNr);
}

int gdxFindSymbol(void *pgdx, const char *SyId, int *SyNr) {
    return static_cast<TGXFileObj *>(pgdx)->gdxFindSymbol(SyId, *SyNr);
}

int gdxGetElemText(void *pgdx, int TxtNr, char *Txt, int *Node) {
    std::string sTxt;
    int rc = static_cast<TGXFileObj *>(pgdx)->gdxGetElemText(TxtNr, sTxt, *Node);
    utils::stocp(sTxt, Txt);
    return rc;
}

int gdxGetLastError(void *pgdx) {
    return static_cast<TGXFileObj *>(pgdx)->gdxGetLastError();
}

int gdxGetMemoryUsed(void *pgdx) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxGetMemoryUsed();
    return 0;
}

int gdxGetSpecialValues(void *pgdx, double AVals[]) {
    std::array<double, 7> cppAVals{};
    int rc = static_cast<TGXFileObj *>(pgdx)->gdxGetSpecialValues(cppAVals);
    for(int i=0; i<cppAVals.size(); i++)
        AVals[i] = cppAVals[i];
    return rc;
}

int gdxGetUEL(void *pgdx, int UelNr, char *Uel) {
    std::string sUel;
    int rc = static_cast<TGXFileObj *>(pgdx)->gdxGetUEL(UelNr, sUel);
    utils::stocp(sUel, Uel);
    return rc;
}

int gdxMapValue(void *pgdx, double D, int *sv) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxMapValue(D, sv);
    return 0;
}

int gdxOpenAppend(void *pgdx, const char *FileName, const char *Producer, int *ErrNr) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxOpenAppend(FileName, Producer, ErrNr);
    return 0;
}

int gdxOpenReadEx(void *pgdx, const char *FileName, int ReadMode, int *ErrNr) {
    return static_cast<TGXFileObj *>(pgdx)->gdxOpenReadEx(FileName, ReadMode, *ErrNr);
}

int gdxResetSpecialValues(void *pgdx) {
    return static_cast<TGXFileObj *>(pgdx)->gdxResetSpecialValues();
}

int gdxSetHasText(void *pgdx, int SyNr) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxSetHasText(SyNr);
    return 0;
}

int gdxSetReadSpecialValues(void *pgdx, const double AVals[]) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxSetReadSpecialValues(AVals);
    return 0;
}

int gdxSetSpecialValues(void *pgdx, const double AVals[]) {
    std::array<double, 7> cppAVals {};
    std::memcpy(cppAVals.data(), AVals, cppAVals.size()*sizeof(double));
    return static_cast<TGXFileObj *>(pgdx)->gdxSetSpecialValues(cppAVals);
}

int gdxSetTextNodeNr(void *pgdx, int TxtNr, int Node) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxSetTextNodeNr(TxtNr, Node);
    return 0;
}

int gdxSetTraceLevel(void *pgdx, int N, const char *s) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxSetTraceLevel(N, s);
    return 0;
}

int gdxSymbIndxMaxLength(void *pgdx, int SyNr, int LengthInfo[]) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxSymbIndxMaxLength(SyNr, LengthInfo);
    return 0;
}

int gdxSymbMaxLength(void *pgdx) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxSymbMaxLength();
    return 0;
}

int gdxSymbolAddComment(void *pgdx, int SyNr, const char *Txt) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxSymbolAddComment(SyNr, Txt);
    return 0;
}

int gdxSymbolGetComment(void *pgdx, int SyNr, int N, char *Txt) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxSymbolGetComment(SyNr, N, Txt);
    return 0;
}

int gdxSymbolGetDomain(void *pgdx, int SyNr, int DomainSyNrs[]) {
    return static_cast<TGXFileObj *>(pgdx)->gdxSymbolGetDomain(SyNr, DomainSyNrs);
}

int gdxSymbolGetDomainX(void *pgdx, int SyNr, char *DomainIDs[]) {
    return static_cast<TGXFileObj *>(pgdx)->gdxSymbolGetDomainX(SyNr, DomainIDs);
}

int gdxSymbolDim(void *pgdx, int SyNr) {
    return static_cast<TGXFileObj *>(pgdx)->gdxSymbolDim(SyNr);
}

int gdxSymbolInfo(void *pgdx, int SyNr, char *SyId, int *Dimen, int *Typ) {
    std::string sSyId;
    int rc = static_cast<TGXFileObj *>(pgdx)->gdxSymbolInfo(SyNr, sSyId, *Dimen, *Typ);
    utils::stocp(sSyId, SyId);
    return rc;
}

int gdxSymbolInfoX(void *pgdx, int SyNr, int *RecCnt, int *UserInfo, char *ExplTxt) {
    std::string sExplTxt;
    int rc = static_cast<TGXFileObj *>(pgdx)->gdxSymbolInfoX(SyNr, *RecCnt, *UserInfo, sExplTxt);
    utils::stocp(sExplTxt, ExplTxt);
    return rc;
}

int gdxSymbolSetDomain(void *pgdx, const char *DomainIDs[]) {
    return static_cast<TGXFileObj *>(pgdx)->gdxSymbolSetDomain(DomainIDs);
}

int gdxSymbolSetDomainX(void *pgdx, int SyNr, const char *DomainIDs[]) {
    return static_cast<TGXFileObj *>(pgdx)->gdxSymbolSetDomainX(SyNr, DomainIDs);
}

int gdxSystemInfo(void *pgdx, int *SyCnt, int *UelCnt) {
    return static_cast<TGXFileObj *>(pgdx)->gdxSystemInfo(*SyCnt, *UelCnt);
}

int gdxUELMaxLength(void *pgdx) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxUELMaxLength();
    return 0;
}

int gdxUELRegisterDone(void *pgdx) {
    return static_cast<TGXFileObj *>(pgdx)->gdxUELRegisterDone();
}

int gdxUELRegisterMap(void *pgdx, int UMap, const char *Uel) {
    return static_cast<TGXFileObj *>(pgdx)->gdxUELRegisterMap(UMap, Uel);
}

int gdxUELRegisterMapStart(void *pgdx) {
    return static_cast<TGXFileObj *>(pgdx)->gdxUELRegisterMapStart();
}

int gdxUELRegisterRaw(void *pgdx, const char *Uel) {
    return static_cast<TGXFileObj *>(pgdx)->gdxUELRegisterRaw(Uel);
}

int gdxUELRegisterRawStart(void *pgdx) {
    return static_cast<TGXFileObj *>(pgdx)->gdxUELRegisterRawStart();
}

int gdxUELRegisterStr(void *pgdx, const char *Uel, int *UelNr) {
    return static_cast<TGXFileObj *>(pgdx)->gdxUELRegisterStr(Uel, *UelNr);
}

int gdxUELRegisterStrStart(void *pgdx) {
    return static_cast<TGXFileObj *>(pgdx)->gdxUELRegisterStrStart();
}

int gdxUMFindUEL(void *pgdx, const char *Uel, int *UelNr, int *UelMap) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxUMFindUEL(Uel, UelNr, UelMap);
    return 0;
}

int gdxUMUelGet(void *pgdx, int UelNr, char *Uel, int *UelMap) {
    std::string sUel;
    int rc = static_cast<TGXFileObj *>(pgdx)->gdxUMUelGet(UelNr, sUel, *UelMap);
    utils::stocp(sUel, Uel);
    return rc;
}

int gdxUMUelInfo(void *pgdx, int *UelCnt, int *HighMap) {
    return static_cast<TGXFileObj *>(pgdx)->gdxUMUelInfo(*UelCnt, *HighMap);
}

int gdxGetDomainElements(void *pgdx, int SyNr, int DimPos, int FilterNr, TDomainIndexProc_t DP, int *NrElem, void *Uptr) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxGetDomainElements(SyNr, DimPos, FilterNr, DP, NrElem, Uptr);
    return 0;
}

int gdxCurrentDim(void *pgdx) {
    return static_cast<TGXFileObj *>(pgdx)->gdxCurrentDim();
}

int gdxRenameUEL(void *pgdx, const char *OldName, const char *NewName) {
    return static_cast<TGXFileObj *>(pgdx)->gdxRenameUEL(OldName, NewName);
}

int gdxStoreDomainSets(void *pgdx) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxStoreDomainSets();
    return 0;
}

void gdxStoreDomainSetsSet(void *pgdx, int x) {
    //return static_cast<TGXFileObj *>(pgdx)->gdxStoreDomainSetsSet(x);
}

int gdxFree(void **pgdx) {
    gdxDestroy(pgdx);
    return 1;
}

int gdxGetReady(char *msgBuf, int msgBufLen) {
    STUBWARN();
    assert(msgBufLen > 0);
    msgBuf[0] = '\0';
    return 1;
}

int gdxLibraryLoaded() {
    STUBWARN();
    return 1;
}

int gdxLibraryUnload() {
    STUBWARN();
    return 1;
}

}






