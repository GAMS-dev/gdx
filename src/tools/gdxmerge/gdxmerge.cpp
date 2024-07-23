/**
 * GAMS - General Algebraic Modeling System C++ API
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

#include <sstream>
#include <iomanip>
#include <cassert>
#include <limits>
#include <cstring>
#include <iostream>
#include <cmath>

#include "gdxmerge.h"
#include "../../gdlib/strutilx.h"

// TODO: Disable at some point?
#define OLD_MEMORY_CHECK

namespace gdxmerge
{

static bool DoBigSymbols, StrictMode;
static int64_t SizeCutOff;
static std::string OutFile;
static std::vector<std::string> FilePatterns;
static gdxHandle_t PGXMerge { nullptr };
static unsigned int InputFilesRead;

std::string FormatDateTime( const std::tm &dt )
{
   auto int2 = []( const int n ) -> std::string {
      std::ostringstream oss;
      oss << std::setw( 2 ) << std::setfill( '0' ) << n;
      return oss.str();
   };

   const int year { dt.tm_year + 1900 },
           month { dt.tm_mon + 1 },
           day { dt.tm_mday },
           hour { dt.tm_hour },
           min { dt.tm_min },
           sec { dt.tm_sec };

   return int2( year ) + '/' + int2( month ) + '/' + int2( day ) + ' ' +
          int2( hour ) + ':' + int2( min ) + ':' + int2( sec );
}

template<typename T>
TGAMSSymbol<T>::TGAMSSymbol( const int ADim, const gdxSyType AType, const int ASubTyp ) : SyDim( ADim ), SyTyp( AType ), SySubTyp( ASubTyp )
{
   SyData = new gdlib::gmsdata::TTblGamsData<T>( ADim, sizeof( T ) );
   SySkip = false;
}

template<typename T>
TGAMSSymbol<T>::~TGAMSSymbol()
{
   delete SyData;
}

template<typename T>
TSymbolList<T>::TSymbolList() : gdlib::gmsobj::TXHashedStringList<T>()
{
   StrPool = new gdlib::gmsobj::TXStrPool<T>();
   const std::string empty_string;
   StrPool->Add( empty_string.data(), empty_string.length() );
   FileList = new TFileList<T>;
   library::short_string Msg;
   gdxCreate( &PGXMerge, Msg.data(), Msg.length() );
}

template<typename T>
TSymbolList<T>::~TSymbolList()
{
   delete StrPool;
   delete FileList;
}

template<typename T>
void TSymbolList<T>::OpenOutput( const std::string &AFileName, int &ErrNr )
{
   gdxOpenWrite( PGXMerge, AFileName.data(), "gdxmerge", &ErrNr );
   gdxStoreDomainSetsSet( PGXMerge, false );
}

template<typename T>
int TSymbolList<T>::AddUEL( const std::string &S )
{
   int result;
   gdxUELRegisterStr( PGXMerge, S.data(), &result );
   return result;
}

template<typename T>
int TSymbolList<T>::AddSymbol( const std::string &AName, const int ADim, const gdxSyType AType, const int ASubTyp )
{
   auto is_in_list = []( const std::vector<std::string> &list, const std::string &value ) {
      return std::find( list.begin(), list.end(), value ) != list.end();
   };

   if( ( !IncludeList.empty() && !is_in_list( IncludeList, AName ) ) ||
       ( !ExcludeList.empty() && is_in_list( ExcludeList, AName ) ) )
      return -1;

   auto *S = new TGAMSSymbol<T>( ADim, AType, ASubTyp );
   return StrPool->AddObject( AName.data(), AName.length(), S->syData );
}

template<typename T>
void TSymbolList<T>::AddPGXFile( int FNr, TProcessPass Pass )
{
   bool FrstError;
   library::short_string SyName, FileName;

   auto CheckError = [&]( const bool Cnd, const std::string &Msg ) -> bool {
      bool Result { !Cnd };
      if( Result )
      {
         FErrorCount++;
         if( FrstError )
         {
            std::cout << "\n**** Error in file " << FileName << std::endl;
            FrstError = false;
         }
         std::cout << "     " << Msg << ": " << SyName << std::endl;
      }
      return Result;
   };

   gdxHandle_t PGX { nullptr };
   int NrSy, NrUel, N, Dim, SyITyp, SyIndx, NrRecs, FDim, D, INode, SySubTyp, DummyCount, ErrNr, RecLen;
   gdxSyType SyTyp;
   TGAMSSymbol<T> SyObj;
   gdxStrIndex_t IndxS;
   gdxStrIndexPtrs_t IndxSPtrs;
   GDXSTRINDEXPTRS_INIT( IndxS, IndxSPtrs );
   gdxUelIndex_t IndxI;
   gdxValues_t Vals;
   library::short_string Txt, SyText, ErrMsg;
   std::string FileId;
   int64_t XCount, Size;

   FileName = FileList->FileName( FNr );
   FileId = FileList->FileId( FNr );

   std::cout << "Reading file: " << FileName << std::endl;
   gdxCreate( &PGX, ErrMsg.data(), ErrMsg.length() );
   gdxOpenRead( PGX, FileName.data(), &ErrNr );
   if( ErrNr != 0 )
   {
      gdxErrorStr( nullptr, ErrNr, ErrMsg.data() );
      std::cout << "\nError reading file, message: " << ErrMsg << std::endl;
      return;
   }
   InputFilesRead++;

   ShareAcronyms( PGX );

   gdxUELRegisterStrStart( PGXMerge );
   gdxSystemInfo( PGX, &NrSy, &NrUel );
   FrstError = true;

   for( N = 1; N <= NrSy; N++ )
   {
      gdxSymbolInfo( PGX, N, SyName.data(), &Dim, &SyITyp );
      gdxSymbolInfoX( PGX, N, &DummyCount, &SySubTyp, SyText.data() );
      if( CheckError( Dim < GMS_MAX_INDEX_DIM, "Dimension too large" ) )
         continue;
      SyTyp = gdxSyType( SyITyp );
      if( SyITyp == GMS_DT_ALIAS )
      {
         // We cannot create two dimensional aliased sets!
         SyTyp = dt_set;
         SySubTyp = 0;
      }
      // TODO: Check if FindAcronym is correct (replaces IndexOf(syName))
      SyIndx = FindAcronym( SyName );
      if( SyIndx < 0 )
      {
         SyIndx = AddSymbol( SyName, Dim + 1, SyTyp, SySubTyp );
         if( SyIndx < 0 )
            continue;
      }
      // TODO: Fix this (replaces TGAMSSymbol(Objects[syIndx]))
      SyObj = new TGAMSSymbol<T>();

      if( SyObj.SyData == nullptr )
         continue;

      if( SyObj.SySkip )
         continue;

      // 64 bit
      XCount = static_cast<int64_t>( DummyCount );
      Size = XCount * SyObj.SyDim;
      if( SyTyp == dt_var || SyTyp == dt_equ )
         RecLen = 4;
      else
         RecLen = 1;

      Size = Size * RecLen;

      if( Pass == TProcessPass::RpScan || Pass == TProcessPass::RpDoAll )
      {
         SyObj.SySize = SyObj.SySize + Size;
         SyObj.SyMemory = SyObj.SyMemory + XCount * ( SyObj.SyDim * sizeof( int ) + RecLen * sizeof( double ) );
         if( CheckError( SyObj.SyData->GetCount + XCount <= std::numeric_limits<int>::max(), "Element count for symbol > maxint" ) )
         {
            SyObj.SySkip = true;
            delete SyObj.SyData;
            continue;
         }
#if defined( OLD_MEMORY_CHECK )
         if( CheckError( SyObj.SyMemory <= std::numeric_limits<int>::max(), "Symbol is too large" ) )
         {
            SyObj.SySkip = true;
            delete SyObj.SyData;
            continue;
         }
#endif
      }

      if( Pass == TProcessPass::RpScan )
         continue;

      if( Pass == TProcessPass::RpSmall && SyObj.SySize >= SizeCutOff )
         continue;
      if( Pass == TProcessPass::RpBig && SyObj.SySize < SizeCutOff )
         continue;

      if( CheckError( Dim + 1 == SyObj.SyDim, "Dimensions do not match" ) )
         continue;
      if( CheckError( SyTyp == SyObj.SyTyp, "Types do not match" ) )
         continue;
      if( ( SyTyp == dt_var || SyTyp == dt_equ ) && CheckError( SySubTyp == SyObj.SySubTyp, "Var/Equ subtypes do not match" ) )
         continue;
      if( SyObj.SyExplTxt.empty() )
         SyObj.SyExplTxt = SyText;
      else if( !SyText.empty() )
         CheckError( SyObj.syExplTxt == SyText, "Explanatory text is different" );
      IndxI[1] = AddUEL( FileId );
      gdxDataReadStrStart( PGX, N, &NrRecs );

      while( gdxDataReadStr( PGX, IndxSPtrs, Vals, &FDim ) != 0 )
      {
         if( Dim > 0 )
            for( D = FDim; D <= Dim; D++ )
               IndxI[D + 1] = AddUEL( IndxS[D] );
         if( SyTyp == dt_set && Vals[GMS_VAL_LEVEL] != 0 )
         {
            gdxGetElemText( PGX, std::round( Vals[GMS_VAL_LEVEL] ), Txt.data(), &INode );
            Vals[GMS_VAL_LEVEL] = StrPool->Add( Txt );
         }
         SyObj.SyData->AddRecord( IndxI, Vals );
      }
      gdxDataReadDone( PGX );
   }

   KeepNewAcronyms( PGX );

   gdxClose( PGX );
   gdxFree( &PGX );
   gdxUELRegisterDone( PGXMerge );
}

template<typename T>
void TSymbolList<T>::WriteNameList()
{
   const std::string BASE_NAME { "Merged_set_" };
   library::short_string SetName;
   int N, SyNr, TextNr;
   gdxStrIndex_t AIndex;
   gdxStrIndexPtrs_t AIndexPtrs;
   GDXSTRINDEXPTRS_INIT( AIndex, AIndexPtrs );
   gdxValues_t AVals {};

   // Find unique name for the merged set
   N = 1;
   while( true )
   {
      SetName = BASE_NAME + std::to_string( N );
      gdxFindSymbol( PGXMerge, SetName.data(), &SyNr );
      if( SyNr < 0 )
         break;
      N++;
   }

   gdxDataWriteStrStart( PGXMerge, SetName.data(), "Merge set", 1, 0, 0 );
   for( N = 0; N < FileList->size(); N++ )
   {
      gdxAddSetText( PGXMerge, FileList->FileInfo( N ), TextNr );
      AVals[GMS_VAL_LEVEL] = TextNr;
      // AIndex[1] = FileList->FileId( N );
      strcpy( AIndexPtrs[1], FileList->FileId( N ) );
      // TODO: Check this const cast
      gdxDataWriteStr( PGXMerge, const_cast<const char **>( AIndexPtrs ), AVals );
   }
   gdxDataWriteDone( PGXMerge );
}

template<typename T>
void TSymbolList<T>::KeepNewAcronyms( const gdxHandle_t &PGX )
{
   int NN = gdxAcronymNextNr( PGX, -1 );
   if( NN <= NextAcroNr )
      return;

   int OrgIndx, NewIndx, AutoIndx, AIndx;
   library::short_string AName, AText;
   for( int N { 1 }; N <= gdxAcronymCount( PGX ); N++ )
   {
      gdxAcronymGetMapping( PGX, N, &OrgIndx, &NewIndx, &AutoIndx );
      if( NewIndx >= NextAcroNr )
      {
         gdxAcronymGetInfo( PGX, N, AName.data(), AText.data(), &AIndx );
         if( AName.empty() )
         {
            // Should not happen
            for( int K { 1 }; K <= std::numeric_limits<int>::max(); K++ )
            {
               AName = "Acronym_Auto_" + std::to_string( K );
               if( FindAcronym( AName ) < 0 )
                  break;
            }
            AText = "GDX file did not have a name for acronym";
         }
         gdxAcronymAdd( PGXMerge, AName.data(), AText.data(), NewIndx );
      }
   }

   NextAcroNr = NN;
}

template<typename T>
void TSymbolList<T>::ShareAcronyms( const gdxHandle_t &PGX )
{
   if( gdxAcronymCount( PGX ) == 0 )
   {
      gdxAcronymNextNr( PGX, 0 );
      return;
   }

   library::short_string AName, AText;
   int AIndx;
   if( NextAcroNr == 0 )
   {
      gdxAcronymGetInfo( PGX, 1, AName.data(), AText.data(), &AIndx );
      NextAcroNr = AIndx;
   }

   gdxAcronymNextNr( PGX, NextAcroNr );

   library::short_string ANameM, ATextM;
   int NM, AIndxM;
   for( int N { 1 }; N <= gdxAcronymCount( PGX ); N++ )
   {
      gdxAcronymGetInfo( PGX, N, AName.data(), AText.data(), &AIndx );
      NM = FindAcronym( AName );
      if( NM <= 0 )
         continue;
      gdxAcronymGetInfo( PGXMerge, NM, ANameM.data(), ATextM.data(), &AIndxM );
      assert( AIndxM > 0 && "ShareAcronyms-1" );
      gdxAcronymSetInfo( PGX, N, AName.data(), AText.data(), AIndxM );
   }
}

template<typename T>
int TSymbolList<T>::FindAcronym( const library::short_string &Id )
{
   library::short_string AName, AText;
   int AIndx;
   for( int N { 1 }; N <= gdxAcronymCount( PGXMerge ); N++ )
   {
      gdxAcronymGetInfo( PGXMerge, N, AName.data(), AText.data(), &AIndx );
      if( gdlib::strutilx::StrUEqual( Id.string(), AName.string() ) )
         return N;
   }
   return {};
}

int main( const int argc, const char *argv[] )
{
   return {};
}

}// namespace gdxmerge

int main( const int argc, const char *argv[] )
{
   return gdxmerge::main( argc, argv );
}
