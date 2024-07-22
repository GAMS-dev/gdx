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

#include "gdxmerge.h"
#include "../../gdlib/strutilx.h"

namespace gdxmerge
{

static bool DoBigSymbols, StrictMode;
static int64_t SizeCutOff;
static std::string OutFile;
static std::vector<std::string> FilePatterns;
static gdxHandle_t PGXMerge { nullptr };

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
TGAMSSymbol<T>::TGAMSSymbol( const int ADim, const int AType, const int ASubTyp ) : syDim( ADim ), syTyp( AType ), sySubTyp( ASubTyp )
{
   syData = new gdlib::gmsdata::TTblGamsData<T>( ADim, sizeof( T ) );
   sySkip = false;
}

template<typename T>
TGAMSSymbol<T>::~TGAMSSymbol()
{
   delete syData;
}

template<typename T>
TSymbolList<T>::TSymbolList() : gdlib::gmsobj::TXHashedStringList<T>()
{
   StrPool = new gdlib::gmsobj::TXStrPool<T>();
   StrPool->Add( "" );
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
int TSymbolList<T>::AddSymbol( const std::string &AName, const int ADim, const int AType, const int ASubTyp )
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
   std::string SyName, FileName;

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

   // find unique name for the merged set
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
