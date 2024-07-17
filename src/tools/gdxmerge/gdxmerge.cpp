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

#include "gdxmerge.h"
#include "../library/short_string.h"

// GDX library interface
#include "../../../generated/gdxcc.h"

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
}

template<typename T>
void TSymbolList<T>::WriteNameList()
{
}

template<typename T>
void TSymbolList<T>::KeepNewAcronyms( const gdxHandle_t &PGX )
{
}

template<typename T>
void TSymbolList<T>::ShareAcronyms( const gdxHandle_t &PGX )
{
}

template<typename T>
int TSymbolList<T>::FindAcronym( const std::string &Id )
{
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
