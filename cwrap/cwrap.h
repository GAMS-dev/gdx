#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
# define GDX_CALLCONV __stdcall
#else
# define GDX_CALLCONV
#endif

    typedef char gdxStrIndex_t[20][256];

    int create_gdx_file(const char *filename);

    int gdxFree(void **pgdx);
    int gdxGetReady(char *msgBuf, int msgBufLen);
    int gdxLibraryLoaded();
    int gdxLibraryUnload();

    int gdxCreate(void **pgdx, char *errBuf, int bufSize);
    void gdxDestroy(void **pgx);

    int gdx_set1d(void *pgx, const char *name, const char **elems);

    typedef struct TGXFileRec TGXFileRec_t;

    typedef void (*TDataStoreProc_t) (const int Indx[], const double Vals[]);
    typedef int (*TDataStoreFiltProc_t) (const int Indx[], const double Vals[], void *Uptr);
    typedef void (*TDomainIndexProc_t) (int RawIndex, int MappedIndex, void *Uptr);
    typedef int (*TDataStoreFiltProc_F_t) (const int Indx[], const double Vals[], long long *Uptr);
    typedef void (*TDomainIndexProc_F_t) (int *RawIndex, int *MappedIndex, void *Uptr);

    int gdxAcronymAdd(void *pgdx, const char *AName, const char *Txt, int AIndx);
    int gdxAcronymCount(void *pgdx);
    int gdxAcronymGetInfo(void *pgdx, int N, char *AName, char *Txt, int *AIndx);
    int gdxAcronymGetMapping(void *pgdx, int N, int *orgIndx, int *newIndx, int *autoIndex);
    int gdxAcronymIndex(void *pgdx, double V);
    int gdxAcronymName(void *pgdx, double V, char *AName);
    int gdxAcronymNextNr(void *pgdx, int NV);
    int gdxAcronymSetInfo(void *pgdx, int N, const char *AName, const char *Txt, int AIndx);
    double gdxAcronymValue(void *pgdx, int AIndx);
    int gdxAddAlias(void *pgdx, const char *Id1, const char *Id2);
    int gdxAddSetText(void *pgdx, const char *Txt, int *TxtNr);
    int gdxAutoConvert(void *pgdx, int NV);
    int gdxClose(void *pgdx);
    int gdxDataErrorCount(void *pgdx);
    int gdxDataErrorRecord(void *pgdx, int RecNr, int KeyInt[], double Values[]);
    int gdxDataErrorRecordX(void *pgdx, int RecNr, int KeyInt[], double Values[]);
    int gdxDataReadDone(void *pgdx);
    int gdxDataReadFilteredStart(void *pgdx, int SyNr, const int FilterAction[], int *NrRecs);
    int gdxDataReadMap(void *pgdx, int RecNr, int KeyInt[], double Values[], int *DimFrst);
    int gdxDataReadMapStart(void *pgdx, int SyNr, int *NrRecs);
    int gdxDataReadRaw(void *pgdx, int KeyInt[], double Values[], int *DimFrst);
    int gdxDataReadRawStart(void *pgdx, int SyNr, int *NrRecs);
    int gdxDataReadSlice(void *pgdx, const char *UelFilterStr[], int *Dimen, TDataStoreProc_t DP);
    int gdxDataReadSliceStart(void *pgdx, int SyNr, int ElemCounts[]);
    int gdxDataReadStr(void *pgdx, char *KeyStr[], double Values[], int *DimFrst);
    int gdxDataReadStrStart(void *pgdx, int SyNr, int *NrRecs);
    int gdxDataSliceUELS(void *pgdx, const int SliceKeyInt[], char *KeyStr[]);
    int gdxDataWriteDone(void *pgdx);
    int gdxDataWriteMap(void *pgdx, const int KeyInt[], const double Values[]);
    int gdxDataWriteMapStart(void *pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo);
    int gdxDataWriteRaw(void *pgdx, const int KeyInt[], const double Values[]);
    int gdxDataWriteRawStart(void *pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo);
    int gdxDataWriteStr(void *pgdx, const char *KeyStr[], const double Values[]);
    int gdxDataWriteStrStart(void *pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo);
    int gdxGetDLLVersion(void *pgdx, char *V);
    int gdxErrorCount(void *pgdx);
    int gdxErrorStr(void *pgdx, int ErrNr, char *ErrMsg);
    int gdxFileInfo(void *pgdx, int *FileVer, int *ComprLev);
    int gdxFileVersion(void *pgdx, char *FileStr, char *ProduceStr);
    int gdxFilterExists(void *pgdx, int FilterNr);
    int gdxFilterRegister(void *pgdx, int UelMap);
    int gdxFilterRegisterDone(void *pgdx);
    int gdxFilterRegisterStart(void *pgdx, int FilterNr);
    int gdxFindSymbol(void *pgdx, const char *SyId, int *SyNr);
    int gdxGetElemText(void *pgdx, int TxtNr, char *Txt, int *Node);
    int gdxGetLastError(void *pgdx);
    int gdxGetMemoryUsed(void *pgdx);
    int gdxGetSpecialValues(void *pgdx, double AVals[]);
    int gdxGetUEL(void *pgdx, int UelNr, char *Uel);
    int gdxMapValue(void *pgdx, double D, int *sv);
    int gdxOpenAppend(void *pgdx, const char *FileName, const char *Producer, int *ErrNr);
    int gdxOpenRead(void *pgdx, const char *FileName, int *ErrNr);
    int gdxOpenReadEx(void *pgdx, const char *FileName, int ReadMode, int *ErrNr);
    int gdxOpenWrite(void *pgdx, const char *FileName, const char *Producer, int *ErrNr);
    int gdxOpenWriteEx(void *pgdx, const char *FileName, const char *Producer, int Compr, int *ErrNr);
    int gdxResetSpecialValues(void *pgdx);
    int gdxSetHasText(void *pgdx, int SyNr);
    int gdxSetReadSpecialValues(void *pgdx, const double AVals[]);
    int gdxSetSpecialValues(void *pgdx, const double AVals[]);
    int gdxSetTextNodeNr(void *pgdx, int TxtNr, int Node);
    int gdxSetTraceLevel(void *pgdx, int N, const char *s);
    int gdxSymbIndxMaxLength(void *pgdx, int SyNr, int LengthInfo[]);
    int gdxSymbMaxLength(void *pgdx);
    int gdxSymbolAddComment(void *pgdx, int SyNr, const char *Txt);
    int gdxSymbolGetComment(void *pgdx, int SyNr, int N, char *Txt);
    int gdxSymbolGetDomain(void *pgdx, int SyNr, int DomainSyNrs[]);
    int gdxSymbolGetDomainX(void *pgdx, int SyNr, char *DomainIDs[]);
    int gdxSymbolDim(void *pgdx, int SyNr);
    int gdxSymbolInfo(void *pgdx, int SyNr, char *SyId, int *Dimen, int *Typ);
    int gdxSymbolInfoX(void *pgdx, int SyNr, int *RecCnt, int *UserInfo, char *ExplTxt);
    int gdxSymbolSetDomain(void *pgdx, const char *DomainIDs[]);
    int gdxSymbolSetDomainX(void *pgdx, int SyNr, const char *DomainIDs[]);
    int gdxSystemInfo(void *pgdx, int *SyCnt, int *UelCnt);
    int gdxUELMaxLength(void *pgdx);
    int gdxUELRegisterDone(void *pgdx);
    int gdxUELRegisterMap(void *pgdx, int UMap, const char *Uel);
    int gdxUELRegisterMapStart(void *pgdx);
    int gdxUELRegisterRaw(void *pgdx, const char *Uel);
    int gdxUELRegisterRawStart(void *pgdx);
    int gdxUELRegisterStr(void *pgdx, const char *Uel, int *UelNr);
    int gdxUELRegisterStrStart(void *pgdx);
    int gdxUMFindUEL(void *pgdx, const char *Uel, int *UelNr, int *UelMap);
    int gdxUMUelGet(void *pgdx, int UelNr, char *Uel, int *UelMap);
    int gdxUMUelInfo(void *pgdx, int *UelCnt, int *HighMap);
    int gdxGetDomainElements(void *pgdx, int SyNr, int DimPos, int FilterNr, TDomainIndexProc_t DP, int *NrElem, void *Uptr);
    int gdxCurrentDim(void *pgdx);
    int gdxRenameUEL(void *pgdx, const char *OldName, const char *NewName);
    int gdxStoreDomainSets(void *pgdx);
    void gdxStoreDomainSetsSet(void *pgdx, int x);

#ifdef __cplusplus
}
#endif
