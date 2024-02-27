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

// Implement TGXFile C++ class via calling C-API

#pragma once

// This is a drop-in replacement for gdx.h
// in order to make gxfiletests.cpp also useful
// for testing the wrapped GDX DLL

#include <string>
#include <iostream>
#include <array>
#include "gdxcc.h"

namespace gdx
{

using TDomainIndexProc_t = void ( * )( int RawIndex, int MappedIndex, void *Uptr );
using TDataStoreProc_t = void ( * )( const int *Indx, const double *Vals );
using TDataStoreFiltProc_t = int ( * )( const int *Indx, const double *Vals, void *Uptr );
using TDataStoreExProc_t = int ( * )( const int *Indx, const double *Vals, const int afdim, void *Uptr );

using TgdxUELIndex = std::array<int, GMS_MAX_INDEX_DIM>;
using TgdxValues = std::array<double, GMS_VAL_SCALE + 1>;

enum TgdxIntlValTyp
{// values stored internally via the indicator byte
   vm_valund,
   vm_valna,
   vm_valpin,
   vm_valmin,
   vm_valeps,
   vm_zero,
   vm_one,
   vm_mone,
   vm_half,
   vm_two,
   vm_normal,
   vm_count
};

constexpr int DOMC_UNMAPPED = -2,// indicator for unmapped index pos
        DOMC_EXPAND = -1,        // indicator growing index pos
        DOMC_STRICT = 0;         // indicator mapped index pos

// Description:
//    Class for reading and writing gdx files
class TGXFileObj
{
   gdxHandle_t pgx {};

public:
   enum class TraceLevels
   {
      trl_none,
      trl_errors,
      trl_some,
      trl_all
   };

   explicit TGXFileObj( std::string &ErrMsg )
   {
      char buf[GMS_SSSIZE];
      if( !::gdxLibraryLoaded() && !::gdxGetReady( buf, GMS_SSSIZE ) )
         throw std::runtime_error( buf );
      if( !::gdxCreate( &pgx, buf, GMS_SSSIZE ) )
         throw std::runtime_error( buf );
      ErrMsg.assign( buf );
   }

   ~TGXFileObj()
   {
      if( pgx )
         ::gdxFree( &pgx );
      ::gdxLibraryUnload();
   }


   [[nodiscard]] int gdxStoreDomainSets() const
   {
     return ::gdxStoreDomainSets( pgx );
   }

   void gdxStoreDomainSetsSet(int flag)
   {
     ::gdxStoreDomainSetsSet( pgx, flag );
   }

   [[nodiscard]] int gdxAllowBogusDomains() const
   {
     return ::gdxAllowBogusDomains( pgx );
   }

   void gdxAllowBogusDomainsSet(int flag)
   {
     ::gdxAllowBogusDomainsSet( pgx, flag );
   }

   [[nodiscard]] int gdxMapAcronymsToNaN() const
   {
     return ::gdxMapAcronymsToNaN( pgx );
   }

   void gdxMapAcronymsToNaNSet(int flag)
   {
     ::gdxMapAcronymsToNaNSet( pgx, flag );
   }

   int gdxAcronymAdd( const char *AName, const char *Txt, int AIndx )
   {
     return ::gdxAcronymAdd( pgx, AName, Txt, AIndx );
   }

   int gdxAcronymCount() const
   {
     return ::gdxAcronymCount( pgx );
   }

   int gdxAcronymGetInfo( int N, char *AName, char *Txt, int &AIndx ) const
   {
     return ::gdxAcronymGetInfo( pgx, N, AName, Txt, &AIndx );
   }

   int gdxAcronymGetMapping( int N, int &orgIndx, int &newIndx, int &autoIndex )
   {
     return ::gdxAcronymGetMapping( pgx, N, &orgIndx, &newIndx, &autoIndex );
   }

   int gdxAcronymIndex( double V ) const
   {
     return ::gdxAcronymIndex( pgx, V );
   }

   int gdxAcronymName( double V, char *AName )
   {
     return ::gdxAcronymName( pgx, V, AName );
   }

   int gdxAcronymNextNr( int NV )
   {
     return ::gdxAcronymNextNr( pgx, NV );
   }

   int gdxAcronymSetInfo( int N, const char *AName, const char *Txt, int AIndx )
   {
     return ::gdxAcronymSetInfo( pgx, N, AName, Txt, AIndx );
   }

   double gdxAcronymValue( int AIndx ) const
   {
     return ::gdxAcronymValue( pgx, AIndx );
   }

