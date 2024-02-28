/*
 * GAMS - General Algebraic Modeling System GDX API
 *
 * Copyright (c) 2017-2024 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2024 GAMS Development Corp. <support@gams.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// Implement C-API via calling TGXFile C++ class

#pragma once

#include "gdx.h"    // for TGXFileObj, DLLLoadPath, TDataStoreProc_t, TDat...
#include <algorithm>// for min
#include <cassert>  // for assert
#include <cstring>  // for memcpy
#include <string>   // for string

#ifdef __cplusplus
extern "C" {
#endif

#if defined( _WIN32 )
#define GDX_CALLCONV __stdcall
#else
#define GDX_CALLCONV
#endif

typedef struct TGXFileRec TGXFileRec_t;
typedef TGXFileRec_t* gdxHandle_t;

typedef void( GDX_CALLCONV *TDataStoreProc_t )( const int Indx[], const double Vals[] );
typedef int( GDX_CALLCONV *TDataStoreExProc_t )( const int Indx[], const double Vals[], int DimFrst, void *Uptr );
typedef int( GDX_CALLCONV *TDataStoreExProc_F_t )( const int Indx[], const double Vals[], int afdim, long long Uptr );
typedef void( GDX_CALLCONV *TDataStoreProc_F_t )( const int Indx[], const double Vals[] );
typedef int( GDX_CALLCONV *TDataStoreFiltProc_t )( const int Indx[], const double Vals[], void *Uptr );
typedef void( GDX_CALLCONV *TDomainIndexProc_t )( int RawIndex, int MappedIndex, void *Uptr );
typedef int( GDX_CALLCONV *TDataStoreFiltProc_F_t )( const int Indx[], const double Vals[], long long *Uptr );
typedef void( GDX_CALLCONV *TDomainIndexProc_F_t )( int *RawIndex, int *MappedIndex, void *Uptr );

typedef void( GDX_CALLCONV *gdxSetLoadPath_t )( const char *s );
typedef void( GDX_CALLCONV *gdxGetLoadPath_t )( char *s );
extern gdxSetLoadPath_t gdxSetLoadPath;
extern gdxGetLoadPath_t gdxGetLoadPath;

#if defined(_WIN32)
typedef __int64 INT64;
#else
typedef signed long int INT64;
#endif

#ifndef GDX_INLINE
#define GDX_INLINE inline
#endif

// PROTOTYPES BEGIN
int gdxAcronymAdd( TGXFileRec_t *pgdx, const char *AName, const char *Txt, int AIndx );
int gdxAcronymCount( TGXFileRec_t *pgdx);
int gdxAcronymGetInfo( TGXFileRec_t *pgdx, int N, char *AName, char *Txt, int *AIndx );
int gdxAcronymGetMapping( TGXFileRec_t *pgdx, int N, int *orgIndx, int *newIndx, int *autoIndex );
int gdxAcronymIndex( TGXFileRec_t *pgdx, double V );
int gdxAcronymName( TGXFileRec_t *pgdx, double V, char *AName );
int gdxAcronymNextNr( TGXFileRec_t *pgdx, int NV );
int gdxAcronymSetInfo( TGXFileRec_t *pgdx, int N, const char *AName, const char *Txt, int AIndx );
double gdxAcronymValue( TGXFileRec_t *pgdx, int AIndx );
int gdxAddAlias( TGXFileRec_t *pgdx, const char *Id1, const char *Id2 );
int gdxAddSetText( TGXFileRec_t *pgdx, const char *Txt, int *TxtNr );
int gdxAutoConvert( TGXFileRec_t *pgdx, int NV );
int gdxClose( TGXFileRec_t *pgdx);
int gdxDataErrorCount( TGXFileRec_t *pgdx);
int gdxDataErrorRecord( TGXFileRec_t *pgdx, int RecNr, int *KeyInt, double *Values );
int gdxDataErrorRecordX( TGXFileRec_t *pgdx, int RecNr, int *KeyInt, double *Values );
int gdxDataReadDone( TGXFileRec_t *pgdx);
int gdxDataReadFilteredStart( TGXFileRec_t *pgdx, int SyNr, const int *FilterAction, int *NrRecs );
int gdxDataReadMap( TGXFileRec_t *pgdx, int RecNr, int *KeyInt, double *Values, int *DimFrst );
int gdxDataReadMapStart( TGXFileRec_t *pgdx, int SyNr, int *NrRecs );
int gdxDataReadRaw( TGXFileRec_t *pgdx, int *KeyInt, double *Values, int *DimFrst );
int gdxDataReadRawFast( TGXFileRec_t *pgdx, int SyNr, TDataStoreProc_t DP, int *NrRecs );
int gdxDataReadRawFastEx( TGXFileRec_t *pgdx, int SyNr, TDataStoreExProc_t DP, int *NrRecs, void *Uptr );
int gdxDataReadRawFastFilt( TGXFileRec_t *pgdx, int SyNr, const char **UelFilterStr, TDataStoreFiltProc_t DP );
int gdxDataReadRawStart( TGXFileRec_t *pgdx, int SyNr, int *NrRecs );
int gdxDataReadSlice( TGXFileRec_t *pgdx, const char **UelFilterStr, int *Dimen, TDataStoreProc_t DP );
int gdxDataReadSliceStart( TGXFileRec_t *pgdx, int SyNr, int *ElemCounts );
int gdxDataReadStr( TGXFileRec_t *pgdx, char **KeyStr, double *Values, int *DimFrst );
int gdxDataReadStrStart( TGXFileRec_t *pgdx, int SyNr, int *NrRecs );
int gdxDataSliceUELS( TGXFileRec_t *pgdx, const int *SliceKeyInt, char **KeyStr );
int gdxDataWriteDone( TGXFileRec_t *pgdx);
int gdxDataWriteMap( TGXFileRec_t *pgdx, const int *KeyInt, const double *Values );
int gdxDataWriteMapStart( TGXFileRec_t *pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo );
int gdxDataWriteRaw( TGXFileRec_t *pgdx, const int *KeyInt, const double *Values );
int gdxDataWriteRawStart( TGXFileRec_t *pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo );
int gdxDataWriteRawStartKeyBounds( TGXFileRec_t *pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo, const int *MinUELIndices, const int *MaxUELIndices );
int gdxDataWriteStr( TGXFileRec_t *pgdx, const char **KeyStr, const double *Values );
int gdxDataWriteStrStart( TGXFileRec_t *pgdx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo );
int gdxGetDLLVersion( TGXFileRec_t *pgdx, char *V );
int gdxErrorCount( TGXFileRec_t *pgdx);
int gdxErrorStr( TGXFileRec_t *pgdx, int ErrNr, char *ErrMsg );
int gdxFileInfo( TGXFileRec_t *pgdx, int *FileVer, int *ComprLev );
int gdxFileVersion( TGXFileRec_t *pgdx, char *FileStr, char *ProduceStr );
int gdxFilterExists( TGXFileRec_t *pgdx, int FilterNr );
int gdxFilterRegister( TGXFileRec_t *pgdx, int UelMap );
int gdxFilterRegisterDone( TGXFileRec_t *pgdx);
int gdxFilterRegisterStart( TGXFileRec_t *pgdx, int FilterNr );
int gdxFindSymbol( TGXFileRec_t *pgdx, const char *SyId, int *SyNr );
int gdxGetElemText( TGXFileRec_t *pgdx, int TxtNr, char *Txt, int *Node );
int gdxGetLastError( TGXFileRec_t *pgdx);
int gdxGetMemoryUsed( TGXFileRec_t *pgdx);
int gdxGetSpecialValues( TGXFileRec_t *pgdx, double *AVals );
int gdxGetUEL( TGXFileRec_t *pgdx, int UelNr, char *Uel );
int gdxMapValue( TGXFileRec_t *pgdx, double D, int *sv );
int gdxOpenAppend( TGXFileRec_t *pgdx, const char *FileName, const char *Producer, int *ErrNr );
int gdxOpenRead( TGXFileRec_t *pgdx, const char *FileName, int *ErrNr );
int gdxOpenReadEx( TGXFileRec_t *pgdx, const char *FileName, int ReadMode, int *ErrNr );
int gdxOpenWrite( TGXFileRec_t *pgdx, const char *FileName, const char *Producer, int *ErrNr );
int gdxOpenWriteEx( TGXFileRec_t *pgdx, const char *FileName, const char *Producer, int Compr, int *ErrNr );
int gdxResetSpecialValues( TGXFileRec_t *pgdx);
int gdxSetHasText( TGXFileRec_t *pgdx, int SyNr );
int gdxSetReadSpecialValues( TGXFileRec_t *pgdx, const double *AVals );
int gdxSetSpecialValues( TGXFileRec_t *pgdx, const double *AVals );
int gdxSetTextNodeNr( TGXFileRec_t *pgdx, int TxtNr, int Node );
int gdxSetTraceLevel( TGXFileRec_t *pgdx, int N, const char *s );
int gdxSymbIndxMaxLength( TGXFileRec_t *pgdx, int SyNr, int *LengthInfo );
int gdxSymbMaxLength( TGXFileRec_t *pgdx);
int gdxSymbolAddComment( TGXFileRec_t *pgdx, int SyNr, const char *Txt );
int gdxSymbolGetComment( TGXFileRec_t *pgdx, int SyNr, int N, char *Txt );
int gdxSymbolGetDomain( TGXFileRec_t *pgdx, int SyNr, int *DomainSyNrs );
int gdxSymbolGetDomainX( TGXFileRec_t *pgdx, int SyNr, char **DomainIDs );
int gdxSymbolDim( TGXFileRec_t *pgdx, int SyNr );
int gdxSymbolInfo( TGXFileRec_t *pgdx, int SyNr, char *SyId, int *Dimen, int *Typ );
int gdxSymbolInfoX( TGXFileRec_t *pgdx, int SyNr, int *RecCnt, int *UserInfo, char *ExplTxt );
int gdxSymbolSetDomain( TGXFileRec_t *pgdx, const char **DomainIDs );
int gdxSymbolSetDomainX( TGXFileRec_t *pgdx, int SyNr, const char **DomainIDs );
int gdxSystemInfo( TGXFileRec_t *pgdx, int *SyCnt, int *UelCnt );
int gdxUELMaxLength( TGXFileRec_t *pgdx);
int gdxUELRegisterDone( TGXFileRec_t *pgdx);
int gdxUELRegisterMap( TGXFileRec_t *pgdx, int UMap, const char *Uel );
int gdxUELRegisterMapStart( TGXFileRec_t *pgdx);
int gdxUELRegisterRaw( TGXFileRec_t *pgdx, const char *Uel );
int gdxUELRegisterRawStart( TGXFileRec_t *pgdx);
int gdxUELRegisterStr( TGXFileRec_t *pgdx, const char *Uel, int *UelNr );
int gdxUELRegisterStrStart( TGXFileRec_t *pgdx);
int gdxUMFindUEL( TGXFileRec_t *pgdx, const char *Uel, int *UelNr, int *UelMap );
int gdxUMUelGet( TGXFileRec_t *pgdx, int UelNr, char *Uel, int *UelMap );
int gdxUMUelInfo( TGXFileRec_t *pgdx, int *UelCnt, int *HighMap );
int gdxGetDomainElements( TGXFileRec_t *pgdx, int SyNr, int DimPos, int FilterNr, TDomainIndexProc_t DP, int *NrElem, void *Uptr );
int gdxCurrentDim( TGXFileRec_t *pgdx);
int gdxRenameUEL( TGXFileRec_t *pgdx, const char *OldName, const char *NewName );

void GDX_CALLCONV doSetLoadPath( const char *s );
void GDX_CALLCONV doGetLoadPath( char *s );
int gdxFree( TGXFileRec_t **pgdx );
int gdxCreate( TGXFileRec_t **pgdx, char *errBuf, int bufSize );
int gdxCreateD( TGXFileRec_t **pgdx, const char *sysDir, char *msgBuf, int msgBufLen );
void gdxDestroy( TGXFileRec_t **pgx );
int gdxStoreDomainSets( TGXFileRec_t *pgdx);
void gdxStoreDomainSetsSet( TGXFileRec_t *pgdx, int flag);
int gdxAllowBogusDomains( TGXFileRec_t *pgdx);
void gdxAllowBogusDomainsSet( TGXFileRec_t *pgdx, int flag);
int gdxMapAcronymsToNaN( TGXFileRec_t *pgdx);
void gdxMapAcronymsToNaNSet( TGXFileRec_t *pgdx, int flag);
void setCallByRef( TGXFileRec_t *TGXFile, const char *FuncName, int cbrValue );
// PROTOTYPES END

GDX_INLINE void GDX_CALLCONV doSetLoadPath( const char *s )
{
   gdx::DLLLoadPath.assign( s );
}

GDX_INLINE void GDX_CALLCONV doGetLoadPath( char *s )
{
   assert( gdx::DLLLoadPath.size() < 256 );
   memcpy( s, gdx::DLLLoadPath.c_str(), gdx::DLLLoadPath.size() );
}

#ifndef NO_SET_LOAD_PATH_DEF
gdxSetLoadPath_t gdxSetLoadPath = doSetLoadPath;
gdxGetLoadPath_t gdxGetLoadPath = doGetLoadPath;
#endif

GDX_INLINE int gdxCreate( TGXFileRec_t **TGXFile, char *errBuf, int bufSize )
{
   std::string ErrMsg;
   auto *pgx = new gdx::TGXFileObj { ErrMsg };
   if( !ErrMsg.empty() )
      memcpy( errBuf, ErrMsg.c_str(), std::min<int>( (int) ErrMsg.length() + 1, bufSize ) );
   else
      errBuf[0] = '\0';
   *TGXFile = reinterpret_cast<TGXFileRec_t *>( pgx );
   return true;
}

GDX_INLINE void gdxDestroy( TGXFileRec_t **pgx )
{
   delete(gdx::TGXFileObj *) *pgx;
   *pgx = nullptr;
}
GDX_INLINE int gdxAcronymAdd( TGXFileRec_t *pgx, const char *AName, const char *Txt, int AIndx )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxAcronymAdd(AName, Txt, AIndx );
}

GDX_INLINE int gdxAcronymCount( TGXFileRec_t *pgx)
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxAcronymCount();
}

GDX_INLINE int gdxAcronymGetInfo( TGXFileRec_t *pgx, int N, char *AName, char *Txt, int *AIndx )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxAcronymGetInfo(N, AName, Txt, *AIndx );
}

GDX_INLINE int gdxAcronymGetMapping( TGXFileRec_t *pgx, int N, int *orgIndx, int *newIndx, int *autoIndex )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxAcronymGetMapping(N, *orgIndx, *newIndx, *autoIndex );
}

GDX_INLINE int gdxAcronymIndex( TGXFileRec_t *pgx, double V )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxAcronymIndex(V );
}

GDX_INLINE int gdxAcronymName( TGXFileRec_t *pgx, double V, char *AName )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxAcronymName(V, AName );
}

GDX_INLINE int gdxAcronymNextNr( TGXFileRec_t *pgx, int NV )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxAcronymNextNr(NV );
}

GDX_INLINE int gdxAcronymSetInfo( TGXFileRec_t *pgx, int N, const char *AName, const char *Txt, int AIndx )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxAcronymSetInfo(N, AName, Txt, AIndx );
}

GDX_INLINE double gdxAcronymValue( TGXFileRec_t *pgx, int AIndx )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxAcronymValue(AIndx );
}

GDX_INLINE int gdxAddAlias( TGXFileRec_t *pgx, const char *Id1, const char *Id2 )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxAddAlias(Id1, Id2 );
}

GDX_INLINE int gdxAddSetText( TGXFileRec_t *pgx, const char *Txt, int *TxtNr )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxAddSetText(Txt, *TxtNr );
}

GDX_INLINE int gdxAutoConvert( TGXFileRec_t *pgx, int NV )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxAutoConvert(NV );
}

GDX_INLINE int gdxClose( TGXFileRec_t *pgx)
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxClose();
}

GDX_INLINE int gdxDataErrorCount( TGXFileRec_t *pgx)
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataErrorCount();
}

GDX_INLINE int gdxDataErrorRecord( TGXFileRec_t *pgx, int RecNr, int *KeyInt, double *Values )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataErrorRecord(RecNr, KeyInt, Values );
}

GDX_INLINE int gdxDataErrorRecordX( TGXFileRec_t *pgx, int RecNr, int *KeyInt, double *Values )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataErrorRecordX(RecNr, KeyInt, Values );
}

GDX_INLINE int gdxDataReadDone( TGXFileRec_t *pgx)
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataReadDone();
}

GDX_INLINE int gdxDataReadFilteredStart( TGXFileRec_t *pgx, int SyNr, const int *FilterAction, int *NrRecs )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataReadFilteredStart(SyNr, FilterAction, *NrRecs );
}

GDX_INLINE int gdxDataReadMap( TGXFileRec_t *pgx, int RecNr, int *KeyInt, double *Values, int *DimFrst )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataReadMap(RecNr, KeyInt, Values, *DimFrst );
}

GDX_INLINE int gdxDataReadMapStart( TGXFileRec_t *pgx, int SyNr, int *NrRecs )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataReadMapStart(SyNr, *NrRecs );
}

GDX_INLINE int gdxDataReadRaw( TGXFileRec_t *pgx, int *KeyInt, double *Values, int *DimFrst )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataReadRaw(KeyInt, Values, *DimFrst );
}

GDX_INLINE int gdxDataReadRawFast( TGXFileRec_t *pgx, int SyNr, ::TDataStoreProc_t DP, int *NrRecs )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataReadRawFast(SyNr, (gdx::TDataStoreProc_t) DP, *NrRecs );
}

GDX_INLINE int gdxDataReadRawFastEx( TGXFileRec_t *pgx, int SyNr, ::TDataStoreExProc_t DP, int *NrRecs, void *Uptr )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataReadRawFastEx(SyNr, (gdx::TDataStoreExProc_t) DP, *NrRecs, Uptr );
}

GDX_INLINE int gdxDataReadRawFastFilt( TGXFileRec_t *pgx, int SyNr, const char **UelFilterStr, ::TDataStoreFiltProc_t DP )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataReadRawFastFilt(SyNr, UelFilterStr, (gdx::TDataStoreFiltProc_t) DP );
}

GDX_INLINE int gdxDataReadRawStart( TGXFileRec_t *pgx, int SyNr, int *NrRecs )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataReadRawStart(SyNr, *NrRecs );
}

GDX_INLINE int gdxDataReadSlice( TGXFileRec_t *pgx, const char **UelFilterStr, int *Dimen, ::TDataStoreProc_t DP )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataReadSlice(UelFilterStr, *Dimen, (gdx::TDataStoreProc_t) DP );
}

GDX_INLINE int gdxDataReadSliceStart( TGXFileRec_t *pgx, int SyNr, int *ElemCounts )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataReadSliceStart(SyNr, ElemCounts );
}

GDX_INLINE int gdxDataReadStr( TGXFileRec_t *pgx, char **KeyStr, double *Values, int *DimFrst )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataReadStr(KeyStr, Values, *DimFrst );
}

GDX_INLINE int gdxDataReadStrStart( TGXFileRec_t *pgx, int SyNr, int *NrRecs )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataReadStrStart(SyNr, *NrRecs );
}

GDX_INLINE int gdxDataSliceUELS( TGXFileRec_t *pgx, const int *SliceKeyInt, char **KeyStr )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataSliceUELS(SliceKeyInt, KeyStr );
}

GDX_INLINE int gdxDataWriteDone( TGXFileRec_t *pgx)
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataWriteDone();
}

GDX_INLINE int gdxDataWriteMap( TGXFileRec_t *pgx, const int *KeyInt, const double *Values )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataWriteMap(KeyInt, Values );
}

GDX_INLINE int gdxDataWriteMapStart( TGXFileRec_t *pgx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataWriteMapStart(SyId, ExplTxt, Dimen, Typ, UserInfo );
}

GDX_INLINE int gdxDataWriteRaw( TGXFileRec_t *pgx, const int *KeyInt, const double *Values )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataWriteRaw(KeyInt, Values );
}

GDX_INLINE int gdxDataWriteRawStart( TGXFileRec_t *pgx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataWriteRawStart(SyId, ExplTxt, Dimen, Typ, UserInfo );
}

GDX_INLINE int gdxDataWriteRawStartKeyBounds( TGXFileRec_t *pgx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo, const int *MinUELIndices, const int *MaxUELIndices )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataWriteRawStartKeyBounds(SyId, ExplTxt, Dimen, Typ, UserInfo, MinUELIndices, MaxUELIndices );
}

GDX_INLINE int gdxDataWriteStr( TGXFileRec_t *pgx, const char **KeyStr, const double *Values )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataWriteStr(KeyStr, Values );
}

GDX_INLINE int gdxDataWriteStrStart( TGXFileRec_t *pgx, const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxDataWriteStrStart(SyId, ExplTxt, Dimen, Typ, UserInfo );
}

GDX_INLINE int gdxGetDLLVersion( TGXFileRec_t *pgx, char *V )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxGetDLLVersion(V );
}

GDX_INLINE int gdxErrorCount( TGXFileRec_t *pgx)
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxErrorCount();
}

GDX_INLINE int gdxErrorStr( TGXFileRec_t *pgx, int ErrNr, char *ErrMsg )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxErrorStr(ErrNr, ErrMsg );
}

GDX_INLINE int gdxFileInfo( TGXFileRec_t *pgx, int *FileVer, int *ComprLev )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxFileInfo(*FileVer, *ComprLev );
}

GDX_INLINE int gdxFileVersion( TGXFileRec_t *pgx, char *FileStr, char *ProduceStr )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxFileVersion(FileStr, ProduceStr );
}

GDX_INLINE int gdxFilterExists( TGXFileRec_t *pgx, int FilterNr )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxFilterExists(FilterNr );
}

GDX_INLINE int gdxFilterRegister( TGXFileRec_t *pgx, int UelMap )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxFilterRegister(UelMap );
}

GDX_INLINE int gdxFilterRegisterDone( TGXFileRec_t *pgx)
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxFilterRegisterDone();
}

GDX_INLINE int gdxFilterRegisterStart( TGXFileRec_t *pgx, int FilterNr )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxFilterRegisterStart(FilterNr );
}

GDX_INLINE int gdxFindSymbol( TGXFileRec_t *pgx, const char *SyId, int *SyNr )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxFindSymbol(SyId, *SyNr );
}

GDX_INLINE int gdxGetElemText( TGXFileRec_t *pgx, int TxtNr, char *Txt, int *Node )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxGetElemText(TxtNr, Txt, *Node );
}

GDX_INLINE int gdxGetLastError( TGXFileRec_t *pgx)
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxGetLastError();
}

GDX_INLINE int gdxGetMemoryUsed( TGXFileRec_t *pgx)
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxGetMemoryUsed();
}

GDX_INLINE int gdxGetSpecialValues( TGXFileRec_t *pgx, double *AVals )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxGetSpecialValues(AVals );
}

GDX_INLINE int gdxGetUEL( TGXFileRec_t *pgx, int UelNr, char *Uel )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxGetUEL(UelNr, Uel );
}

GDX_INLINE int gdxMapValue( TGXFileRec_t *pgx, double D, int *sv )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxMapValue(D, *sv );
}

GDX_INLINE int gdxOpenAppend( TGXFileRec_t *pgx, const char *FileName, const char *Producer, int *ErrNr )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxOpenAppend(FileName, Producer, *ErrNr );
}

GDX_INLINE int gdxOpenRead( TGXFileRec_t *pgx, const char *FileName, int *ErrNr )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxOpenRead(FileName, *ErrNr );
}

GDX_INLINE int gdxOpenReadEx( TGXFileRec_t *pgx, const char *FileName, int ReadMode, int *ErrNr )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxOpenReadEx(FileName, ReadMode, *ErrNr );
}

GDX_INLINE int gdxOpenWrite( TGXFileRec_t *pgx, const char *FileName, const char *Producer, int *ErrNr )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxOpenWrite(FileName, Producer, *ErrNr );
}

GDX_INLINE int gdxOpenWriteEx( TGXFileRec_t *pgx, const char *FileName, const char *Producer, int Compr, int *ErrNr )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxOpenWriteEx(FileName, Producer, Compr, *ErrNr );
}

GDX_INLINE int gdxResetSpecialValues( TGXFileRec_t *pgx)
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxResetSpecialValues();
}

GDX_INLINE int gdxSetHasText( TGXFileRec_t *pgx, int SyNr )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxSetHasText(SyNr );
}

GDX_INLINE int gdxSetReadSpecialValues( TGXFileRec_t *pgx, const double *AVals )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxSetReadSpecialValues(AVals );
}

GDX_INLINE int gdxSetSpecialValues( TGXFileRec_t *pgx, const double *AVals )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxSetSpecialValues(AVals );
}

GDX_INLINE int gdxSetTextNodeNr( TGXFileRec_t *pgx, int TxtNr, int Node )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxSetTextNodeNr(TxtNr, Node );
}

GDX_INLINE int gdxSetTraceLevel( TGXFileRec_t *pgx, int N, const char *s )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxSetTraceLevel(N, s );
}

GDX_INLINE int gdxSymbIndxMaxLength( TGXFileRec_t *pgx, int SyNr, int *LengthInfo )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxSymbIndxMaxLength(SyNr, LengthInfo );
}

GDX_INLINE int gdxSymbMaxLength( TGXFileRec_t *pgx)
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxSymbMaxLength();
}

GDX_INLINE int gdxSymbolAddComment( TGXFileRec_t *pgx, int SyNr, const char *Txt )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxSymbolAddComment(SyNr, Txt );
}

GDX_INLINE int gdxSymbolGetComment( TGXFileRec_t *pgx, int SyNr, int N, char *Txt )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxSymbolGetComment(SyNr, N, Txt );
}

GDX_INLINE int gdxSymbolGetDomain( TGXFileRec_t *pgx, int SyNr, int *DomainSyNrs )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxSymbolGetDomain(SyNr, DomainSyNrs );
}

GDX_INLINE int gdxSymbolGetDomainX( TGXFileRec_t *pgx, int SyNr, char **DomainIDs )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxSymbolGetDomainX(SyNr, DomainIDs );
}

GDX_INLINE int gdxSymbolDim( TGXFileRec_t *pgx, int SyNr )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxSymbolDim(SyNr );
}

GDX_INLINE int gdxSymbolInfo( TGXFileRec_t *pgx, int SyNr, char *SyId, int *Dimen, int *Typ )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxSymbolInfo(SyNr, SyId, *Dimen, *Typ );
}

GDX_INLINE int gdxSymbolInfoX( TGXFileRec_t *pgx, int SyNr, int *RecCnt, int *UserInfo, char *ExplTxt )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxSymbolInfoX(SyNr, *RecCnt, *UserInfo, ExplTxt );
}

GDX_INLINE int gdxSymbolSetDomain( TGXFileRec_t *pgx, const char **DomainIDs )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxSymbolSetDomain(DomainIDs );
}

GDX_INLINE int gdxSymbolSetDomainX( TGXFileRec_t *pgx, int SyNr, const char **DomainIDs )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxSymbolSetDomainX(SyNr, DomainIDs );
}

GDX_INLINE int gdxSystemInfo( TGXFileRec_t *pgx, int *SyCnt, int *UelCnt )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxSystemInfo(*SyCnt, *UelCnt );
}

GDX_INLINE int gdxUELMaxLength( TGXFileRec_t *pgx)
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxUELMaxLength();
}

GDX_INLINE int gdxUELRegisterDone( TGXFileRec_t *pgx)
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxUELRegisterDone();
}

GDX_INLINE int gdxUELRegisterMap( TGXFileRec_t *pgx, int UMap, const char *Uel )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxUELRegisterMap(UMap, Uel );
}

GDX_INLINE int gdxUELRegisterMapStart( TGXFileRec_t *pgx)
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxUELRegisterMapStart();
}

GDX_INLINE int gdxUELRegisterRaw( TGXFileRec_t *pgx, const char *Uel )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxUELRegisterRaw(Uel );
}

GDX_INLINE int gdxUELRegisterRawStart( TGXFileRec_t *pgx)
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxUELRegisterRawStart();
}

GDX_INLINE int gdxUELRegisterStr( TGXFileRec_t *pgx, const char *Uel, int *UelNr )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxUELRegisterStr(Uel, *UelNr );
}

GDX_INLINE int gdxUELRegisterStrStart( TGXFileRec_t *pgx)
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxUELRegisterStrStart();
}

GDX_INLINE int gdxUMFindUEL( TGXFileRec_t *pgx, const char *Uel, int *UelNr, int *UelMap )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxUMFindUEL(Uel, *UelNr, *UelMap );
}

GDX_INLINE int gdxUMUelGet( TGXFileRec_t *pgx, int UelNr, char *Uel, int *UelMap )
{
   if(!UelMap)
   {
     int tmpUelMap;
     return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxUMUelGet(UelNr, Uel, tmpUelMap );
   }
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxUMUelGet(UelNr, Uel, *UelMap );
}

GDX_INLINE int gdxUMUelInfo( TGXFileRec_t *pgx, int *UelCnt, int *HighMap )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxUMUelInfo(*UelCnt, *HighMap );
}

GDX_INLINE int gdxGetDomainElements( TGXFileRec_t *pgx, int SyNr, int DimPos, int FilterNr, ::TDomainIndexProc_t DP, int *NrElem, void *Uptr )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxGetDomainElements(SyNr, DimPos, FilterNr, (gdx::TDomainIndexProc_t) DP, *NrElem, Uptr );
}

GDX_INLINE int gdxCurrentDim( TGXFileRec_t *pgx)
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxCurrentDim();
}

GDX_INLINE int gdxRenameUEL( TGXFileRec_t *pgx, const char *OldName, const char *NewName )
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxRenameUEL(OldName, NewName );
}


GDX_INLINE int gdxStoreDomainSets( TGXFileRec_t *pgx)
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxStoreDomainSets();
}

GDX_INLINE void gdxStoreDomainSetsSet( TGXFileRec_t *pgx, int flag)
{
   reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxStoreDomainSetsSet( flag );
}

GDX_INLINE int gdxAllowBogusDomains( TGXFileRec_t *pgx)
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxAllowBogusDomains();
}

GDX_INLINE void gdxAllowBogusDomainsSet( TGXFileRec_t *pgx, int flag)
{
   reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxAllowBogusDomainsSet( flag );
}

GDX_INLINE int gdxMapAcronymsToNaN( TGXFileRec_t *pgx)
{
   return reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxMapAcronymsToNaN();
}

GDX_INLINE void gdxMapAcronymsToNaNSet( TGXFileRec_t *pgx, int flag)
{
   reinterpret_cast<gdx::TGXFileObj *>( pgx )->gdxMapAcronymsToNaNSet( flag );
}

GDX_INLINE int gdxFree( TGXFileRec_t **TGXFile )
{
   gdxDestroy( TGXFile );
   return 1;
}

GDX_INLINE int gdxCreateD( TGXFileRec_t **TGXFile, const char *sysDir, char *msgBuf, int msgBufLen )
{
   doSetLoadPath( sysDir );
   return gdxCreate( TGXFile, msgBuf, msgBufLen );
}

GDX_INLINE void setCallByRef( TGXFileRec_t *TGXFile, const char *FuncName, int cbrValue )
{
   const auto obj = reinterpret_cast<gdx::TGXFileObj *>( TGXFile );
   if( !std::strcmp( FuncName, "gdxDataReadRawFastEx_DP" ) )
      obj->gdxDataReadRawFastEx_DP_CallByRef = cbrValue;
   else if( !std::strcmp( FuncName, "gdxDataReadRawFastFilt_DP" ) )
      obj->gdxDataReadRawFastFilt_DP_CallByRef = cbrValue;
   else if( !std::strcmp( FuncName, "gdxGetDomainElements_DP" ) )
      obj->gdxGetDomainElements_DP_CallByRef = cbrValue;
}

#ifdef __cplusplus
}
#endif
