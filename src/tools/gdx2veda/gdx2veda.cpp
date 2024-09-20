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
#include <cmath>

#include "gdx2veda.h"
#include "../library/common.h"
#include "../../gdlib/strutilx.h"
#include "../../gdlib/utils.h"

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

std::string StripExt( const std::string &fln )
{
   std::string result { fln };
   std::string ext { gdlib::strutilx::ExtractFileExtEx( fln ) };
   if( !ext.empty() )
      return result.erase( result.length() - ext.length(), ext.length() );
   return result;
}

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
   MappedValue = SpecialValueMapping.at( spv );
   IsAString = SpecialValueIsString.at( spv );
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

      for( SyNr = 1; SyNr <= NrSy; SyNr++ )
      {
         gdxSymbolInfo( PGX, SyNr, SyName.data(), &SyDim, &iSyType );
         SyType = gdxSyType( iSyType );
         gdxSymbolInfoX( PGX, SyNr, &ElemCount, &iDummy, SyText.data() );

         // TODO: Check whether this is necessary
         for( i = 3; SyNr <= MaxSyDim + 2; i++ )
            DataLine.at( i ).clear();

         switch( SyType )
         {
            case dt_set:
               DataLine.at( 2 ) = '"' + SyName.string() + '"';
               gdxDataReadStrStart( PGX, SyNr, &NrRecs );
               while( gdxDataReadStr( PGX, ElementsPtrs, Values, &First ) != 0 )
               {
                  for( i = First; i <= SyDim; i++ )
                     DataLine.at( i + 2 ) = '"' + std::string { Elements[i] } + '"';
                  WriteDataLine();
                  nn = std::round( Values[GMS_VAL_LEVEL] );
                  f << nn;
                  if( nn == 0 )
                     f << std::endl;
                  else
                  {
                     gdxGetElemText( PGX, std::round( Values[GMS_VAL_LEVEL] ), NodeStr.data(), &NodeNr );
                     f << ",\"" << NodeStr << '"' << std::endl;
                  }
               }
               break;

            case dt_par:
               DataLine.at( 2 ) = '"' + SyName.string() + '"';
               gdxDataReadStrStart( PGX, SyNr, &NrRecs );
               while( gdxDataReadStr( PGX, ElementsPtrs, Values, &First ) != 0 )
               {
                  for( i = First; i <= SyDim; i++ )
                     DataLine.at( i + 2 ) = '"' + std::string { Elements[i] } + '"';
                  WriteDataLine();
                  if( IsASpecialValue( Values[GMS_VAL_LEVEL], MappedValue, IsAString ) )
                  {
                     if( IsAString )
                        f << '"' << MappedValue << '"' << std::endl;
                     else
                        f << MappedValue << std::endl;
                  }
                  else
                     f << gdlib::strutilx::DblToStr( Values[GMS_VAL_LEVEL] ) << std::endl;
               }
               break;

            case dt_var:
            case dt_equ:
               gdxDataReadStrStart( PGX, SyNr, &NrRecs );
               while( gdxDataReadStr( PGX, ElementsPtrs, Values, &First ) != 0 )
               {
                  for( i = First; i <= SyDim; i++ )
                     DataLine.at( i + 2 ) = '"' + std::string { Elements[i] } + '"';
                  for( i = 1; i <= MaxSuff; i++ )
                  {
                     DataLine.at( 2 ) = '"' + SyName.string() + '.' + NameSuff.at( i ) + '"';
                     WriteDataLine();
                     if( IsASpecialValue( Values[i - 1], MappedValue, IsAString ) )
                     {
                        if( IsAString )
                           f << '"' << MappedValue << '"' << std::endl;
                        else
                           f << MappedValue << std::endl;
                     }
                     else
                        f << gdlib::strutilx::DblToStr( Values[i - 1] ) << std::endl;
                  }
               }
               break;

            default:
               // TODO: Throw an error here?
               break;
         }
      }

      f.close();
      gdxClose( PGX );
      gdxFree( &PGX );
      // UnloadGdxLibrary();
      return 1;
   }

   FnVdd = ParamStr[2];

   // Default RunId is the gdx file name
   // if runid given (3rd parameter) is also use
   // to see what format to write

   if( ParamCount > 2 )
   {
      FnVeda = ParamStr[3];
      RunId = StripExt( gdlib::strutilx::ExtractFileNameEx( FnVeda.string() ) );
      if( utils::sameText( gdlib::strutilx::ExtractFileExtEx( FnVeda.string() ), ".csv" ) )
      {
         FnVeda = gdlib::strutilx::ChangeFileExtEx( RunId.string() + "_vd", ".csv" );
         VedaLine = false;
      }
      else
      {
         VedaLine = true;
         FnVeda = gdlib::strutilx::ChangeFileExtEx( FnVeda.string(), ".vd" );
      }
   }
   else
   {
      RunId = StripExt( gdlib::strutilx::ExtractFileNameEx( FnGdx.string() ) );
      FnVeda = gdlib::strutilx::ChangeFileExtEx( RunId.string(), ".vd" );
      VedaLine = true;
   }

   // Empty strings are different for VEDA and CSV files
   if( VedaLine )
      Filler = "\"-\"";
   else
      Filler.clear();

   return 0;
}

}// namespace gdx2veda

int main( const int argc, const char *argv[] )
{
   return gdx2veda::main( argc, argv );
}