   int gdxAddAlias( const char *Id1, const char *Id2 )
   {
     return ::gdxAddAlias( pgx, Id1, Id2 );
   }

   int gdxAddSetText( const char *Txt, int &TxtNr )
   {
     return ::gdxAddSetText( pgx, Txt, &TxtNr );
   }

   int gdxAutoConvert( int NV )
   {
     return ::gdxAutoConvert( pgx, NV );
   }

   int gdxClose()
   {
     return ::gdxClose( pgx );
   }

   int gdxDataErrorCount() const
   {
     return ::gdxDataErrorCount( pgx );
   }

   int gdxDataErrorRecord( int RecNr, int *KeyInt, double *Values )
   {
     return ::gdxDataErrorRecord( pgx, RecNr, KeyInt, Values );
   }

   int gdxDataErrorRecordX( int RecNr, int *KeyInt, double *Values )
   {
     return ::gdxDataErrorRecordX( pgx, RecNr, KeyInt, Values );
   }

   int gdxDataReadDone()
   {
     return ::gdxDataReadDone( pgx );
   }

   int gdxDataReadFilteredStart( int SyNr, const int *FilterAction, int &NrRecs )
   {
     return ::gdxDataReadFilteredStart( pgx, SyNr, FilterAction, &NrRecs );
   }

   int gdxDataReadMap( int RecNr, int *KeyInt, double *Values, int &DimFrst )
   {
     return ::gdxDataReadMap( pgx, RecNr, KeyInt, Values, &DimFrst );
   }

   int gdxDataReadMapStart( int SyNr, int &NrRecs )
   {
     return ::gdxDataReadMapStart( pgx, SyNr, &NrRecs );
   }

   int gdxDataReadRaw( int *KeyInt, double *Values, int &DimFrst )
   {
     return ::gdxDataReadRaw( pgx, KeyInt, Values, &DimFrst );
   }

   int gdxDataReadRawFast( int SyNr, TDataStoreProc_t DP, int &NrRecs )
   {
     return ::gdxDataReadRawFast( pgx, SyNr, DP, &NrRecs );
   }

   int gdxDataReadRawFastEx( int SyNr, TDataStoreExProc_t DP, int &NrRecs, void *Uptr )
   {
     return ::gdxDataReadRawFastEx( pgx, SyNr, DP, &NrRecs, Uptr );
   }

   int gdxDataReadRawFastFilt( int SyNr, const char **UelFilterStr, TDataStoreFiltProc_t DP )
   {
     return ::gdxDataReadRawFastFilt( pgx, SyNr, UelFilterStr, DP );
   }

   int gdxDataReadRawStart( int SyNr, int &NrRecs )
   {
     return ::gdxDataReadRawStart( pgx, SyNr, &NrRecs );
   }

   int gdxDataReadSlice( const char **UelFilterStr, int &Dimen, TDataStoreProc_t DP )
   {
     return ::gdxDataReadSlice( pgx, UelFilterStr, &Dimen, DP );
   }

   int gdxDataReadSliceStart( int SyNr, int *ElemCounts )
   {
     return ::gdxDataReadSliceStart( pgx, SyNr, ElemCounts );
   }

   int gdxDataReadStr( char **KeyStr, double *Values, int &DimFrst )
   {
     return ::gdxDataReadStr( pgx, KeyStr, Values, &DimFrst );
   }

   int gdxDataReadStrStart( int SyNr, int &NrRecs )
   {
     return ::gdxDataReadStrStart( pgx, SyNr, &NrRecs );
   }

   int gdxDataSliceUELS( const int *SliceKeyInt, char **KeyStr )
   {
     return ::gdxDataSliceUELS( pgx, SliceKeyInt, KeyStr );
   }

   int gdxDataWriteDone()
   {
     return ::gdxDataWriteDone( pgx );
   }

   int gdxDataWriteMap( const int *KeyInt, const double *Values )
   {
     return ::gdxDataWriteMap( pgx, KeyInt, Values );
   }

   int gdxDataWriteMapStart( const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo )
   {
     return ::gdxDataWriteMapStart( pgx, SyId, ExplTxt, Dimen, Typ, UserInfo );
   }

   int gdxDataWriteRaw( const int *KeyInt, const double *Values )
   {
     return ::gdxDataWriteRaw( pgx, KeyInt, Values );
   }

   int gdxDataWriteRawStart( const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo )
   {
     return ::gdxDataWriteRawStart( pgx, SyId, ExplTxt, Dimen, Typ, UserInfo );
   }

