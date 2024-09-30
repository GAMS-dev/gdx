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
#include <cstring>
#include <cmath>
#include <cstring>
#include <cassert>

#include "gdx2veda.h"
#include "../../gdlib/strutilx.h"
#include "../../gdlib/utils.h"

namespace gdx2veda
{

// YYYY-MM-DD, ISO format
const std::string BuildVersion { "2005-10-07" };

// Store the Uels
library::container<library::short_string> Uels { 1000000 };
library::container<library::short_string> DataLine { GMS_MAX_INDEX_DIM };

Projection_t Projection {};
library::short_string ParentField;

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
        FnVedaH,
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

DataLineMapping_t DataLineMapping {};
int h {};
gdxUelIndex_t x {};
library::short_string s, s2, s3;
int Uel {};
library::short_string DimensionName;
int DimensionNumber {};
library::container<int> TextDim { MaxText };
gdxUelIndex_t IndexMapping {};
int UelLength {}, Mark {};

library::short_string MappedValue;
bool IsAString {};

int ParentIndx {}, ChildIndx {};
int ParentUel {}, ChildUel {}, ExplanText {};

library::short_string Symbol;

int DimNo {}, TextDimension {};

int ExplText {};
library::container<int> LiteralFilter { GMS_MAX_INDEX_DIM };

bool DoSuppressZero {};
double xx1 {}, xx2 {};

HashTuple_t::HashTuple_t()
{
   // TODO: Implement constructor
}

Projection_t::Projection_t()
{
   // TODO: Implement constructor
}

DataLineMapping_t::DataLineMapping_t()
{
   // TODO: Implement constructor
}

void DataLineMapping_t::Clear()
{
   // TODO: Implement function
}

void DataLineMapping_t::Insert( const int Dimension, const int TupleIndex, const int EntryIndex )
{
   // TODO: Implement function
}

std::string StripExt( const std::string &fln )
{
   std::string result { fln };
   std::string ext { gdlib::strutilx::ExtractFileExtEx( fln ) };
   if( !ext.empty() )
      return result.erase( result.length() - ext.length(), ext.length() );
   return result;
}

std::string DQuotedStr( const std::string &s )
{
   // Workaround for p2c bug
   std::string qs { "\"" }, result { s };
   for( i = result.length() - 1; i >= 0; i-- )
      if( result.at( i ) == '"' )
         qs.insert( i, result );
   return "\"" + result + "\"";
}

void WriteHeader( std::ofstream &f, const std::string &key, const std::string &value )
{
   switch( Options.Format )
   {
      case Format_t::FormatCSV:
         f << DQuotedStr( key ) << ',' << DQuotedStr( value ) << std::endl;
         break;

      case Format_t::FormatVeda:
         f << '*' << std::setw( 17 ) << key << "- " << value << std::endl;
         break;
   }
}

void ShortHelp( const library::AuditLine &AuditLine )
{
   std::cout << '\n'
             << AuditLine.getAuditLine() << "\n\n"
             << ">gdx2veda gdx vdd [run]\n"
             << '\n'
             << "   gdx  GAMS GDX file\n"
             << "   vdd  VEDA Data Definition file\n"
             << "   run  VEDA Run identifier (optional)\n"
             << '\n'
             << "The VEDA data file name and run identifier are either taken from the gdx file name\n"
             << "or specified with the run name. Use \"token with blanks\" if needed.\n"
             << '\n'
             << ">gdx2veda mygdx    //  will dump the gdx symbols\n"
             << ">gdx2veda          //  prints this message\n"
             << ">gdx2veda --help   //  prints more detailed help message\n"
             << '\n'
             << "Add .csv to the run name to write in csv format\n"
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

void WriteValue( std::ofstream &f, double v )
{
   library::short_string MappedValue;
   bool IsAString;
   if( IsASpecialValue( v, MappedValue, IsAString ) )
   {
      if( IsAString )
         f << '"' << MappedValue << '"';
      else
         f << MappedValue;
   }
   else
      f << gdlib::strutilx::DblToStr( v );
}

std::string WritePV( char sepchar )
{
   if( Options.ValueDim == 2 )
      return "PV" + std::string { sepchar } + "DV";
   else
      return "PV";
}

std::string FormIndices( const gdxUelIndex_t &x, int n )
{
   // TODO: Implement function
   return {};
}

void CheckLiterals()
{
   int EN {}, UMap {};
   for( i = 1; i <= NumLiteral; i++ )
   {
      if( gdxUMFindUEL( PGX, LiteralPool.at( i ).data(), &EN, &UMap ) == 0 )
      {
         ReportError( "Literal " + LiteralPool.at( i ) + " not a valid UEL in GDX file" );
         EN = -1;
      }
      LiteralUel.at( i ) = EN;
   }
}

bool CheckLiteralFilter( const gdxUelIndex_t &Indices, const int N )
{
   if( NumLiteral == 0 )
      return true;
   for( int k { 1 }; k <= N; k++ )
   {
      if( LiteralFilter.at( k ) == -1 )
         continue;
      if( Indices[k] != LiteralFilter.at( k ) )
         return false;
   }
   return true;
}

double SpecialValueCheck( const double d )
{
   gdxSpecValue xsv { gdxSpecValue( static_cast<int>( d ) ) };
   if( xsv == sv_normal )
      return d;
   if( SpecialValueIsZero.at( xsv ) )
      return {};
   return d;
}

int main( const int argc, const char *argv[] )
{
   const int ParamCount { argc - 1 };
   const char **ParamStr { argv };

   library::AuditLine AuditLine { "GMS2VEDA" };
   if( argc > 1 && gdlib::strutilx::StrUEqual( ParamStr[1], "AUDIT" ) )
   {
      std::cout << AuditLine.getAuditLine() << std::endl;
      return 0;
   }

   GDXSTRINDEXPTRS_INIT( Elements, ElementsPtrs );

   NumErr = 0;

   std::string help;
   if( ParamCount == 0 )
      help = "-H";
   else
      help = gdlib::strutilx::UpperCase( ParamStr[1] );

   if( help == "--HELP" )
   {
      ShortHelp( AuditLine );
      // VddHelp();
      return 0;
   }

   if( help == "-H" || help == "-HELP" || help == "/?" || help == "?" )
   {
      ShortHelp( AuditLine );
      return 0;
   }

   if( !gdxGetReady( Msg.data(), Msg.length() ) )
   {
      library::printErrorMessage( "*** Could not load GDX library" );
      library::printErrorMessage( "*** Msg: " + Msg );
      return 1;
   }

   FnGdx = gdlib::strutilx::CompleteFileExtEx( ParamStr[1], ".gdx" );
   gdxCreate( &PGX, Msg.data(), Msg.length() );
   gdxOpenRead( PGX, FnGdx.data(), &rc );
   if( rc != 0 )
   {
      gdxErrorStr( nullptr, rc, Msg.data() );
      library::printErrorMessage( "*** Could not open GDX: " + FnGdx );
      library::printErrorMessage( "*** Msg: " + Msg );
      // UnloadGdxLibrary();
      return 1;
   }

   GetSpecialValues( PGX );
   gdxSystemInfo( PGX, &NrSy, &NrUel );
   std::cout << "--- GDX File : Symbols=" << NrSy << " UELs=" << NrUel << std::endl;

   if( ParamCount == 1 )
   {
      FnVeda = gdlib::strutilx::ChangeFileExtEx( FnGdx, ".csv" );
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

      std::cout << std::setw( 17 ) << Cnt << "  GDX record count\n"
                << std::setw( 17 ) << Cnt1 + 1 << "  CSV record count (including header)" << std::endl;

      f.open( FnVeda );
      if( !f.is_open() )
      {
         ReportError( "Could not open file: " + FnVeda );
         ReportError( "Msg: " + std::string { strerror( errno ) } );
         return 1;
      }

      DataLine.at( 1 ) = '"' + gdlib::strutilx::ExtractFileNameEx( FnGdx ) + '"';
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
               DataLine.at( 2 ) = '"' + SyName + '"';
               gdxDataReadStrStart( PGX, SyNr, &NrRecs );
               while( gdxDataReadStr( PGX, ElementsPtrs, Values, &First ) != 0 )
               {
                  for( i = First; i <= SyDim; i++ )
                     DataLine.at( i + 2 ) = '"' + std::string { Elements[i] } + '"';
                  WriteDataLine();
                  nn = static_cast<int>( std::round( Values[GMS_VAL_LEVEL] ) );
                  f << nn;
                  if( nn == 0 )
                     f << std::endl;
                  else
                  {
                     gdxGetElemText( PGX, static_cast<int>( std::round( Values[GMS_VAL_LEVEL] ) ), NodeStr.data(), &NodeNr );
                     f << ",\"" << NodeStr << '"' << std::endl;
                  }
               }
               break;

            case dt_par:
               DataLine.at( 2 ) = '"' + SyName + '"';
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
                     DataLine.at( 2 ) = '"' + SyName + '.' + NameSuff.at( i ) + '"';
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
      RunId = StripExt( gdlib::strutilx::ExtractFileNameEx( FnVeda ) );
      if( utils::sameText( gdlib::strutilx::ExtractFileExtEx( FnVeda ), ".csv" ) )
      {
         FnVeda = gdlib::strutilx::ChangeFileExtEx( RunId + "_vd", ".csv" );
         VedaLine = false;
      }
      else
      {
         VedaLine = true;
         FnVeda = gdlib::strutilx::ChangeFileExtEx( FnVeda, ".vd" );
      }
   }
   else
   {
      RunId = StripExt( gdlib::strutilx::ExtractFileNameEx( FnGdx ) );
      FnVeda = gdlib::strutilx::ChangeFileExtEx( RunId, ".vd" );
      VedaLine = true;
   }

   // Empty strings are different for VEDA and CSV files
   if( VedaLine )
      Filler = "\"-\"";
   else
      Filler.clear();

   LoadVdd( FnVdd );
   if( strcmp( ParamStr[4], "" ) != 0 )
   {
      f.open( ParamStr[4] );
      if( !f.is_open() )
      {
         ReportError( "Could not open dump file: " + std::string { ParamStr[4] } );
         ReportError( "Msg: " + std::string { strerror( errno ) } );
      }
      else
      {
         DumpVdd( f );
         f.close();
      }
   }

   std::cout << "--- VEDA Cube: Dimensions=" << NumDimension
             << " Entries=" << NumDataEntry
             << "' Text=" << NumText
             << " SubSets=" << NumSubset << std::endl;

   // Match gdx and cube
   Cnt = 0;
   for( i = 1; i <= NumDataEntry; i++ )
   {
      gdxFindSymbol( PGX, GamsName.at( i ).data(), &SyNr );

      if( SyNr <= 0 )
      {
         ReportError( "Did not find GAMS name " + GamsName.at( i ) + " in GDX file" );
         continue;
      }
      gdxSymbolInfo( PGX, SyNr, SyName.data(), &SyDim, &iSyType );
      SyType = gdxSyType( iSyType );
      gdxSymbolInfoX( PGX, SyNr, &ElemCount, &iDummy, SyText.data() );

      if( SyDim != GamsDim.at( i ) )
      {
         ReportError( "Symbol dimensions do not match for GAMS name " + GamsName.at( i ) );
         ReportError( "GDX dimension=" + std::to_string( SyDim ) + " VDD dimension is " + std::to_string( GamsDim.at( i ) ) );
         continue;
      }

      switch( SyType )
      {
         case dt_set:
            if( GamsSuff.at( i ) != 0 )
            {
               ReportError( "Suffix not allowed for GAMS symbol " + GamsName.at( i ) );
               continue;
            }
            break;

         case dt_par:
            // Apparently gary wants primal/dual pairs also for parameters
            if( !( GamsSuff.at( i ) == 0 || GamsSuff.at( i ) == 5 ) )
            {
               ReportError( "Suffix not allowed for GAMS symbol " + GamsName.at( i ) );
               continue;
            }
            break;

         case dt_var:
         case dt_equ:
            if( GamsSuff.at( i ) == 0 )
            {
               ReportError( "Suffix missing for GAMS symbol " + GamsName.at( i ) );
               continue;
            }

         default:
            // TODO: Throw an error here?
            break;
      }

      Cnt += ElemCount;
   }

   std::cout << "--- VEDA Cube: DataRecords=" << Cnt << std::endl;

   for( i = 1; i <= NumText; i++ )
   {
      gdxFindSymbol( PGX, GamsText.at( i ).data(), &SyNr );
      if( SyNr <= 0 )
      {
         ReportError( "Did not find GAMS text/set name \"" + GamsText.at( i ) + "\" in GDX file" );
         continue;
      }
      gdxSymbolInfo( PGX, SyNr, SyName.data(), &SyDim, &iSyType );
      SyType = gdxSyType( iSyType );
      if( SyType != dt_set )
      {
         ReportError( "Gams text \"" + GamsText.at( i ) + "\" not a set" );
         continue;
      }

      // We allow multiple dimensions now
      // if( SyDim != 1 )
      // {
      //    ReportError( "Gams text \"" + GamsText.at( i ) + "\" not of dimension 1 but " + std::to_string( SyDim ) );
      //    continue;
      // }

      // Instead lets make sure the dimension of gamstext[i] is the same
      // as the number of indices in dimensionstore.textmap
      k = DimensionStore.TextListLength( i );
      assert( k >= 1 );

      // if( SyDim > k && Options.RelaxDimensionAll )
      // {
      //    ReportError( "Dim(" + GamsText.at( i ) + ")=" + std::to_string( SyDim ) + "  Dim in [dimensiontext(all)]=" + std::to_string( k ) );
      //    continue;
      // }

      if( ( ExpandMap.at( i ) == false && SyDim != k ) ||
          ( ExpandMap.at( i ) == true && SyDim != k && !Options.RelaxDimensionAll ) )
      {
         ReportError( "Dimension mismatch: Dim(" + GamsText.at( i ) + ")=" + std::to_string( SyDim ) + "  Dim in [DimensionText]=" + std::to_string( k ) );
         continue;
      }

      // Get the dimension number corresponding to this [DimensionText(all)] entry.
      // Note: SyName is same as GamsText[i].
      DimensionNumber = -1;

      for( j = 1; j <= k; j++ )
      {
         s = DimensionStore.TextList( i, j );
         DimNo = DimensionStore.GetDimensionS( s );

         if( DimNo == Parent )
            continue;
         if( DimensionNumber == -1 )
            DimensionNumber = DimNo;
         if( DimNo != DimensionNumber )
         {
            ReportError( "Record " + SyName + " in [DimensionText(all)] section points to different tabs" );
            continue;
         }

         for( nn = 1; nn <= k - 1; nn++ )
            if( DimensionStore.TextList( i, nn ) == DimensionStore.TextList( i, k ) )
            {
               ReportError( "Duplicate index for " + SyName + " in [DimensionText(all)] section" );
               break;
            }
      }

      if( DimensionNumber == -1 )
      {
         ReportError( "Record " + SyName + " in [DimensionText(all)] has not a valid tab" );
         return 1;
      }

      TextDim.at( i ) = DimensionNumber;
   }

   if( NumErr > 0 )
      return 1;

   // Finally we can dump the veda data

   if( Options.Format == Format_t::FormatCSV )
   {
      FnVedaH = gdlib::strutilx::ChangeFileExtEx( RunId + "_vdheader", ".csv" );
      f.open( FnVedaH );
   }
   else
      f.open( FnVeda );

   if( !f.is_open() )
   {
      ReportError( "Could not open file: " + FnVeda );
      ReportError( "Msg: " + std::string { strerror( errno ) } );
      return 1;
   }

   if( VedaLine )
   {
      // bveda headers
      // f << "*ImportID         - " << DimensionStore.GetTabName( RunPos ) << ':' << RunId << std::endl;
      WriteHeader( f, "GDX2VEDAversion", BuildVersion );
      WriteHeader( f, "ImportID", "Scenario:" + RunId );
      WriteHeader( f, "VEDAFlavor", VEDAFlavor );

      s.clear();
      for( i = 1; i < NumDimension; i++ )
         s += DimensionStore.GetTabName( i ) + ';';
      s += WritePV( ';' );
      WriteHeader( f, "Dimensions", s );

      if( Parent != -1 )
      {
         s.clear();
         for( i = 1; i <= NumChildren; i++ )
         {
            if( i > 1 )
               s += ", ";
            s += DimensionStore.GetTabName( Children.at( i ) ) + ": " +
                 DimensionStore.GetTabName( Parent );
         }
         WriteHeader( f, "ParentDimensions", s );
      }

      Skip = true;
      s.clear();
      for( i = 1; i < NumDimension; i++ )
         if( !Options.SetsAllowedFlag || Options.SetsAllowed.at( i ) )
            // if( i != RunPos || i != AtrPos )
            if( i != AtrPos )
            {
               if( Skip )
                  Skip = false;
               else
                  s += ';';
               s += DimensionStore.GetTabName( i );
            }
      WriteHeader( f, "SetsAllowed", s );

      s.clear();
      for( i = 1; i <= NumDimension; i++ )
         s += DimensionStore.GetTabName( i ) + ":63;";
      s += "PV:20";
      if( Options.ValueDim == 2 )
         s += ";DV:20";
      WriteHeader( f, "FieldSize", s );

      // if( AtrPos > 0 )
      //    f << "*UnitsID          - " << DimensionStore.GetTabName( AtrPos ) << std::endl;
      // else
      //    f << "*UnitsID          - Attributes is missing" << std::endl;

      WriteHeader( f, "NotIndexed", WritePV( ';' ) );
      WriteHeader( f, "ValueDim", WritePV( ';' ) );
      WriteHeader( f, "DefaultValueDim", "PV" );
      WriteHeader( f, "FieldSeparator", "," );
      WriteHeader( f, "TextDelim", "\"" );

      if( Options.Format == Format_t::FormatVeda )
         f << std::endl;
   }
   else
   {
      // csv headers
      for( i = 1; i <= NumDimension; i++ )
         f << '"' << DimensionStore.GetTabName( i ) << "\",";
      f << "\"PV\"";
      if( Options.ValueDim == 2 )
         f << ",\"DV\"";
      f << std::endl;
   }

   if( Options.Format == Format_t::FormatCSV )
   {
      f.close();
      FnVeda = gdlib::strutilx::ChangeFileExtEx( RunId + "_vd", ".csv" );
      f.open( FnVeda );
      if( !f.is_open() )
      {
         ReportError( "Could not open file: " + FnVeda );
         ReportError( "Msg: " + std::string { strerror( errno ) } );
         return 1;
      }
   }

   // Read all UEL names (strings) in array
   for( i = 1; i <= NrUel; i++ )
      gdxUMUelGet( PGX, i, Uels.at( i ).data(), &j );

   // Check literals: must be uel
   CheckLiterals();

   // We will also project all index position to be able to
   // write a .VDE file and attach set text if specified
   // in the [DimensionText] section

   // EK:
   // For every dimension/tab we need a hash table
   // Some dimensions/tabs have tuples.

   for( i = 1; i <= NumDataEntry; i++ )
   {
      gdxFindSymbol( PGX, GamsName.at( i ).data(), &SyNr );
      gdxSymbolInfo( PGX, SyNr, SyName.data(), &SyDim, &iSyType );
      SyType = gdxSyType( iSyType );

      for( k = 1; k < NumDimension; k++ )
         DataLine.at( k ) = Filler;
      if( GamsSuff.at( i ) == 0 )
         j = 1;
      else
         j = GamsSuff.at( i );

      DoSuppressZero = SuppressZero.IndexOf( AtrName.at( i ).data() ) != -1;

      // DataLine.at( RunPos ) = '"' + RunId + '"';
      if( AtrPos > 0 )
         DataLine.at( AtrPos ) = '"' + AtrName.at( i ) + '"';

      for( k = 1; k <= GMS_MAX_INDEX_DIM; k++ )
         LiteralFilter.at( k ) = -1;

      // Prepare for mapping
      DataLineMapping.Clear();

      for( k = 1; k <= SyDim; k++ )
      {
         h = DimensionStore.EntryList( i, k );
         if( h > 0 )
            // TODO: Check -h
            LiteralFilter.at( k ) = LiteralUel.at( -h );
         else
            DataLineMapping.Insert( DimensionStore.GetDimensionI( h ), DimensionStore.GetTupleIndex( h ), k );
      }

      gdxDataReadRawStart( PGX, SyNr, &NrRecs );
      while( gdxDataReadRaw( PGX, Indices, Values, &First ) != 0 )
      {
         if( CheckLiteralFilter( Indices, SyDim ) )
            continue;

         if( DoSuppressZero )
         {
            xx1 = 0;
            xx2 = 0;

            switch( j )
            {
               case 1:
               case 2:
               case 3:
               case 4:
                  xx1 = SpecialValueCheck( Values[j - 1] );
                  break;

               case 5:
                  xx1 = SpecialValueCheck( Values[GMS_VAL_LEVEL] );
                  if( Options.ValueDim == 2 && ( SyType == dt_equ || SyType == dt_var ) )
                     xx2 = SpecialValueCheck( Values[GMS_VAL_MARGINAL] );
                  break;

               default:
                  // TODO: Throw an error here?
                  break;
            }
         }

         if( xx1 == 0 && xx2 == 0 )
            continue;
      }
   }


   return 0;
}

}// namespace gdx2veda

int main( const int argc, const char *argv[] )
{
   return gdx2veda::main( argc, argv );
}
