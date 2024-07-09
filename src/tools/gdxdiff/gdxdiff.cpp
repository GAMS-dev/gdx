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

#include <map>
#include <set>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <limits>

#include "gdxdiff.h"
#include "../library/common.h"
#include "../library/cmdpar.h"
#include "../../gdlib/strutilx.h"
#include "../../gdlib/strhash.h"
#include "../../gdlib/gmsobj.h"
#include "../../rtl/sysutils_p3.h"
// #include "../../rtl/p3process.h"
// Global constants
#include "../../../generated/gclgms.h"

// Increase value to use
#define VERBOSE 0

namespace gdxdiff
{

using tvarvaltype = unsigned int;
using TgdxUELIndex = std::array<int, GMS_MAX_INDEX_DIM>;
using TgdxValues = std::array<double, GMS_VAL_SCALE + 1>;

static library::short_string DiffTmpName;
static gdxHandle_t PGX1 { nullptr }, PGX2 { nullptr }, PGXDIF { nullptr };
static bool diffUELsRegistered;
static std::shared_ptr<gdlib::strhash::TXStrHashList<nullptr_t>> UELTable;
static int staticUELNum;
static double EpsAbsolute, EpsRelative;
static std::map<library::short_string, TStatusCode> StatusTable;
static std::shared_ptr<library::cmdpar::TCmdParams> CmdParams;
static std::set<tvarvaltype> ActiveFields;
// Use FldOnlyVar instead of FldOnly as the variable name
static FldOnly FldOnlyVar;
static tvarvaltype FldOnlyFld;
static bool DiffOnly, CompSetText, matrixFile, ignoreOrder;
static std::shared_ptr<gdlib::gmsobj::TXStrings> IDsOnly;
static bool ShowDefRec, CompDomains;

std::string ValAsString( const gdxHandle_t &PGX, const double V )
{
   constexpr int WIDTH { 14 };
   library::short_string result;
   if( gdxAcronymName( PGX, V, result.data() ) == 0 )
   {
      int iSV;
      gdxMapValue( PGX, V, &iSV );
      if( iSV != sv_normal )
         return gdlib::strutilx::PadLeft( library::gdxSpecialValuesStr( iSV ), WIDTH );
      else
      {
         std::ostringstream oss;
         oss << std::fixed << std::setprecision( 5 ) << V;
         result = oss.str();
         return gdlib::strutilx::PadLeft( result.string(), WIDTH );
      }
   }
   // Empty string will be returned
   return {};
}

void FatalErrorExit( const int ErrNr )
{
   if( !DiffTmpName.empty() && rtl::sysutils_p3::FileExists( DiffTmpName.string() ) )
   {
      gdxClose( PGXDIF );
      rtl::sysutils_p3::DeleteFileFromDisk( DiffTmpName.string() );
   }
   exit( ErrNr );
}

void FatalError( const std::string &Msg, const int ErrNr )
{
   std::cerr << "GDXDIFF error: " << Msg << std::endl;
   FatalErrorExit( ErrNr );
}

void FatalError2( const std::string &Msg1, const std::string &Msg2, const int ErrNr )
{
   std::cerr << "GDXDIFF error: " << Msg1 << std::endl;
   std::cerr << "               " << Msg2 << std::endl;
   FatalErrorExit( ErrNr );
}

void CheckGDXError( const gdxHandle_t &PGX )
{
   int ErrNr { gdxGetLastError( PGX ) };
   if( ErrNr != 0 )
   {
      library::short_string S;
      gdxErrorStr( PGX, ErrNr, S.data() );
      std::cerr << "GDXDIFF GDX Error: " << S << std::endl;
   }
}

void OpenGDX( const library::short_string &fn, gdxHandle_t &PGX )
{
   if( !rtl::sysutils_p3::FileExists( fn.string() ) )
      FatalError( "Input file not found " + fn.string(), static_cast<int>( ErrorCode::ERR_NOFILE ) );

   library::short_string S;
   if( !gdxCreate( &PGX, S.data(), S.length() ) )
      FatalError( "Cannot load GDX library " + S.string(), static_cast<int>( ErrorCode::ERR_LOADDLL ) );

   int ErrNr;
   gdxOpenRead( PGX, fn.data(), &ErrNr );
   if( ErrNr != 0 )
   {
      gdxErrorStr( PGX, ErrNr, S.data() );
      FatalError2( "Problem reading GDX file + " + fn.string(), S.string(), static_cast<int>( ErrorCode::ERR_READGDX ) );
   }

   int NrElem, HighV;
   gdxUMUelInfo( PGX, &NrElem, &HighV );
   gdxUELRegisterMapStart( PGX );
   for( int N { 1 }; N <= NrElem; N++ )
   {
      int NN;
      library::short_string UEL;
      gdxUMUelGet( PGX, N, UEL.data(), &NN );
      NN = UELTable->Add( UEL.data(), UEL.length() );
      gdxUELRegisterMap( PGX, NN, UEL.data() );
   }
   gdxUELRegisterDone( PGX );
   CheckGDXError( PGX );
}

void registerDiffUELs()
{
   // TODO: Check exit value or use return?
   if( diffUELsRegistered ) exit( 0 );

   int maxUEL;
   if( ignoreOrder ) maxUEL = staticUELNum;
   else
      maxUEL = UELTable->Count();

   gdxUELRegisterStrStart( PGXDIF );
   int d;
   for( int N { 1 }; N <= maxUEL; N++ )
      gdxUELRegisterStr( PGXDIF, UELTable->GetString( N ), &d );
   gdxUELRegisterDone( PGXDIF );
   CheckGDXError( PGXDIF );

   diffUELsRegistered = true;
}

void CompareSy( const int Sy1, const int Sy2 )
{
   int Dim, VarEquType;
   gdxSyType ST;
   library::short_string Id;
   bool SymbOpen {};
   TStatusCode Status;
   TgdxValues DefValues;

   auto CheckSymbOpen = [&]() -> bool {
      registerDiffUELs();
      if( Status == TStatusCode::sc_dim10 ) Status = TStatusCode::sc_dim10_diff;
      if( !SymbOpen && Status != TStatusCode::sc_dim10_diff )
      {
         if( FldOnlyVar == FldOnly::fld_yes && ( ST == dt_var || ST == dt_equ ) )
         {
            library::short_string ExplTxt { "Differences Field = " + GamsFieldNames[FldOnlyFld] };
            gdxDataWriteStrStart( PGXDIF, Id.data(), ExplTxt.data(), Dim + 1, dt_par, 0 );
         }
         if( DiffOnly && ( ST == dt_var || ST == dt_equ ) )
            gdxDataWriteStrStart( PGXDIF, Id.data(), "Differences Only", Dim + 2, dt_par, 0 );
         else
            gdxDataWriteStrStart( PGXDIF, Id.data(), "Differences", Dim + 1, ST, VarEquType );
         SymbOpen = true;
      }
      return SymbOpen;
   };

   auto SymbClose = [&]() {
      if( SymbOpen )
      {
         SymbOpen = false;
         CheckGDXError( PGXDIF );
         gdxDataWriteDone( PGXDIF );
         CheckGDXError( PGXDIF );
      }
   };

   auto WriteDiff = [&]( const std::string &Act, const std::string &FldName, const TgdxUELIndex &Keys, const TgdxValues &Vals ) {
      gdxStrIndex_t StrKeys;
      gdxStrIndexPtrs_t StrKeysPtrs;
      GDXSTRINDEXPTRS_INIT( StrKeys, StrKeysPtrs );
      TgdxValues Vals2;

      registerDiffUELs();
      for( int D { 1 }; D <= Dim; D++ )
         strcpy( StrKeys[D], UELTable->GetString( Keys[D] ) );
      if( !DiffOnly && ( ST == dt_var || ST == dt_equ ) )
         strcpy( StrKeys[Dim + 1], Act.data() );
      else
      {
         strcpy( StrKeys[Dim + 1], FldName.data() );
         strcpy( StrKeys[Dim + 2], Act.data() );
      }

#if VERBOSE >= 3
      for( int D { 1 }; D <= Dim + 1; D++ )
      {
         std::cout << StrKeys[D];
         if( D < Dim + 1 ) std::cout << ", ";
      }
      std::cout << std::endl;
#endif

      if( FldOnlyVar == FldOnly::fld_yes && ( ST == dt_var || ST == dt_equ ) )
      {
         Vals2[GMS_VAL_LEVEL] = Vals[FldOnlyFld];
         gdxDataWriteStr( PGXDIF, const_cast<const char **>( StrKeysPtrs ), Vals2.data() );
      }
      else
         gdxDataWriteStr( PGXDIF, const_cast<const char **>( StrKeysPtrs ), Vals.data() );
   };

   auto WriteSetDiff = [&]( const std::string &Act, const TgdxUELIndex &Keys, const library::short_string &S ) {
      gdxStrIndex_t StrKeys;
      gdxStrIndexPtrs_t StrKeysPtrs;
      GDXSTRINDEXPTRS_INIT( StrKeys, StrKeysPtrs );
      TgdxValues Vals;
      int iNode;

      registerDiffUELs();
      for( int D { 1 }; D <= Dim; D++ )
         strcpy( StrKeys[D], UELTable->GetString( Keys[D] ) );
      strcpy( StrKeys[Dim + 1], Act.data() );
      gdxAddSetText( PGXDIF, S.data(), &iNode );
      Vals[GMS_VAL_LEVEL] = iNode;
      gdxDataWriteStr( PGXDIF, const_cast<const char **>( StrKeysPtrs ), Vals.data() );
   };

#if VERBOSE >= 2
   auto WriteValues = [&]( const gdxHandle_t &PGX, const TgdxValues &Vals ) {
      switch( ST )
      {
         case dt_set:
            assert( false && "Should not be called" );
            break;

         case dt_par:
            std::cout << ValAsString( PGX, Vals[GMS_VAL_LEVEL] ) << std::endl;
            break;

         default:
            for( int T { 0 }; T < tvarvaltype_size; T++ )
               std::cout << ValAsString( PGX, Vals[T] ) << ' ';
            std::cout << std::endl;
            break;
      }
   };

   auto WriteKeys = [&]( const TgdxUELIndex &Keys ) {
      registerDiffUELs();
      for( int D { 1 }; D <= Dim; D++ )
      {
         std::cout << ' ' << UELTable->GetString( Keys[D] );
         if( D < Dim ) std::cout << " .";
      }
   };
#endif

   auto DoublesEqual = []( const double V1, const double V2 ) -> bool {
      auto DMin = []( const double a, const double b ) -> double {
         if( a <= b ) return a;
         else
            return b;
      };

      // auto DMax = []( const double a, const double b ) -> double {
      //    if( a >= b ) return a;
      //    else
      //       return b;
      // };

      int iSV1, iSV2;
      gdxMapValue( PGX1, V1, &iSV1 );
      gdxMapValue( PGX2, V2, &iSV2 );

      bool result;
      library::short_string S1, S2;
      double AbsDiff;
      if( iSV1 == sv_normal )
      {
         if( iSV2 == sv_normal )
         {
            if( gdxAcronymName( PGX1, V1, S1.data() ) != 0 && gdxAcronymName( PGX2, V2, S2.data() ) != 0 )
               result = gdlib::strutilx::StrUEqual( S1.string(), S2.string() );
            else
            {
               AbsDiff = abs( V1 - V2 );
               if( AbsDiff <= EpsAbsolute ) result = true;
               else if( EpsRelative > 0 )
                  result = AbsDiff / ( 1 + DMin( abs( V1 ), abs( V2 ) ) ) <= EpsRelative;
               else
                  result = false;
            }
         }
         else
            result = iSV2 == sv_valeps && EpsAbsolute > 0 && abs( V1 ) <= EpsAbsolute;
      }
      else
      {
         if( iSV2 == sv_normal ) result = iSV1 == sv_valeps && EpsAbsolute > 0 && abs( V2 ) <= EpsAbsolute;
         else
            result = iSV1 == iSV2;
      }
      return result;
   };

   auto CheckParDifference = [&]( const TgdxUELIndex &Keys, const TgdxValues &V1, const TgdxValues &V2 ) -> bool {
      bool result { true };
      if( ST == dt_par )
         result = DoublesEqual( V1[GMS_VAL_LEVEL], V2[GMS_VAL_LEVEL] );
      else if( FldOnlyVar == FldOnly::fld_yes )
         result = DoublesEqual( V1[FldOnlyFld], V2[FldOnlyFld] );
      else
      {
         for( int T { 0 }; T < tvarvaltype_size; T++ )
         {
            if( ActiveFields.find( static_cast<tvarvaltype>( T ) ) != ActiveFields.end() && !DoublesEqual( V1[T], V2[T] ) )
            {
               result = false;
               break;
            }
         }
      }
      if( !result )
      {
#if VERBOSE >= 2
         std::cout << "Different ";
         WriteKeys( Keys );
         std::cout << std::endl;
         WriteValues( PGX1, V1 );
         WriteValues( PGX2, V2 );
#endif

         // TODO: Check exit value or use return?
         if( !CheckSymbOpen() ) exit( 0 );
         if( !( DiffOnly && ( ST == dt_var || ST == dt_equ ) ) )
         {
            WriteDiff( c_dif1, "", Keys, V1 );
            WriteDiff( c_dif2, "", Keys, V2 );
         }
         else
         {
            for( int T { 0 }; T < tvarvaltype_size; T++ )
            {
               if( ActiveFields.find( static_cast<tvarvaltype>( T ) ) == ActiveFields.end() ) continue;
               if( DoublesEqual( V1[T], V2[T] ) ) continue;

               TgdxValues Vals;
               Vals[GMS_VAL_LEVEL] = V1[T];
               WriteDiff( c_dif1, GamsFieldNames[T], Keys, Vals );
               Vals[GMS_VAL_LEVEL] = V2[T];
               WriteDiff( c_dif2, GamsFieldNames[T], Keys, Vals );
            }
         }
      }
      return result;
   };

   auto CheckSetDifference = [&]( const TgdxUELIndex &Keys, const int txt1, const int txt2 ) -> bool {
      library::short_string S1, S2;
      int iNode;
      if( txt1 == 0 ) S1.clear();
      else
         gdxGetElemText( PGX1, txt1, S1.data(), &iNode );
      if( txt2 == 0 ) S2.clear();
      else
         gdxGetElemText( PGX2, txt2, S2.data(), &iNode );

      bool result { S1 == S2 };
      if( !result )
      {
#if VERBOSE >= 2
         std::cout << "Associated text is different ";
         WriteKeys( Keys );
         std::cout << std::endl;
         std::cout << S1 << std::endl;
         std::cout << S2 << std::endl;
#endif

         // TODO: Check exit value or use return?
         if( !CheckSymbOpen() ) exit( 0 );
         WriteSetDiff( c_dif1, Keys, S1 );
         WriteSetDiff( c_dif2, Keys, S2 );
      }
      return result;
   };

   auto ShowInsert = [&]( const std::string &Act, const TgdxUELIndex &Keys, TgdxValues &Vals ) {
      // We check if this insert has values we want to ignore
      bool Eq {};
      switch( ST )
      {
         case dt_par:
            Eq = DoublesEqual( Vals[GMS_VAL_LEVEL], 0 );
            break;

         case dt_var:
         case dt_equ:
            Eq = true;
            for( int T { 0 }; T < tvarvaltype_size; T++ )
            {
               if( ActiveFields.find( static_cast<tvarvaltype>( T ) ) != ActiveFields.end() && !DoublesEqual( Vals[T], DefValues[T] ) )
               {
                  Eq = false;
                  break;
               }
            }
            break;

         default:
            // TODO: Print error message?
            break;
      }

      // TODO: Check exit value or use return?
      if( Eq && !ShowDefRec ) exit( 0 );

      if( Status == TStatusCode::sc_same ) Status = TStatusCode::sc_key;
      if( Status == TStatusCode::sc_dim10 ) Status = TStatusCode::sc_dim10_diff;

      // TODO: Check exit value or use return?
      if( Status == TStatusCode::sc_dim10_diff || !CheckSymbOpen() ) exit( 0 );

#if VERBOSE >= 2
      std::cout << "Insert: " << Act << ' ';
#endif

      if( ST == dt_set && Vals[GMS_VAL_LEVEL] != 0 )
      {
         library::short_string stxt;
         int N;
         if( Act == c_ins1 )
            gdxGetElemText( PGX1, static_cast<int>( round( Vals[GMS_VAL_LEVEL] ) ), stxt.data(), &N );
         else
            gdxGetElemText( PGX2, static_cast<int>( round( Vals[GMS_VAL_LEVEL] ) ), stxt.data(), &N );
         gdxAddSetText( PGXDIF, stxt.data(), &N );
         Vals[GMS_VAL_LEVEL] = N;
      }

      if( !( DiffOnly && ( ST == dt_var || ST == dt_equ ) ) )
         WriteDiff( Act, "", Keys, Vals );
      else
      {
         TgdxValues Vals2;
         for( int T { 0 }; T < tvarvaltype_size; T++ )
         {
            if( ActiveFields.find( static_cast<tvarvaltype>( T ) ) == ActiveFields.end() )
               continue;
            Vals2[GMS_VAL_LEVEL] = Vals[T];
            WriteDiff( Act, GamsFieldNames[T], Keys, Vals2 );
         }
      }
   };

   int Dim2, AFDim, iST, iST2, R1Last, R2Last, C, acount;
   gdxSyType ST2;
   bool Flg1, Flg2, Eq, DomFlg;
   TgdxUELIndex Keys1, Keys2;
   TgdxValues Vals1, Vals2;
   library::short_string stxt;
   gdxStrIndex_t DomSy1;
   gdxStrIndexPtrs_t DomSy1Ptrs;
   GDXSTRINDEXPTRS_INIT( DomSy1, DomSy1Ptrs );
   gdxStrIndex_t DomSy2;
   gdxStrIndexPtrs_t DomSy2Ptrs;
   GDXSTRINDEXPTRS_INIT( DomSy2, DomSy2Ptrs );

   SymbOpen = false;
   gdxSymbolInfo( PGX1, Sy1, Id.data(), &Dim, &iST );
   ST = static_cast<gdxSyType>( iST );
   if( ST == dt_alias ) ST = dt_set;
   // We do nothing with type in file2
   gdxSymbolInfoX( PGX1, Sy1, &acount, &VarEquType, stxt.data() );

   gdxSymbolInfo( PGX2, Sy2, Id.data(), &Dim2, &iST2 );
   ST2 = static_cast<gdxSyType>( iST2 );
   if( ST2 == dt_alias ) ST2 = dt_set;
   Status = TStatusCode::sc_same;

   if( Dim != Dim2 || ST != ST2 )
   {
      std::cout << "*** symbol = " << Id << " cannot be compared" << std::endl;
      if( ST != ST2 )
      {
         std::cout << "Typ1 = " << library::gdxDataTypStrL( ST ) << ", Typ2 = " << library::gdxDataTypStrL( ST2 ) << std::endl;
         Status = TStatusCode::sc_typ;
      }
      if( Dim != Dim2 )
      {
         std::cout << "Dim1 = " << Dim << ", Dim2 = " << Dim2 << std::endl;
         if( Status == TStatusCode::sc_same ) Status = TStatusCode::sc_dim;
      }
      goto label999;
   }

   // Check domains
   if( CompDomains && Dim > 0 )
   {
      gdxSymbolGetDomainX( PGX1, Sy1, DomSy1Ptrs );
      gdxSymbolGetDomainX( PGX2, Sy2, DomSy2Ptrs );
      DomFlg = false;
      for( int D { 1 }; D <= Dim; D++ )
         if( gdlib::strutilx::StrUEqual( DomSy1[D], DomSy2[D] ) )
         {
            DomFlg = true;
            break;
         }
      if( DomFlg )
      {
         Status = TStatusCode::sc_domain;

#if VERBOSE >= 1
         std::cout << "Domain differences for symbol = " << Id << std::endl;
         for( int D { 1 }; D <= Dim; D++ )
         {
            if( gdlib::strutilx::StrUEqual( DomSy1[D], DomSy2[D] ) )
               continue;
            std::cout << gdlib::strutilx::PadRight( std::to_string( D ), 2 ) << ' ' << DomSy1[D] << ' ' << DomSy2[D] << std::endl;
         }
         std::cout << std::endl;
#endif

         goto label999;
      }
   }

#if VERBOSE >= 1
   std::cout << library::gdxDataTypStrL( ST ) << ' ' << Id << std::endl;
#endif

   // Create default record for this type
   if( ST == dt_var || ST == dt_equ )
   {
      if( ST == dt_var )
         std::copy( std::begin( gmsDefRecVar[VarEquType] ), std::end( gmsDefRecVar[VarEquType] ), std::begin( DefValues ) );
      else
         std::copy( std::begin( gmsDefRecEqu[VarEquType] ), std::end( gmsDefRecEqu[VarEquType] ), std::begin( DefValues ) );
   }

   if( Dim >= GMS_MAX_INDEX_DIM || ( DiffOnly && Dim - 1 >= GMS_MAX_INDEX_DIM ) )
      Status = TStatusCode::sc_dim10;

   if( matrixFile )
   {
      Flg1 = gdxDataReadRawStart( PGX1, Sy1, &R1Last ) != 0;
      if( Flg1 ) Flg1 = gdxDataReadRaw( PGX1, Keys1.data(), Vals1.data(), &AFDim ) != 0;

      Flg2 = gdxDataReadRawStart( PGX2, Sy2, &R2Last ) != 0;
      if( Flg2 ) Flg2 = gdxDataReadRaw( PGX2, Keys2.data(), Vals2.data(), &AFDim ) != 0;
   }
   else
   {
      Flg1 = gdxDataReadMapStart( PGX1, Sy1, &R1Last ) != 0;
      if( Flg1 ) Flg1 = gdxDataReadMap( PGX1, 0, Keys1.data(), Vals1.data(), &AFDim ) != 0;

      Flg2 = gdxDataReadMapStart( PGX2, Sy2, &R2Last ) != 0;
      if( Flg2 ) Flg2 = gdxDataReadMap( PGX2, 0, Keys2.data(), Vals2.data(), &AFDim ) != 0;
   }

   while( Flg1 && Flg2 )
   {
      C = 0;
      if( Dim > 0 )
      {
         for( int D { 1 }; D <= Dim; D++ )
         {
            C = Keys1[D] - Keys2[D];
            if( C != 0 ) break;
         }
      }
      if( C == 0 )
      {
         if( ST == dt_set )
         {
            if( CompSetText )
               Eq = CheckSetDifference( Keys1, static_cast<int>( round( Vals1[GMS_VAL_LEVEL] ) ), static_cast<int>( round( Vals2[GMS_VAL_LEVEL] ) ) );
            else
               Eq = true;
         }
         else
            Eq = CheckParDifference( Keys1, Vals1, Vals2 );

         if( !Eq && Status == TStatusCode::sc_same )
            Status = TStatusCode::sc_data;

         if( matrixFile )
         {
            Flg1 = gdxDataReadRaw( PGX1, Keys1.data(), Vals1.data(), &AFDim ) != 0;
            Flg2 = gdxDataReadRaw( PGX2, Keys2.data(), Vals2.data(), &AFDim ) != 0;
         }
         else
         {
            Flg1 = gdxDataReadMap( PGX1, 0, Keys1.data(), Vals1.data(), &AFDim ) != 0;
            Flg2 = gdxDataReadMap( PGX2, 0, Keys2.data(), Vals2.data(), &AFDim ) != 0;
         }
      }
      if( C < 0 )
      {
         ShowInsert( c_ins1, Keys1, Vals1 );
         if( matrixFile )
            Flg1 = gdxDataReadRaw( PGX1, Keys1.data(), Vals1.data(), &AFDim ) != 0;
         else
            Flg1 = gdxDataReadMap( PGX1, 0, Keys1.data(), Vals1.data(), &AFDim ) != 0;
      }
      else
      {
         ShowInsert( c_ins2, Keys2, Vals2 );
         if( matrixFile )
            Flg2 = gdxDataReadRaw( PGX2, Keys2.data(), Vals2.data(), &AFDim ) != 0;
         else
            Flg2 = gdxDataReadMap( PGX2, 0, Keys2.data(), Vals2.data(), &AFDim ) != 0;
      }
      // Change in status happens inside ShowInsert
   }

   while( Flg1 )
   {
      ShowInsert( c_ins1, Keys1, Vals1 );
      if( matrixFile )
         Flg1 = gdxDataReadRaw( PGX1, Keys1.data(), Vals1.data(), &AFDim ) != 0;
      else
         Flg1 = gdxDataReadMap( PGX1, 0, Keys1.data(), Vals1.data(), &AFDim ) != 0;
   }

   while( Flg2 )
   {
      ShowInsert( c_ins2, Keys2, Vals2 );
      if( matrixFile )
         Flg2 = gdxDataReadRaw( PGX2, Keys2.data(), Vals2.data(), &AFDim ) != 0;
      else
         Flg2 = gdxDataReadMap( PGX2, 0, Keys2.data(), Vals2.data(), &AFDim ) != 0;
   }

   SymbClose();

label999:
   if( !( Status == TStatusCode::sc_same || Status == TStatusCode::sc_dim10 ) )
      StatusTable[Id] = Status;
}

bool GetAsDouble( const library::short_string &S, double &V )
{
   int k;
   utils::val( S.string(), V, k );
   bool result { k == 0 && V >= 0 };
   if( !result ) V = 0;
   return result;
}

void Usage()
{
   std::cout << "gdxdiff: GDX file differ" << '\n'
             // TODO: Replace blank line with function output
             // << gdlGetAuditLine() << '\n'
             << '\n'
             << "Usage: " << '\n'
             << "   gdxdiff file1.gdx file2.gdx [diffile.gdx] [options]" << '\n'
             << "   Options:" << '\n'
             << "      Eps     = Val       epsilon for comparison" << '\n'
             << "      RelEps  = Val       epsilon for relative comparison" << '\n'
             << "      Field   = gamsfield (L, M, Up, Lo, Prior, Scale or All" << '\n'
             << "      FldOnly             write var or equ as parameter for selected field" << '\n'
             << "      DiffOnly            write var or equ as parameter with field as an extra dimension" << '\n'
             << "      CmpDefaults         compare default values" << '\n'
             << "      CmpDomains          compare domains" << '\n'
             << "      MatrixFile          compare GAMS matrix files in GDX format" << '\n'
             << "      IgnoreOrder         ignore UEL order of input files - reduces size of output file" << '\n'
             << "      SetDesc = Y|N       compare explanatory texts for set elements, activated by default (=Y)" << '\n'
             << "      Id      = one or more identifiers; only ids listed will be compared" << '\n'
             << "   The .gdx file extension is the default" << std::endl;
}

// Function is empty in Delphi code
// void CopyAcronyms( const gdxHandle_t &PGX ) {}

void CheckFile( library::short_string &fn )
{
   if( !rtl::sysutils_p3::FileExists( fn.string() ) && gdlib::strutilx::ExtractFileExtEx( fn.string() ).empty() )
      fn = gdlib::strutilx::ChangeFileExtEx( fn.string(), ".gdx" );
}

int main( const int argc, const char *argv[] )
{
   int ErrorCode, ErrNr, Dim, iST, StrNr;
   library::short_string S, ID, InFile1, InFile2, DiffFileName;
   std::map<library::short_string, int> IDTable;
   bool UsingIDE, RenameOK;
   gdxStrIndex_t StrKeys;
   gdxStrIndexPtrs_t StrKeysPtrs;
   GDXSTRINDEXPTRS_INIT( StrKeys, StrKeysPtrs );
   TgdxValues StrVals;

   // TODO: Remove?
   // gdlSetSystemName( 'GDXDIFF' );
   // if( argv[1] == "AUDIT" )
   // {
   //    std::cout << gdlGetAuditLine() << std::endl;
   //    // TODO: Check exit value or use return?
   //    exit( 0 );
   // }

   // So we can check later
   DiffTmpName.clear();

   CmdParams = std::make_unique<library::cmdpar::TCmdParams>();

   CmdParams->AddParam( static_cast<int>( library::cmdpar::CmdParamStatus::kp_input ), "I" );
   CmdParams->AddParam( static_cast<int>( library::cmdpar::CmdParamStatus::kp_input ), "Input" );
   CmdParams->AddParam( static_cast<int>( KP::kp_output ), "Output" );
   CmdParams->AddParam( static_cast<int>( KP::kp_output ), "O" );
   CmdParams->AddParam( static_cast<int>( KP::kp_eps ), "Eps" );
   CmdParams->AddParam( static_cast<int>( KP::kp_releps ), "RelEps" );
   CmdParams->AddParam( static_cast<int>( KP::kp_cmpfld ), "Field" );
   CmdParams->AddParam( static_cast<int>( KP::kp_settext ), "SetDesc" );
   CmdParams->AddParam( static_cast<int>( KP::kp_ide ), "IDE" );
   CmdParams->AddParam( static_cast<int>( KP::kp_id ), "ID" );

   CmdParams->AddKeyWord( static_cast<int>( KP::kp_fldonly ), "FldOnly" );
   CmdParams->AddKeyWord( static_cast<int>( KP::kp_diffonly ), "DiffOnly" );
   CmdParams->AddKeyWord( static_cast<int>( KP::kp_showdef ), "CmpDefaults" );
   CmdParams->AddKeyWord( static_cast<int>( KP::kp_cmpdomain ), "CmpDomains" );
   CmdParams->AddKeyWord( static_cast<int>( KP::kp_matrixfile ), "MatrixFile" );
   CmdParams->AddKeyWord( static_cast<int>( KP::kp_ignoreOrd ), "IgnoreOrder" );

   if( !CmdParams->CrackCommandLine( argc, argv ) )
   {
      Usage();
      exit( static_cast<int>( ErrorCode::ERR_USAGE ) );
   }

   ErrorCode = 0;
   UsingIDE = false;
   matrixFile = false;
   diffUELsRegistered = false;

   library::cmdpar::TParamRec Parameter { CmdParams->GetParams( 0 ) };
   if( Parameter.Key == static_cast<int>( library::cmdpar::CmdParamStatus::kp_input ) )
      InFile1 = Parameter.KeyS;
   // else
   //    InFile1.clear();

   Parameter = CmdParams->GetParams( 1 );
   if( Parameter.Key == static_cast<int>( library::cmdpar::CmdParamStatus::ke_unknown ) )
      InFile2 = Parameter.KeyS;
   // else
   //    InFile2.clear();

   if( InFile1.empty() || InFile2.empty() )
      ErrorCode = 1;

   if( !CmdParams->HasParam( static_cast<int>( KP::kp_output ), DiffFileName ) )
   {
      Parameter = CmdParams->GetParams( 2 );
      if( Parameter.Key == static_cast<int>( library::cmdpar::CmdParamStatus::ke_unknown ) )
         DiffFileName = Parameter.KeyS;
      // else
      //    DiffFileName.clear();
   }

   if( DiffFileName.empty() )
      DiffFileName = "diffile";

   if( gdlib::strutilx::ExtractFileExtEx( DiffFileName.string() ).empty() )
      DiffFileName = gdlib::strutilx::ChangeFileExtEx( DiffFileName.string(), ".gdx" );

   if( !CmdParams->HasParam( static_cast<int>( KP::kp_eps ), S ) )
      EpsAbsolute = 0;
   else if( GetAsDouble( S, EpsAbsolute ) )
   {
      if( EpsAbsolute < 0 )
      {
         std::cout << "Eps cannot be negative" << std::endl;
         ErrorCode = 2;
      }
   }
   else
   {
      std::cout << "Bad value for Eps = " << S << std::endl;
      ErrorCode = 2;
   }

   if( !CmdParams->HasParam( static_cast<int>( KP::kp_releps ), S ) )
      EpsRelative = 0;
   else if( GetAsDouble( S, EpsRelative ) )
   {
      if( EpsRelative < 0 )
      {
         std::cout << "RelEps cannot be negative" << std::endl;
         ErrorCode = 2;
      }
   }
   else
   {
      std::cout << "Bad value for RelEps = " << S << std::endl;
      ErrorCode = 2;
   }

   DiffOnly = CmdParams->HasKey( static_cast<int>( KP::kp_diffonly ) );
   FldOnlyVar = FldOnly::fld_no;
   matrixFile = CmdParams->HasKey( static_cast<int>( KP::kp_matrixfile ) );
   ignoreOrder = CmdParams->HasKey( static_cast<int>( KP::kp_ignoreOrd ) );

   if( !CmdParams->HasParam( static_cast<int>( KP::kp_cmpfld ), S ) )
      ActiveFields = { GMS_VAL_LEVEL, GMS_VAL_MARGINAL, GMS_VAL_LOWER, GMS_VAL_UPPER, GMS_VAL_SCALE };
   else
   {
      if( gdlib::strutilx::StrUEqual( S.string(), "All" ) )
         ActiveFields = { GMS_VAL_LEVEL, GMS_VAL_MARGINAL, GMS_VAL_LOWER, GMS_VAL_UPPER, GMS_VAL_SCALE };
      else if( gdlib::strutilx::StrUEqual( S.string(), "L" ) )
      {
         FldOnlyFld = GMS_VAL_LEVEL;
         FldOnlyVar = FldOnly::fld_maybe;
      }
      else if( gdlib::strutilx::StrUEqual( S.string(), "M" ) )
      {
         FldOnlyFld = GMS_VAL_MARGINAL;
         FldOnlyVar = FldOnly::fld_maybe;
      }
      else if( gdlib::strutilx::StrUEqual( S.string(), "Up" ) )
      {
         FldOnlyFld = GMS_VAL_UPPER;
         FldOnlyVar = FldOnly::fld_maybe;
      }
      else if( gdlib::strutilx::StrUEqual( S.string(), "Lo" ) )
      {
         FldOnlyFld = GMS_VAL_LOWER;
         FldOnlyVar = FldOnly::fld_maybe;
      }
      else if( gdlib::strutilx::StrUEqual( S.string(), "Prior" ) || gdlib::strutilx::StrUEqual( S.string(), "Scale" ) )
      {
         FldOnlyFld = GMS_VAL_SCALE;
         FldOnlyVar = FldOnly::fld_maybe;
      }
      else
      {
         std::cout << "Bad field name = " << S << std::endl;
         ErrorCode = 4;
      }

      if( FldOnlyVar == FldOnly::fld_maybe )
         ActiveFields = { FldOnlyFld };
   }

   if( CmdParams->HasKey( static_cast<int>( KP::kp_fldonly ) ) )
   {
      if( FldOnlyVar == FldOnly::fld_maybe )
      {
         FldOnlyVar = FldOnly::fld_yes;
         if( DiffOnly )
         {
            // TODO: Change combines to combined?
            std::cout << "Diff only cannot be combines with FldOnly" << std::endl;
            ErrorCode = 4;
         }
      }
      else
      {
         std::cout << "FldOnly option used with a single field comparison" << std::endl;
         ErrorCode = 4;
      }
   }

   if( CmdParams->HasParam( static_cast<int>( KP::kp_ide ), S ) )
      UsingIDE = !S.empty() && ( S[0] == 'Y' || S[0] == 'y' || S[0] == '1' );

   CompSetText = true;

   // This is a mistake; should be HasKey but leave it
   if( CmdParams->HasParam( static_cast<int>( KP::kp_settext ), S ) )
   {
      S = gdlib::strutilx::UpperCase( S.string() );
      if( S == "0" || S == "N" || S == "F" )
         CompSetText = false;
      else if( S.empty() || S == "1" || S == "Y" || S == "T" )
         CompSetText = true;
      else
      {
         std::cout << "Bad value for CompSetText = " << S << std::endl;
         ErrorCode = 4;
      }
   }

   ShowDefRec = CmdParams->HasKey( static_cast<int>( KP::kp_showdef ) );
   CompDomains = CmdParams->HasKey( static_cast<int>( KP::kp_cmpdomain ) );

   if( CmdParams->HasParam( static_cast<int>( KP::kp_id ), S ) )
   {
      IDsOnly = std::make_unique<gdlib::gmsobj::TXStrings>();
      for( int N { 0 }; N < CmdParams->GetParamCount(); N++ )
      {
         if( CmdParams->GetParams( N ).Key == static_cast<int>( KP::kp_id ) )
         {
            S = utils::trim( CmdParams->GetParams( N ).KeyS );
            // std::cout << S << std::endl;
            while( !S.empty() )
            {
               int k { gdlib::strutilx::LChSetPos( std::array<char, 2> { ',', ' ' }.data(), S.data(), static_cast<int>( S.length() ) ) };
               if( k == 0 )
               {
                  ID = S;
                  S.clear();
               }
               else
               {
                  ID = S.string().substr( 0, k - 1 );
                  S = S.string().erase( 0, k );
                  S = utils::trim( S.string() );
               }
               ID = utils::trim( ID.string() );
               if( !ID.empty() && IDsOnly->IndexOf( ID.data() ) < 0 )
               {
                  // std::cout << "Include Id: " << ID << std::endl;
                  IDsOnly->Add( ID.data(), ID.length() );
               }
            }
         }
      }
   }

   // if( IDsOnly != nullptr && IDsOnly->GetCount() == 0 )
   //    // Like ID = ""
   //    FreeAndNil( IDsOnly );

   // We removed this but not sure why
   // if( rtl::sysutils_p3::FileExists( DiffFileName ) )
   //    rtl::sysutils_p3::DeleteFileFromDisk( DiffFileName );

   // Parameter errors
   if( ErrorCode > 0 )
   {
      std::cout << std::endl;
      Usage();
      exit( static_cast<int>( ErrorCode::ERR_USAGE ) );
   }

   // TODO: Remove?
   // std::cout << gdlGetAuditLine() << '\n';

   CheckFile( InFile1 );
   CheckFile( InFile2 );

   // TODO: Remove spaces before colons?
   std::cout << "File1 : " << InFile1 << std::endl;
   std::cout << "File2 : " << InFile2 << std::endl;

   if( IDsOnly != nullptr )
   {
      std::cout << "Id    :";
      for( int N { 0 }; N < IDsOnly->GetCount(); N++ )
         std::cout << ' ' << IDsOnly->GetConst( N );
      std::cout << std::endl;
   }

   library::short_string S2;
   if( !gdxCreate( &PGXDIF, S2.data(), S2.length() ) )
      FatalError( "Unable to load GDX library: " + S2.string(), static_cast<int>( ErrorCode::ERR_LOADDLL ) );

   // Temporary file name
   for( int N { 1 }; N <= std::numeric_limits<int>::max(); N++ )
   {
      DiffTmpName = "tmpdifffile" + std::to_string( N ) + ".gdx";
      if( !rtl::sysutils_p3::FileExists( DiffTmpName.string() ) )
         break;
   }

   gdxOpenWrite( PGXDIF, DiffTmpName.data(), "GDXDIFF", &ErrNr );
   if( ErrNr != 0 )
   {
      int N { gdxGetLastError( PGXDIF ) };
      // Nil is used instead of PGXDIF in Delphi code
      gdxErrorStr( PGXDIF, N, S.data() );
      FatalError2( "Cannot create file: " + DiffTmpName.string(), S.string(), static_cast<int>( ErrorCode::ERR_WRITEGDX ) );
   }

   UELTable = std::make_unique<gdlib::strhash::TXStrHashList<nullptr_t>>();
   UELTable->OneBased = true;
   gdxStoreDomainSetsSet( PGXDIF, false );

   UELTable->Add( c_ins1.data(), c_ins1.length() );
   UELTable->Add( c_ins2.data(), c_ins2.length() );
   UELTable->Add( c_dif1.data(), c_dif1.length() );
   UELTable->Add( c_dif2.data(), c_dif2.length() );

   if( DiffOnly )
      for( int T { 0 }; T < tvarvaltype_size; T++ )
         UELTable->Add( GamsFieldNames[T].data(), GamsFieldNames[T].length() );

   staticUELNum = UELTable->Count();

   // These calls register UELs
   OpenGDX( InFile1, PGX1 );
   OpenGDX( InFile2, PGX2 );

   {
      int N { 1 };
      while( gdxSymbolInfo( PGX1, N, ID.data(), &Dim, &iST ) != 0 )
      {
         if( IDsOnly == nullptr || IDsOnly->IndexOf( ID.data() ) >= 0 )
            IDTable[ID] = N;
         N++;
      }
   }

   for( const auto &pair: IDTable )
   {
      int NN;
      if( gdxFindSymbol( PGX2, pair.first.data(), &NN ) != 0 )
         CompareSy( pair.second, NN );
      else
         StatusTable[pair.first] = TStatusCode::sc_notf2;
   }

   // Find symbols in file 2 that are not in file 1
   IDTable.clear();

   {
      int N { 1 };
      while( gdxSymbolInfo( PGX2, N, ID.data(), &Dim, &iST ) != 0 )
      {
         if( IDsOnly == nullptr || IDsOnly->IndexOf( ID.data() ) >= 0 )
            IDTable[ID] = N;
         N++;
      }
   }

   for( const auto &pair: IDTable )
   {
      int NN;
      if( gdxFindSymbol( PGX1, pair.first.data(), &NN ) == 0 )
         StatusTable[pair.first] = TStatusCode::sc_notf1;
   }

   if( StatusTable.empty() )
      std::cout << "No differences found" << std::endl;
   else
   {
      std::cout << "Summary of differences:" << std::endl;
      // Find longest ID
      int NN { 1 };
      for( const auto &pair: StatusTable )
         if( pair.first.length() > NN )
            NN = static_cast<int>( pair.first.length() );
      for( const auto &pair: StatusTable )
         std::cout << gdlib::strutilx::PadRight( pair.first.string(), NN ) << "   "
                   << StatusText.at( static_cast<int>( pair.second ) ) << std::endl;
   }

   // CopyAcronyms( PGX1 );
   // CopyAcronyms( PGX2 );

   // Write the two filenames used as explanatory texts
   {

      int N {};
      int NN;
      do
      {
         ID = "FilesCompared";
         if( N > 0 )
            ID += std::to_string( '_' ) + std::to_string( N );
         if( gdxFindSymbol( PGXDIF, ID.data(), &NN ) == 0 )
            break;
         N++;
      } while( true );
   }

   gdxDataWriteStrStart( PGXDIF, ID.data(), "", 1, dt_set, 0 );
   strcpy( StrKeys[1], "File1" );
   gdxAddSetText( PGXDIF, InFile1.data(), &StrNr );
   StrVals[GMS_VAL_LEVEL] = StrNr;
   gdxDataWriteStr( PGXDIF, const_cast<const char **>( StrKeysPtrs ), StrVals.data() );
   strcpy( StrKeys[1], "File2" );
   gdxAddSetText( PGXDIF, InFile2.data(), &StrNr );
   StrVals[GMS_VAL_LEVEL] = StrNr;
   gdxDataWriteStr( PGXDIF, const_cast<const char **>( StrKeysPtrs ), StrVals.data() );
   gdxDataWriteDone( PGXDIF );

   // Note that input files are not closed at this point; so if we wrote
   // to an input file, the delete will fail and we keep the original input file alive
   gdxClose( PGX1 );
   gdxClose( PGX2 );
   gdxClose( PGXDIF );

   if( !rtl::sysutils_p3::FileExists( DiffFileName.string() ) )
      RenameOK = true;
   else
   {
      RenameOK = rtl::sysutils_p3::DeleteFileFromDisk( DiffFileName.string() );
#if defined( _WIN32 )
      if( !RenameOK )
      {
         int ShellCode;
         if( rtl::p3process::P3ExecP( "IDECmds.exe ViewClose \"" + DiffFileName.string() + "\"", ShellCode ) == 0 )
            RenameOK = rtl::sysutils_p3::DeleteFileFromDisk( DiffFileName.string() );
      }
#endif
   }

   if( RenameOK )
   {
      rtl::sysutils_p3::RenameFile( DiffTmpName.string(), DiffFileName.string() );
      RenameOK = rtl::sysutils_p3::FileExists( DiffFileName.string() );
   }

   int ExitCode {};
   if( !RenameOK )
   {
      std::cout << "Could not rename " << DiffTmpName << " to " << DiffFileName << std::endl;
      DiffFileName = DiffTmpName;
      ExitCode = static_cast<int>( ErrorCode::ERR_RENAME );
   }
   std::cout << "Output: " << DiffFileName << std::endl;

   // UnloadGDXLibrary;
   if( UsingIDE )
      std::cout << "GDXDiff file written: " << DiffFileName << "[FIL:\"" << DiffFileName << "\",0,0]" << std::endl;
   std::cout << "GDXDiff finished" << std::endl;
   // FreeAndNil( UELTable );

   if( ExitCode == 0 && !StatusTable.empty() )
      exit( static_cast<int>( ErrorCode::ERR_DIFFERENT ) );

   return ExitCode;
}

}// namespace gdxdiff

int main( const int argc, const char *argv[] )
{
   return gdxdiff::main( argc, argv );
}