   int gdxDataWriteRawStartKeyBounds( const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo, const int *MinUELIndices, const int *MaxUELIndices )
   {
     return ::gdxDataWriteRawStartKeyBounds( pgx, SyId, ExplTxt, Dimen, Typ, UserInfo, MinUELIndices, MaxUELIndices );
   }

   int gdxDataWriteStr( const char **KeyStr, const double *Values )
   {
     return ::gdxDataWriteStr( pgx, KeyStr, Values );
   }

   int gdxDataWriteStrStart( const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo )
   {
     return ::gdxDataWriteStrStart( pgx, SyId, ExplTxt, Dimen, Typ, UserInfo );
   }

   int gdxGetDLLVersion( char *V )
   {
     return ::gdxGetDLLVersion( pgx, V );
   }

   int gdxErrorCount() const
   {
     return ::gdxErrorCount( pgx );
   }

   static int gdxErrorStr( int ErrNr, char *ErrMsg )
   {
     return ::gdxErrorStr( nullptr, ErrNr, ErrMsg );
   }

   int gdxFileInfo( int &FileVer, int &ComprLev ) const
   {
     return ::gdxFileInfo( pgx, &FileVer, &ComprLev );
   }

   int gdxFileVersion( char *FileStr, char *ProduceStr ) const
   {
     return ::gdxFileVersion( pgx, FileStr, ProduceStr );
   }

   int gdxFilterExists( int FilterNr )
   {
     return ::gdxFilterExists( pgx, FilterNr );
   }

   int gdxFilterRegister( int UelMap )
   {
     return ::gdxFilterRegister( pgx, UelMap );
   }

   int gdxFilterRegisterDone()
   {
     return ::gdxFilterRegisterDone( pgx );
   }

   int gdxFilterRegisterStart( int FilterNr )
   {
     return ::gdxFilterRegisterStart( pgx, FilterNr );
   }

   int gdxFindSymbol( const char *SyId, int &SyNr )
   {
     return ::gdxFindSymbol( pgx, SyId, &SyNr );
   }

   int gdxGetElemText( int TxtNr, char *Txt, int &Node )
   {
     return ::gdxGetElemText( pgx, TxtNr, Txt, &Node );
   }

   int gdxGetLastError()
   {
     return ::gdxGetLastError( pgx );
   }

   int64_t gdxGetMemoryUsed()
   {
     return ::gdxGetMemoryUsed( pgx );
   }

   int gdxGetSpecialValues( double *AVals )
   {
     return ::gdxGetSpecialValues( pgx, AVals );
   }

   int gdxGetUEL( int UelNr, char *Uel ) const
   {
     return ::gdxGetUEL( pgx, UelNr, Uel );
   }

   int gdxMapValue( double D, int &sv )
   {
     return ::gdxMapValue( pgx, D, &sv );
   }

   int gdxOpenAppend( const char *FileName, const char *Producer, int &ErrNr )
   {
     return ::gdxOpenAppend( pgx, FileName, Producer, &ErrNr );
   }

   int gdxOpenRead( const char *FileName, int &ErrNr )
   {
     return ::gdxOpenRead( pgx, FileName, &ErrNr );
   }

   int gdxOpenReadEx( const char *FileName, int ReadMode, int &ErrNr )
   {
     return ::gdxOpenReadEx( pgx, FileName, ReadMode, &ErrNr );
   }

   int gdxOpenWrite( const char *FileName, const char *Producer, int &ErrNr )
   {
     return ::gdxOpenWrite( pgx, FileName, Producer, &ErrNr );
   }

   int gdxOpenWriteEx( const char *FileName, const char *Producer, int Compr, int &ErrNr )
   {
     return ::gdxOpenWriteEx( pgx, FileName, Producer, Compr, &ErrNr );
   }

   int gdxResetSpecialValues()
   {
     return ::gdxResetSpecialValues( pgx );
   }

   int gdxSetHasText( int SyNr )
   {
     return ::gdxSetHasText( pgx, SyNr );
   }

   int gdxSetReadSpecialValues( const double *AVals )
   {
     return ::gdxSetReadSpecialValues( pgx, AVals );
   }

   int gdxSetSpecialValues( const double *AVals )
   {
     return ::gdxSetSpecialValues( pgx, AVals );
   }

   int gdxSetTextNodeNr( int TxtNr, int Node )
   {
     return ::gdxSetTextNodeNr( pgx, TxtNr, Node );
   }

   int gdxSetTraceLevel( int N, const char *s )
   {
     return ::gdxSetTraceLevel( pgx, N, s );
   }

