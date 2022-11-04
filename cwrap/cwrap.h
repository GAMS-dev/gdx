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

    int create_gdx_file(const char *filename);

    int gdxFree(TGXFileRec_t **pgdx);
    int gdxGetReady(char *msgBuf, int msgBufLen);
    int gdxLibraryLoaded();
    int gdxLibraryUnload();

    int gdxCreate(TGXFileRec_t **pgdx, char *errBuf, int bufSize);
    void gdxCreateD(TGXFileRec_t **pgdx, const char *sysDir, char *msgBuf, int msgBufLen);
    void gdxDestroy(TGXFileRec_t **pgx);

    int gdx_set1d(TGXFileRec_t *pgx, const char *name, const char **elems);

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

#ifdef __cplusplus
}
#endif
