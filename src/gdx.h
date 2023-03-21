/*
 * GAMS - General Algebraic Modeling System GDX API
 *
 * Copyright (c) 2017-2023 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2023 GAMS Development Corp. <support@gams.com>
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

#pragma once

#include "gxfile.h"

namespace gdx
{

// Description:
//    Class for reading and writing gdx files
class TGXFileObj
{
public:
   explicit TGXFileObj( std::string &ErrMsg );
   ~TGXFileObj();

   int gdxOpenWrite( const char *FileName, const char *Producer, int &ErrNr );
   int gdxOpenWriteEx( const char *FileName, const char *Producer, int Compr, int &ErrNr );
   int gdxDataWriteStrStart( const char *SyId, const char *ExplTxt, int Dim, int Typ, int UserInfo );
   int gdxDataWriteStr( const char **KeyStr, const double *Values );
   int gdxDataWriteDone();
   int gdxUELRegisterMapStart();
   int gdxUELRegisterMap( int UMap, const char *Uel );
   int gdxClose();
   int gdxResetSpecialValues();
   static int gdxErrorStr( int ErrNr, char *ErrMsg );
   int gdxOpenRead( const char *FileName, int &ErrNr );
   int gdxFileVersion( char *FileStr, char *ProduceStr ) const;
   int gdxFindSymbol( const char *SyId, int &SyNr );
   int gdxDataReadStr( char **KeyStr, double *Values, int &DimFrst );
   int gdxDataReadDone();
   int gdxSymbolInfo( int SyNr, char *SyId, int &Dim, int &Typ );
   int gdxDataReadStrStart( int SyNr, int &NrRecs );
   int gdxAddAlias( const char *Id1, const char *Id2 );
   int gdxAddSetText( const char *Txt, int &TxtNr );
   [[nodiscard]] int gdxDataErrorCount() const;
   int gdxDataErrorRecord( int RecNr, int *KeyInt, double *Values );
   int gdxDataErrorRecordX( int RecNr, int *KeyInt, double *Values );
   int gdxDataReadRaw( int *KeyInt, double *Values, int &DimFrst );
   int gdxDataReadRawStart( int SyNr, int &NrRecs );
   int gdxDataWriteRaw( const int *KeyInt, const double *Values );
   int gdxDataWriteRawStart( const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo );
   [[nodiscard]] int gdxErrorCount() const;
   int gdxGetElemText( int TxtNr, char *Txt, int &Node );
   int gdxGetLastError();
   int gdxGetSpecialValues( double *Avals );
   int gdxSetSpecialValues( const double *AVals );
   int gdxSymbolGetDomain( int SyNr, int *DomainSyNrs );
   int gdxSymbolGetDomainX( int SyNr, char **DomainIDs );
   int gdxSymbolDim( int SyNr );
   int gdxSymbolInfoX( int SyNr, int &RecCnt, int &UserInfo, char *ExplTxt );
   int gdxSymbolSetDomain( const char **DomainIDs );
   int gdxSymbolSetDomainX( int SyNr, const char **DomainIDs );
   int gdxSystemInfo( int &SyCnt, int &UelCnt ) const;
   int gdxUELRegisterDone();
   int gdxUELRegisterRaw( const char *Uel );
   int gdxUELRegisterRawStart();
   int gdxUELRegisterStr( const char *Uel, int &UelNr );
   int gdxUELRegisterStrStart();
   int gdxUMUelGet( int UelNr, char *Uel, int &UelMap );
   int gdxUMUelInfo( int &UelCnt, int &HighMap ) const;
   [[nodiscard]] int gdxCurrentDim() const;
   int gdxRenameUEL( const char *OldName, const char *NewName );
   int gdxOpenReadEx( const char *FileName, int ReadMode, int &ErrNr );
   int gdxGetUEL( int uelNr, char *Uel ) const;
   int gdxDataWriteMapStart( const char *SyId, const char *ExplTxt, int Dimen, int Typ, int UserInfo );
   int gdxDataWriteMap( const int *KeyInt, const double *Values );
   int gdxDataReadMapStart( int SyNr, int &NrRecs );
   int gdxDataReadMap( int RecNr, int *KeyInt, double *Values, int &DimFrst );

   enum class TraceLevels
   {
      trl_none,
      trl_errors,
      trl_some,
      trl_all
   };
   void SetTraceLevel( TraceLevels tl );

   // region Acronym handling
   [[nodiscard]] int gdxAcronymCount() const;
   int gdxAcronymGetInfo( int N, char *AName, char *Txt, int &AIndx ) const;
   int gdxAcronymSetInfo( int N, const char *AName, const char *Txt, int AIndx );
   int gdxAcronymNextNr( int nv );
   int gdxAcronymGetMapping( int N, int &orgIndx, int &newIndx, int &autoIndex );
   // endregion

   // region Filter handling
   int gdxFilterExists( int FilterNr );
   int gdxFilterRegisterStart( int FilterNr );
   int gdxFilterRegister( int UelMap );
   int gdxFilterRegisterDone();
   int gdxDataReadFilteredStart( int SyNr, const int *FilterAction, int &NrRecs );
   // endregion

   int gdxSetTextNodeNr( int TxtNr, int Node );
   int gdxGetDomainElements( int SyNr, int DimPos, int FilterNr, TDomainIndexProc_t DP, int &NrElem, void *UPtr );
   int gdxSetTraceLevel( int N, const char *s );
   int gdxAcronymAdd( const char *AName, const char *Txt, int AIndx );
   [[nodiscard]] int gdxAcronymIndex( double V ) const;
   int gdxAcronymName( double V, char *AName );
   [[nodiscard]] double gdxAcronymValue( int AIndx ) const;
   int gdxAutoConvert( int nv );
   static int gdxGetDLLVersion( char *V );
   int gdxFileInfo( int &FileVer, int &ComprLev ) const;
   int gdxDataReadSliceStart( int SyNr, int *ElemCounts );
   int gdxDataReadSlice( const char **UelFilterStr, int &Dimen, TDataStoreProc_t DP );
   int gdxDataSliceUELS( const int *SliceKeyInt, char **KeyStr );
   int64_t gdxGetMemoryUsed();
   int gdxMapValue( double D, int &sv );
   int gdxOpenAppend( const char *FileName, const char *Producer, int &ErrNr );
   int gdxSetHasText( int SyNr );
   int gdxSetReadSpecialValues( const double *AVals );
   int gdxSymbIndxMaxLength( int SyNr, int *LengthInfo );
   [[nodiscard]] int gdxSymbMaxLength() const;
   int gdxSymbolAddComment( int SyNr, const char *Txt );
   int gdxSymbolGetComment( int SyNr, int N, char *Txt );
   [[nodiscard]] int gdxUELMaxLength() const;
   int gdxUMFindUEL( const char *Uel, int &UelNr, int &UelMap );
   [[nodiscard]] int gdxStoreDomainSets() const;
   void gdxStoreDomainSetsSet( int x );
   int gdxDataReadRawFastFilt( int SyNr, const char **UelFilterStr, TDataStoreFiltProc_t DP );
   int gdxDataReadRawFast( int SyNr, TDataStoreProc_t DP, int &NrRecs );
   int gdxDataReadRawFastEx( int SyNr, TDataStoreExProc_t DP, int &NrRecs, void *Uptr );

private:
   std::unique_ptr<gmsstrm::TMiBufferedStreamDelphi> FFile;
   TgxFileMode fmode{ f_not_open }, fmode_AftReg{ f_not_open };
   enum
   {
      stat_notopen,
      stat_read,
      stat_write
   } fstatus{ stat_notopen };
   int fComprLev{};
   std::unique_ptr<TUELTable> UELTable;
   std::unique_ptr<TSetTextList> SetTextList{};
   std::unique_ptr<int[]> MapSetText{};
   int FCurrentDim{};
   std::array<int, GLOBAL_MAX_INDEX_DIM> LastElem{}, PrevElem{}, MinElem{}, MaxElem{};
   std::array<std::array<char, GLOBAL_UEL_IDENT_SIZE>, GLOBAL_MAX_INDEX_DIM> LastStrElem{};
   int DataSize{};
   tvarvaltype LastDataField{};
   std::unique_ptr<TNameList> NameList;
   std::unique_ptr<TDomainStrList> DomainStrList;
   std::unique_ptr<LinkedDataType> SortList;
   std::optional<LinkedDataIteratorType> ReadPtr;
   std::unique_ptr<TTblGamsDataImpl<double>> ErrorList;
   PgdxSymbRecord CurSyPtr{};
   int ErrCnt{}, ErrCntTotal{};
   int LastError{}, LastRepError{};
   std::unique_ptr<TFilterList> FilterList;
   TDFilter *CurFilter{};
   TDomainList DomainList{};
   bool StoreDomainSets{ true };
   TIntlValueMapDbl intlValueMapDbl{}, readIntlValueMapDbl{};
   TIntlValueMapI64 intlValueMapI64{};
   TraceLevels TraceLevel{ TraceLevels::trl_all };
   std::string TraceStr;
   int VersionRead{};
   std::string FProducer, FProducer2, FileSystemID;
   int64_t MajorIndexPosition{};
   int64_t NextWritePosition{};
   int DataCount{}, NrMappedAdded{};
   std::array<TgdxElemSize, GLOBAL_MAX_INDEX_DIM> ElemType{};
   std::string MajContext;
   std::array<std::optional<TIntegerMapping>, GLOBAL_MAX_INDEX_DIM> SliceIndxs, SliceRevMap;
   int SliceSyNr{};
   std::array<std::string, GMS_MAX_INDEX_DIM> SliceElems;
   bool DoUncompress{},  // when reading
           CompressOut{};// when writing
   int DeltaForWrite{};  // delta for last dimension or first changed dimension
   int DeltaForRead{};   // first position indicating change
   double Zvalacr{};     // tricky
   std::unique_ptr<TAcronymList> AcronymList;
   std::array<TSetBitMap *, GLOBAL_MAX_INDEX_DIM> WrBitMaps{};
   bool ReadUniverse{};
   int UniverseNr{}, UelCntOrig{};// original uel count when we open the file
   int AutoConvert{ 1 };
   int NextAutoAcronym{};
   bool AppendActive{};

#ifndef VERBOSE_TRACE
   const TraceLevels defaultTraceLevel{ TraceLevels::trl_none };
   const bool verboseTrace{};
#else
   const TraceLevels defaultTraceLevel{ TraceLevels::trl_all };
   const bool verboseTrace{ true };
#endif

   //api wrapper magic for Fortran
   TDataStoreFiltProc_t gdxDataReadRawFastFilt_DP{};
   TDomainIndexProc_t gdxGetDomainElements_DP{};

   bool PrepareSymbolWrite( std::string_view Caller, const char *AName, const char *AText, int ADim, int AType, int AUserInfo );
   int PrepareSymbolRead( std::string_view Caller, int SyNr, const int *ADomainNrs, TgxFileMode newmode );

   void InitErrors();
   void SetError( int N );
   void ReportError( int N );
   bool ErrorCondition( bool cnd, int N );

   bool MajorCheckMode( std::string_view Routine, TgxFileMode m );
   bool MajorCheckMode( std::string_view Routine, const TgxModeSet &MS );

   bool CheckMode( std::string_view Routine );
   bool CheckMode( std::string_view Routine, TgxFileMode m );
   bool CheckMode( std::string_view Routine, const TgxModeSet &MS );

   void WriteTrace( std::string_view s ) const;
   void InitDoWrite( int NrRecs );
   bool DoWrite( const int *AElements, const double *AVals );
   bool DoRead( double *AVals, int &AFDim );
   void AddToErrorListDomErrs( const std::array<int, GLOBAL_MAX_INDEX_DIM> &AElements, const double *AVals );
   void AddToErrorList( const int *AElements, const double *AVals );
   void GetDefaultRecord( double *Avals ) const;
   double AcronymRemap( double V );
   bool IsGoodNewSymbol( const char *s );
   bool ResultWillBeSorted( const int *ADomainNrs );

   int gdxOpenReadXX( const char *Afn, int filemode, int ReadMode, int &ErrNr );

   // This one is a helper function for a callback from a Fortran client
   void gdxGetDomainElements_DP_FC( int RawIndex, int MappedIndex, void *Uptr );
   int gdxDataReadRawFastFilt_DP_FC( const int *Indx, const double *Vals, void *Uptr );

public:
   bool gdxGetDomainElements_DP_CallByRef{},
           gdxDataReadRawFastFilt_DP_CallByRef{},
           gdxDataReadRawFastEx_DP_CallByRef{};
};

}// namespace gdx