   int gdxSymbIndxMaxLength( int SyNr, int *LengthInfo )
   {
     return ::gdxSymbIndxMaxLength( pgx, SyNr, LengthInfo );
   }

   int gdxSymbMaxLength() const
   {
     return ::gdxSymbMaxLength( pgx );
   }

   int gdxSymbolAddComment( int SyNr, const char *Txt )
   {
     return ::gdxSymbolAddComment( pgx, SyNr, Txt );
   }

   int gdxSymbolGetComment( int SyNr, int N, char *Txt )
   {
     return ::gdxSymbolGetComment( pgx, SyNr, N, Txt );
   }

   int gdxSymbolGetDomain( int SyNr, int *DomainSyNrs )
   {
     return ::gdxSymbolGetDomain( pgx, SyNr, DomainSyNrs );
   }

   int gdxSymbolGetDomainX( int SyNr, char **DomainIDs )
   {
     return ::gdxSymbolGetDomainX( pgx, SyNr, DomainIDs );
   }

   int gdxSymbolDim( int SyNr )
   {
     return ::gdxSymbolDim( pgx, SyNr );
   }

   int gdxSymbolInfo( int SyNr, char *SyId, int &Dimen, int &Typ )
   {
     return ::gdxSymbolInfo( pgx, SyNr, SyId, &Dimen, &Typ );
   }

   int gdxSymbolInfoX( int SyNr, int &RecCnt, int &UserInfo, char *ExplTxt )
   {
     return ::gdxSymbolInfoX( pgx, SyNr, &RecCnt, &UserInfo, ExplTxt );
   }

   int gdxSymbolSetDomain( const char **DomainIDs )
   {
     return ::gdxSymbolSetDomain( pgx, DomainIDs );
   }

   int gdxSymbolSetDomainX( int SyNr, const char **DomainIDs )
   {
     return ::gdxSymbolSetDomainX( pgx, SyNr, DomainIDs );
   }

   int gdxSystemInfo( int &SyCnt, int &UelCnt ) const
   {
     return ::gdxSystemInfo( pgx, &SyCnt, &UelCnt );
   }

   int gdxUELMaxLength() const
   {
     return ::gdxUELMaxLength( pgx );
   }

   int gdxUELRegisterDone()
   {
     return ::gdxUELRegisterDone( pgx );
   }

   int gdxUELRegisterMap( int UMap, const char *Uel )
   {
     return ::gdxUELRegisterMap( pgx, UMap, Uel );
   }

   int gdxUELRegisterMapStart()
   {
     return ::gdxUELRegisterMapStart( pgx );
   }

   int gdxUELRegisterRaw( const char *Uel )
   {
     return ::gdxUELRegisterRaw( pgx, Uel );
   }

   int gdxUELRegisterRawStart()
   {
     return ::gdxUELRegisterRawStart( pgx );
   }

   int gdxUELRegisterStr( const char *Uel, int &UelNr )
   {
     return ::gdxUELRegisterStr( pgx, Uel, &UelNr );
   }

   int gdxUELRegisterStrStart()
   {
     return ::gdxUELRegisterStrStart( pgx );
   }

   int gdxUMFindUEL( const char *Uel, int &UelNr, int &UelMap )
   {
     return ::gdxUMFindUEL( pgx, Uel, &UelNr, &UelMap );
   }

   int gdxUMUelGet( int UelNr, char *Uel, int &UelMap )
   {
     return ::gdxUMUelGet( pgx, UelNr, Uel, &UelMap );
   }

   int gdxUMUelInfo( int &UelCnt, int &HighMap ) const
   {
     return ::gdxUMUelInfo( pgx, &UelCnt, &HighMap );
   }

   int gdxGetDomainElements( int SyNr, int DimPos, int FilterNr, TDomainIndexProc_t DP, int &NrElem, void *Uptr )
   {
     return ::gdxGetDomainElements( pgx, SyNr, DimPos, FilterNr, DP, &NrElem, Uptr );
   }

   int gdxCurrentDim() const
   {
     return ::gdxCurrentDim( pgx );
   }

   int gdxRenameUEL( const char *OldName, const char *NewName )
   {
     return ::gdxRenameUEL( pgx, OldName, NewName );
   }

   int gdxUELMaxLength()
   {
      return ::gdxUELMaxLength( pgx );
   }

   int gdxGetDLLVersion( char *V ) const
   {
      return ::gdxGetDLLVersion( pgx, V );
   }
};

}// namespace gdx