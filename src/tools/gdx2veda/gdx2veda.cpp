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

#include <iostream>
#include <iomanip>
#include <map>
#include <fstream>
#include <cstring>

#include "gdx2veda.h"
#include "../library/common.h"
#include "../../gdlib/strutilx.h"

namespace gdx2veda
{

// YYYY-MM-DD, ISO format
const std::string BuildVersion { "2005-10-07" };

// Store the Uels
library::one_indexed_container<library::short_string> Uels { 1000000 };
library::one_indexed_container<library::short_string> DataLine { GMS_MAX_INDEX_DIM };

gdxSVals_t SpecialValues {};

// std::ofstream f, g;
std::ofstream g;

library::short_string
        Msg,
        HomeDir,
        RunId,
        FnVdd,
        FnDll,
        FnVeda,
        FnVedah,
        FnVde,
        FnVds,
        FnGdx;

gdxHandle_t PGX;

gdxStrIndex_t Elements {};
gdxStrIndexPtrs_t ElementsPtrs;

gdxValues_t Values {};
gdxUelIndex_t Indices {}, MappedIndices {};
int First {};
library::short_string tmp;

int
        NodeNr {},
        NrRecs {},
        iSyType {},
        SyNr {},
        NrSy {}, // Number of symbols in gdx file
        NrUel {},// Number of Uels in gdx file
        ElemCount {},
        SyDim {}, MaxSyDim {}, SyDimMapped {},
        Cnt {}, Cnt1 {},
        CntTxt {}, CntSub {},
        i {}, j {}, jj {}, k {}, l {}, n {}, nn {},
        iDummy {},
        rc {};

gdxSyType SyType;

library::short_string NodeStr, SyText, SyName;

std::map<gdxSyType, std::string> DataText {
        { dt_set, "Set" },
        { dt_par, "Par" },
        { dt_var, "Var" },
        { dt_equ, "Equ" } };

bool Skip {};
library::short_string Filler;
bool VedaLine {};

// TDataLineMapping DataLineMapping {};
int h {};
gdxUelIndex_t x {};
library::short_string s, s2, s3;
int Uel {};
library::short_string DimensionName;
int DimensionNumber {};
library::one_indexed_container<int> TextDim { MaxText };
gdxUelIndex_t IndexMapping {};
int UelLength {}, Mark {};

library::short_string MappedValue;
bool IsAString {};

int ParentIndx {}, ChildIndx {};
int ParentUel {}, ChildUel {}, ExplanText {};

library::short_string Symbol;

int DimNo {}, TextDimension {};

int ExplText {};
library::one_indexed_container<int> LiteralFilter { GMS_MAX_INDEX_DIM };

bool DoSuppressZero {};
double xx1 {}, xx2 {};

void ShortHelp()
{
   std::cout << '\n'
             //  << gdlGetAuditLine() << '\n'
             //  << '\n'
             //  << '\n'
             << ">gdx2veda gdx vdd [run]" << '\n'
             << '\n'
             << "   gdx  GAMS GDX file" << '\n'
             << "   vdd  VEDA Data Definition file" << '\n'
             << "   run  VEDA Run identifier (optional)" << '\n'
             << '\n'
             << "The VEDA data file name and run identifier are either taken from the gdx file name" << '\n'
             << "or specified with the run name. Use \"token with blanks\" if needed." << '\n'
             << '\n'
             << ">gdx2veda mygdx    //  will dump the gdx symbols" << '\n'
             << ">gdx2veda          //  prints this message" << '\n'
             << ">gdx2veda --help   //  prints more detailed help message" << '\n'
             << '\n'
             << "Add .csv to the run name to write in csv format" << '\n'
             << std::endl;
}

void WriteDataLine()
{
   for( int i { 1 }; i <= MaxSyDim + 2; i++ )
      f << DataLine.at( i ) << ',';
   // f << std::endl;
}

void GetSpecialValues( const gdxHandle_t &PGX )
{
   gdxResetSpecialValues( PGX );
   gdxGetSpecialValues( PGX, SpecialValues );
}

bool IsASpecialValue( const double v, library::short_string &MappedValue, bool &IsAString )
{
   gdxSpecValue spv { gdxSpecValue( v ) };
   if( spv == sv_normal )
      return false;
   // TODO:
   // MappedValue = SpecialValueMapping.at(spv);
   // IsAString = SpecialValueIsString.at(spv);
   return true;
}

int main( const int argc, const char *argv[] )
{
   const int ParamCount { argc - 1 };
   const char **ParamStr { argv };

   // TODO: Remove? Update to GDX2VEDA?
   // gdlSetSystemName( 'GMS2VEDA' );
   // if( gdlib::strutilx::StrUEqual( ParamStr[1], "AUDIT" ) )
   // {
   //    std::cout << gdlGetAuditLine() << std::endl;
   //    return 0;
   // }

   GDXSTRINDEXPTRS_INIT( Elements, ElementsPtrs );

   NumErr = 0;

   std::string help;
   if( ParamCount == 0 )
      help = "-H";
   else
      help = gdlib::strutilx::UpperCase( ParamStr[1] );

   if( help == "--HELP" )
   {
      ShortHelp();
      // VddHelp();
      return 0;
   }

   if( help == "-H" || help == "-HELP" || help == "/?" || help == "?" )
   {
      ShortHelp();
      return 0;
   }

   if( !gdxGetReady( Msg.data(), Msg.length() ) )
   {
      library::printErrorMessage( "*** Could not load GDX library" );
      library::printErrorMessage( "*** Msg: " + Msg.string() );
      return 1;
   }

   FnGdx = gdlib::strutilx::CompleteFileExtEx( ParamStr[1], ".gdx" );
   gdxCreate( &PGX, Msg.data(), Msg.length() );
   gdxOpenRead( PGX, FnGdx.data(), &rc );
   if( rc != 0 )
   {
      gdxErrorStr( nullptr, rc, Msg.data() );
      library::printErrorMessage( "*** Could not open GDX: " + FnGdx.string() );
      library::printErrorMessage( "*** Msg: " + Msg.string() );
      // UnloadGdxLibrary();
      return 1;
   }

   GetSpecialValues( PGX );
   gdxSystemInfo( PGX, &NrSy, &NrUel );
   std::cout << "--- GDX File : Symbols=" << NrSy << " UELs=" << NrUel << std::endl;

   if( ParamCount == 1 )
   {
      FnVeda = gdlib::strutilx::ChangeFileExtEx( FnGdx.string(), ".csv" );
      std::cout << "\nContent of GDX " << FnGdx << " dump written to " << FnVeda << '\n'
                << "\nNum Typ Dim Count  Name" << std::endl;

      for( n = 1; n <= NrSy; n++ )
      {
         int xf;
         gdxSymbolInfo( PGX, n, SyName.data(), &xf, &iSyType );
         SyType = gdxSyType( iSyType );

         if( SyDim > MaxSyDim )
            MaxSyDim = SyDim;

         gdxSymbolInfoX( PGX, n, &ElemCount, &iDummy, SyText.data() );

         std::cout << std::setw( 3 ) << n << ' '
                   << DataText.at( SyType )
                   << std::setw( 3 ) << SyDim
                   << std::setw( 6 ) << ElemCount
                   << "  " << SyName
                   << std::setw( 12 - SyName.length() ) << ' '
                   << SyText << std::endl;

         if( SyType == dt_var || SyType == dt_equ )
            Cnt1 += MaxSuff * ElemCount;
         else
            Cnt1 += ElemCount;
         Cnt += ElemCount;
      }

      std::cout << std::setw( 17 ) << Cnt << "  GDX record count" << '\n'
                << std::setw( 17 ) << Cnt1 + 1 << "  CSV record count (including header)" << std::endl;
   }

   f.open( FnVeda.string() );
   if( !f.is_open() )
   {
      ReportError( "Could not open file: " + FnVeda.string() );
      ReportError( "Msg: " + std::string { strerror( errno ) } );
      return 1;
   }

   DataLine.at( 1 ) = '"' + gdlib::strutilx::ExtractFileNameEx( FnGdx.string() ) + '"';
   DataLine.at( 2 ) = "\"Name\"";
   for( i = 1; i <= MaxSyDim; i++ )
      DataLine.at( i + 2 ) = "\"Index " + std::to_string( i ) + '"';
   WriteDataLine();
   f << "\"Value\",\"Text\"" << std::endl;

   // TODO: f.close();
   return 0;
}

}// namespace gdx2veda

int main( const int argc, const char *argv[] )
{
   return gdx2veda::main( argc, argv );
}
