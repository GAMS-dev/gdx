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
#include "../../rtl/p3process.h"
// Global constants
#include "../../../generated/gclgms.h"

// Increase value to use
#define VERBOSE 0

namespace gdxdiff
{

static std::string DiffTmpName {};
static std::shared_ptr<gdx::TGXFileObj> PGX1, PGX2, PGXDIF;
static bool diffUELsRegistered;
static std::shared_ptr<gdlib::strhash::TXStrHashList<nullptr_t>> UELTable;
static int staticUELNum;
static double EpsAbsolute, EpsRelative;
static std::map<std::string, TStatusCode> StatusTable;
static std::shared_ptr<library::cmdpar::TCmdParams> CmdParams;
static std::set<gdx::tvarvaltype> ActiveFields;
// Use FldOnlyVar instead of FldOnly as the variable name
static FldOnly FldOnlyVar;
static gdx::tvarvaltype FldOnlyFld;
static bool DiffOnly, CompSetText, matrixFile, ignoreOrder;
static std::shared_ptr<gdlib::gmsobj::TXStrings> IDsOnly;
static bool ShowDefRec, CompDomains;

std::string ValAsString( const std::shared_ptr<gdx::TGXFileObj> &PGX, const double V )
{
   constexpr int WIDTH { 14 };
   std::string result;
   if( PGX->gdxAcronymName( V, result.data() ) == 0 )
   {
      int iSV;
      PGX->gdxMapValue( V, iSV );
      if( iSV != sv_normal )
         return gdlib::strutilx::PadLeft( library::gdxSpecialValuesStr( iSV ), WIDTH );
      else
      {
         std::ostringstream oss;
         oss << std::fixed << std::setprecision( 5 ) << V;
         result = oss.str();
         return gdlib::strutilx::PadLeft( result, WIDTH );
      }
   }
   // Empty string will be returned
   return result;
}

void FatalErrorExit( const int ErrNr )
{
   if( !DiffTmpName.empty() && rtl::sysutils_p3::FileExists( DiffTmpName ) )
   {
      PGXDIF->gdxClose();
      rtl::sysutils_p3::DeleteFileFromDisk( DiffTmpName );
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

void CheckGDXError( const std::shared_ptr<gdx::TGXFileObj> &PGX )
{
   int ErrNr { PGX->gdxGetLastError() };
   if( ErrNr != 0 )
   {
      std::string S;
      PGX->gdxErrorStr( ErrNr, S.data() );
      std::cerr << "GDXDIFF GDX Error: " << S << std::endl;
   }
}

void OpenGDX( const std::string &fn, const std::shared_ptr<gdx::TGXFileObj> &PGX )
{
   if( !rtl::sysutils_p3::FileExists( fn ) )
      FatalError( "Input file not found " + fn, static_cast<int>( ErrorCode::ERR_NOFILE ) );

   // TODO: Remove?
   // if( !PGX->gdxCreateX( S ) )
   //    FatalError( "Cannot load GDX library " + S, static_cast<int>( ErrorCode::ERR_LOADDLL ) );

   int ErrNr;
   PGX->gdxOpenRead( fn.data(), ErrNr );
   if( ErrNr != 0 )
   {
      std::string S;
      PGX->gdxErrorStr( ErrNr, S.data() );
      FatalError2( "Problem reading GDX file + " + fn, S, static_cast<int>( ErrorCode::ERR_READGDX ) );
   }

   int NrElem, HighV;
   PGX->gdxUMUelInfo( NrElem, HighV );
   PGX->gdxUELRegisterMapStart();
   for( int N { 1 }; N <= NrElem; N++ )
   {
      int NN;
      std::string UEL;
      PGX->gdxUMUelGet( N, UEL.data(), NN );
      NN = UELTable->Add( UEL.data(), UEL.length() );
      PGX->gdxUELRegisterMap( NN, UEL.data() );
   }
   PGX->gdxUELRegisterDone();
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

   PGXDIF->gdxUELRegisterStrStart();
   int d;
   for( int N { 1 }; N <= maxUEL; N++ )
      PGXDIF->gdxUELRegisterStr( UELTable->GetString( N ), d );
   PGXDIF->gdxUELRegisterDone();
   CheckGDXError( PGXDIF );

   diffUELsRegistered = true;
}

void CompareSy( const int Sy1, const int Sy2 )
{
   int Dim, VarEquType;
   gdxSyType ST;
   std::string Id;
   bool SymbOpen {};
   TStatusCode Status;
   gdx::TgdxValues DefValues;

   auto CheckSymbOpen = [&]() -> bool {
      registerDiffUELs();
      if( Status == TStatusCode::sc_dim10 ) Status = TStatusCode::sc_dim10_diff;
      if( !SymbOpen && Status != TStatusCode::sc_dim10_diff )
      {
         if( FldOnlyVar == FldOnly::fld_yes && ( ST == dt_var || ST == dt_equ ) )
         {
            std::string ExplTxt = "Differences Field = " + GamsFieldNames[FldOnlyFld];
            PGXDIF->gdxDataWriteStrStart( Id.data(), ExplTxt.data(), Dim + 1, dt_par, 0 );
         }
         if( DiffOnly && ( ST == dt_var || ST == dt_equ ) )
            PGXDIF->gdxDataWriteStrStart( Id.data(), "Differences Only", Dim + 2, dt_par, 0 );
         else
            PGXDIF->gdxDataWriteStrStart( Id.data(), "Differences", Dim + 1, ST, VarEquType );
         SymbOpen = true;
      }
      return SymbOpen;
   };

   auto SymbClose = [&]() {
      if( SymbOpen )
      {
         SymbOpen = false;
         CheckGDXError( PGXDIF );
         PGXDIF->gdxDataWriteDone();
         CheckGDXError( PGXDIF );
      }
   };

   auto WriteDiff = [&]( const std::string &Act, const std::string &FldName, const gdx::TgdxUELIndex &Keys, const gdx::TgdxValues &Vals ) {
      gdxStrIndex_t StrKeys;
      gdxStrIndexPtrs_t StrKeysPtrs;
      GDXSTRINDEXPTRS_INIT( StrKeys, StrKeysPtrs );
      gdx::TgdxValues Vals2;

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
         Vals2[gdx::vallevel] = Vals[FldOnlyFld];
         PGXDIF->gdxDataWriteStr( const_cast<const char **>( StrKeysPtrs ), Vals2.data() );
      }
      else
         PGXDIF->gdxDataWriteStr( const_cast<const char **>( StrKeysPtrs ), Vals.data() );
   };

   auto WriteSetDiff = [&]( const std::string &Act, const gdx::TgdxUELIndex &Keys, const std::string &S ) {
      gdxStrIndex_t StrKeys;
      gdxStrIndexPtrs_t StrKeysPtrs;
      GDXSTRINDEXPTRS_INIT( StrKeys, StrKeysPtrs );
      gdx::TgdxValues Vals;
      int iNode;

      registerDiffUELs();
      for( int D { 1 }; D <= Dim; D++ )
         strcpy( StrKeys[D], UELTable->GetString( Keys[D] ) );
      strcpy( StrKeys[Dim + 1], Act.data() );
      PGXDIF->gdxAddSetText( S.data(), iNode );
      Vals[gdx::vallevel] = iNode;
      PGXDIF->gdxDataWriteStr( const_cast<const char **>( StrKeysPtrs ), Vals.data() );
   };

   auto WriteValues = [&]( const std::shared_ptr<gdx::TGXFileObj> &PGX, const gdx::TgdxValues &Vals ) {
      switch( ST )
      {
         case dt_set:
            assert( false && "Should not be called" );
            break;

         case dt_par:
            std::cout << ValAsString( PGX, Vals[gdx::vallevel] ) << std::endl;
            break;

         default:
            for( int T { 0 }; T < tvarvaltype_size; T++ )
               std::cout << ValAsString( PGX, Vals[T] ) << ' ';
            std::cout << std::endl;
            break;
      }
   };

   auto WriteKeys = [&]( const gdx::TgdxUELIndex &Keys ) {
      registerDiffUELs();
      for( int D { 1 }; D <= Dim; D++ )
      {
         std::cout << ' ' << UELTable->GetString( Keys[D] );
         if( D < Dim ) std::cout << " .";
      }
   };

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
      PGX1->gdxMapValue( V1, iSV1 );
      PGX2->gdxMapValue( V2, iSV2 );

      bool result;
      std::string S1, S2;
      double AbsDiff;
      if( iSV1 == sv_normal )
      {
         if( iSV2 == sv_normal )
         {
            if( PGX1->gdxAcronymName( V1, S1.data() ) != 0 && PGX2->gdxAcronymName( V2, S2.data() ) != 0 )
               result = gdlib::strutilx::StrUEqual( S1, S2 );
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

   auto CheckParDifference = [&]( const gdx::TgdxUELIndex &Keys, const gdx::TgdxValues &V1, const gdx::TgdxValues &V2 ) -> bool {
      bool result { true };
      if( ST == dt_par )
         result = DoublesEqual( V1[gdx::vallevel], V2[gdx::vallevel] );
      else if( FldOnlyVar == FldOnly::fld_yes )
         result = DoublesEqual( V1[FldOnlyFld], V2[FldOnlyFld] );
      else
      {
         for( int T { 0 }; T < tvarvaltype_size; T++ )
         {
            if( ActiveFields.find( gdx::tvarvaltype( T ) ) != ActiveFields.end() && !DoublesEqual( V1[T], V2[T] ) )
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
               if( ActiveFields.find( gdx::tvarvaltype( T ) ) == ActiveFields.end() ) continue;
               if( DoublesEqual( V1[T], V2[T] ) ) continue;

               gdx::TgdxValues Vals;
               Vals[gdx::vallevel] = V1[T];
               WriteDiff( c_dif1, GamsFieldNames[T], Keys, Vals );
               Vals[gdx::vallevel] = V2[T];
               WriteDiff( c_dif2, GamsFieldNames[T], Keys, Vals );
            }
         }
      }
      return result;
   };

   auto CheckSetDifference = [&]( const gdx::TgdxUELIndex &Keys, const int txt1, const int txt2 ) -> bool {
      std::string S1, S2;
      int iNode;
      if( txt1 == 0 ) S1.clear();
      else
         PGX1->gdxGetElemText( txt1, S1.data(), iNode );
      if( txt2 == 0 ) S2.clear();
      else
         PGX2->gdxGetElemText( txt2, S2.data(), iNode );

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

   auto ShowInsert = [&]( const std::string &Act, const gdx::TgdxUELIndex &Keys, gdx::TgdxValues &Vals ) {
      // We check if this insert has values we want to ignore
      bool Eq {};
      switch( ST )
      {
         case dt_par:
            Eq = DoublesEqual( Vals[gdx::vallevel], 0 );
            break;

         case dt_var:
         case dt_equ:
            Eq = true;
            for( int T { 0 }; T < tvarvaltype_size; T++ )
            {
               if( ActiveFields.find( gdx::tvarvaltype( T ) ) != ActiveFields.end() && !DoublesEqual( Vals[T], DefValues[T] ) )
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

      if( ST == dt_set && Vals[gdx::vallevel] != 0 )
      {
         std::string stxt;
         int N;
         if( Act == c_ins1 )
            PGX1->gdxGetElemText( static_cast<int>( round( Vals[gdx::vallevel] ) ), stxt.data(), N );
         else
            PGX2->gdxGetElemText( static_cast<int>( round( Vals[gdx::vallevel] ) ), stxt.data(), N );
         PGXDIF->gdxAddSetText( stxt.data(), N );
         Vals[gdx::vallevel] = N;
      }

      if( !( DiffOnly && ( ST == dt_var || ST == dt_equ ) ) )
         WriteDiff( Act, "", Keys, Vals );
      else
      {
         gdx::TgdxValues Vals2;
         for( int T { 0 }; T < tvarvaltype_size; T++ )
         {
            if( ActiveFields.find( gdx::tvarvaltype( T ) ) == ActiveFields.end() )
               continue;
            Vals2[gdx::vallevel] = Vals[T];
            WriteDiff( Act, GamsFieldNames[T], Keys, Vals2 );
         }
      }
   };

   int Dim2, AFDim, iST, iST2, R1Last, R2Last, C, acount;
   gdxSyType ST2;
   bool Flg1, Flg2, Eq, DomFlg;
   gdx::TgdxUELIndex Keys1, Keys2;
   gdx::TgdxValues Vals1, Vals2;
   std::string stxt;
   gdxStrIndex_t DomSy1;
   gdxStrIndexPtrs_t DomSy1Ptrs;
   GDXSTRINDEXPTRS_INIT( DomSy1, DomSy1Ptrs );
   gdxStrIndex_t DomSy2;
   gdxStrIndexPtrs_t DomSy2Ptrs;
   GDXSTRINDEXPTRS_INIT( DomSy2, DomSy2Ptrs );

   SymbOpen = false;
   PGX1->gdxSymbolInfo( Sy1, Id.data(), Dim, iST );
   ST = static_cast<gdxSyType>( iST );
   if( ST == dt_alias ) ST = dt_set;
   // We do nothing with type in file2
   PGX1->gdxSymbolInfoX( Sy1, acount, VarEquType, stxt.data() );

   PGX2->gdxSymbolInfo( Sy2, Id.data(), Dim2, iST2 );
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
      PGX1->gdxSymbolGetDomainX( Sy1, DomSy1Ptrs );
      PGX2->gdxSymbolGetDomainX( Sy2, DomSy2Ptrs );
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
      Flg1 = PGX1->gdxDataReadRawStart( Sy1, R1Last ) != 0;
      if( Flg1 ) Flg1 = PGX1->gdxDataReadRaw( Keys1.data(), Vals1.data(), AFDim ) != 0;

      Flg2 = PGX2->gdxDataReadRawStart( Sy2, R2Last ) != 0;
      if( Flg2 ) Flg2 = PGX2->gdxDataReadRaw( Keys2.data(), Vals2.data(), AFDim ) != 0;
   }
   else
   {
      Flg1 = PGX1->gdxDataReadMapStart( Sy1, R1Last ) != 0;
      if( Flg1 ) Flg1 = PGX1->gdxDataReadMap( 0, Keys1.data(), Vals1.data(), AFDim ) != 0;

      Flg2 = PGX2->gdxDataReadMapStart( Sy2, R2Last ) != 0;
      if( Flg2 ) Flg2 = PGX2->gdxDataReadMap( 0, Keys2.data(), Vals2.data(), AFDim ) != 0;
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
               Eq = CheckSetDifference( Keys1, static_cast<int>( round( Vals1[gdx::vallevel] ) ), static_cast<int>( round( Vals2[gdx::vallevel] ) ) );
            else
               Eq = true;
         }
         else
            Eq = CheckParDifference( Keys1, Vals1, Vals2 );

         if( !Eq && Status == TStatusCode::sc_same )
            Status = TStatusCode::sc_data;

         if( matrixFile )
         {
            Flg1 = PGX1->gdxDataReadRaw( Keys1.data(), Vals1.data(), AFDim ) != 0;
            Flg2 = PGX2->gdxDataReadRaw( Keys2.data(), Vals2.data(), AFDim ) != 0;
         }
         else
         {
            Flg1 = PGX1->gdxDataReadMap( 0, Keys1.data(), Vals1.data(), AFDim ) != 0;
            Flg2 = PGX2->gdxDataReadMap( 0, Keys2.data(), Vals2.data(), AFDim ) != 0;
         }
      }
      if( C < 0 )
      {
         ShowInsert( c_ins1, Keys1, Vals1 );
         if( matrixFile )
            Flg1 = PGX1->gdxDataReadRaw( Keys1.data(), Vals1.data(), AFDim ) != 0;
         else
            Flg1 = PGX1->gdxDataReadMap( 0, Keys1.data(), Vals1.data(), AFDim ) != 0;
      }
      else
      {
         ShowInsert( c_ins2, Keys2, Vals2 );
         if( matrixFile )
            Flg2 = PGX2->gdxDataReadRaw( Keys2.data(), Vals2.data(), AFDim ) != 0;
         else
            Flg2 = PGX2->gdxDataReadMap( 0, Keys2.data(), Vals2.data(), AFDim ) != 0;
      }
      // Change in status happens inside ShowInsert
   }

   while( Flg1 )
   {
      ShowInsert( c_ins1, Keys1, Vals1 );
      if( matrixFile )
         Flg1 = PGX1->gdxDataReadRaw( Keys1.data(), Vals1.data(), AFDim ) != 0;
      else
         Flg1 = PGX1->gdxDataReadMap( 0, Keys1.data(), Vals1.data(), AFDim ) != 0;
   }

   while( Flg2 )
   {
      ShowInsert( c_ins2, Keys2, Vals2 );
      if( matrixFile )
         Flg2 = PGX2->gdxDataReadRaw( Keys2.data(), Vals2.data(), AFDim ) != 0;
      else
         Flg2 = PGX2->gdxDataReadMap( 0, Keys2.data(), Vals2.data(), AFDim ) != 0;
   }

   SymbClose();

label999:
   if( !( Status == TStatusCode::sc_same || Status == TStatusCode::sc_dim10 ) )
      StatusTable[Id] = Status;
}

bool GetAsDouble( const std::string &S, double &V )
{
   int k;
   utils::val( S, V, k );
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
// void CopyAcronyms( const std::shared_ptr<gdx::TGXFileObj> &PGX ) {}

void CheckFile( std::string &fn )
{
   if( !rtl::sysutils_p3::FileExists( fn ) && gdlib::strutilx::ExtractFileExtEx( fn ).empty() )
      fn = gdlib::strutilx::ChangeFileExtEx( fn, ".gdx" );
}

int main( const int argc, const char *argv[] )
{
   int ErrorCode, ErrNr, Dim, iST, StrNr;
   std::string S, S2, ID, InFile1, InFile2, DiffFileName;
   std::map<std::string, int> IDTable;
   bool UsingIDE, RenameOK;
   gdxStrIndex_t StrKeys;
   gdxStrIndexPtrs_t StrKeysPtrs;
   GDXSTRINDEXPTRS_INIT( StrKeys, StrKeysPtrs );
   gdx::TgdxValues StrVals;

   // TODO: Remove?
   // gdlSetSystemName( 'GDXDIFF' );
   // if( argv[1] == "AUDIT" )
   // {
   //    std::cout << gdlGetAuditLine() << std::endl;
   //    // TODO: Check exit value or use return?
   //    exit( 0 );
   // }

   // So we can check later
   // DiffTmpName.clear();

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

   if( gdlib::strutilx::ExtractFileExtEx( DiffFileName ).empty() )
      DiffFileName = gdlib::strutilx::ChangeFileExtEx( DiffFileName, ".gdx" );

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
      ActiveFields = { gdx::vallevel, gdx::valmarginal, gdx::vallower, gdx::valupper, gdx::valscale };
   else
   {
      if( gdlib::strutilx::StrUEqual( S, "All" ) )
         ActiveFields = { gdx::vallevel, gdx::valmarginal, gdx::vallower, gdx::valupper, gdx::valscale };
      else if( gdlib::strutilx::StrUEqual( S, "L" ) )
      {
         FldOnlyFld = gdx::vallevel;
         FldOnlyVar = FldOnly::fld_maybe;
      }
      else if( gdlib::strutilx::StrUEqual( S, "M" ) )
      {
         FldOnlyFld = gdx::valmarginal;
         FldOnlyVar = FldOnly::fld_maybe;
      }
      else if( gdlib::strutilx::StrUEqual( S, "Up" ) )
      {
         FldOnlyFld = gdx::valupper;
         FldOnlyVar = FldOnly::fld_maybe;
      }
      else if( gdlib::strutilx::StrUEqual( S, "Lo" ) )
      {
         FldOnlyFld = gdx::vallower;
         FldOnlyVar = FldOnly::fld_maybe;
      }
      else if( gdlib::strutilx::StrUEqual( S, "Prior" ) || gdlib::strutilx::StrUEqual( S, "Scale" ) )
      {
         FldOnlyFld = gdx::valscale;
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
      S = gdlib::strutilx::UpperCase( S );
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
                  ID = S.substr( 0, k - 1 );
                  S = S.erase( 0, k );
                  S = utils::trim( S );
               }
               ID = utils::trim( ID );
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

   {
      std::string ErrMsg;
      PGX1 = std::make_shared<gdx::TGXFileObj>( ErrMsg );
      PGX2 = std::make_shared<gdx::TGXFileObj>( ErrMsg );
      PGXDIF = std::make_shared<gdx::TGXFileObj>( ErrMsg );
   }

   // TODO: Remove?
   // if( !PGXDIF->gdxCreateX( S2 ) )
   //    FatalError( "Unable to load GDX library: " + S2, static_cast<int>( ErrorCode::ERR_LOADDLL ) );

   // Temporary file name
   for( int N { 1 }; N <= std::numeric_limits<int>::max(); N++ )
   {
      DiffTmpName = "tmpdifffile" + std::to_string( N ) + ".gdx";
      if( !rtl::sysutils_p3::FileExists( DiffTmpName ) )
         break;
   }

   PGXDIF->gdxOpenWrite( DiffTmpName.data(), "GDXDIFF", ErrNr );
   if( ErrNr != 0 )
   {
      int N { PGXDIF->gdxGetLastError() };
      // Nil is used instead of PGXDIF in Delphi code
      PGXDIF->gdxErrorStr( N, S.data() );
      FatalError2( "Cannot create file: " + DiffTmpName, S, static_cast<int>( ErrorCode::ERR_WRITEGDX ) );
   }

   UELTable = std::make_unique<gdlib::strhash::TXStrHashList<nullptr_t>>();
   UELTable->OneBased = true;
   PGXDIF->gdxStoreDomainSetsSet( false );

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
      while( PGX1->gdxSymbolInfo( N, ID.data(), Dim, iST ) != 0 )
      {
         if( IDsOnly == nullptr || IDsOnly->IndexOf( ID.data() ) >= 0 )
            IDTable[ID] = N;
         N++;
      }
   }

   for( const auto &pair: IDTable )
   {
      int NN;
      if( PGX2->gdxFindSymbol( pair.first.data(), NN ) != 0 )
         CompareSy( pair.second, NN );
      else
         StatusTable[pair.first] = TStatusCode::sc_notf2;
   }

   // Find symbols in file 2 that are not in file 1
   IDTable.clear();

   {
      int N { 1 };
      while( PGX2->gdxSymbolInfo( N, ID.data(), Dim, iST ) != 0 )
      {
         if( IDsOnly == nullptr || IDsOnly->IndexOf( ID.data() ) >= 0 )
            IDTable[ID] = N;
         N++;
      }
   }

   for( const auto &pair: IDTable )
   {
      int NN;
      if( PGX1->gdxFindSymbol( pair.first.data(), NN ) == 0 )
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
         std::cout << gdlib::strutilx::PadRight( pair.first, NN ) << "   "
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
         if( PGXDIF->gdxFindSymbol( ID.data(), NN ) == 0 )
            break;
         N++;
      } while( true );
   }

   PGXDIF->gdxDataWriteStrStart( ID.data(), "", 1, dt_set, 0 );
   strcpy( StrKeys[1], "File1" );
   PGXDIF->gdxAddSetText( InFile1.data(), StrNr );
   StrVals[gdx::vallevel] = StrNr;
   PGXDIF->gdxDataWriteStr( const_cast<const char **>( StrKeysPtrs ), StrVals.data() );
   strcpy( StrKeys[1], "File2" );
   PGXDIF->gdxAddSetText( InFile2.data(), StrNr );
   StrVals[gdx::vallevel] = StrNr;
   PGXDIF->gdxDataWriteStr( const_cast<const char **>( StrKeysPtrs ), StrVals.data() );
   PGXDIF->gdxDataWriteDone();

   // Note that input files are not closed at this point; so if we wrote
   // to an input file, the delete will fail and we keep the original input file alive
   PGX1->gdxClose();
   PGX2->gdxClose();
   PGXDIF->gdxClose();

   if( !rtl::sysutils_p3::FileExists( DiffFileName ) )
      RenameOK = true;
   else
   {
      RenameOK = rtl::sysutils_p3::DeleteFileFromDisk( DiffFileName );
#if defined( _WIN32 )
      if( !RenameOK )
      {
         int ShellCode;
         if( rtl::p3process::P3ExecP( "IDECmds.exe ViewClose \"" + DiffFileName + "\"", ShellCode ) == 0 )
            RenameOK = rtl::sysutils_p3::DeleteFileFromDisk( DiffFileName );
      }
#endif
   }

   if( RenameOK )
   {
      rtl::sysutils_p3::RenameFile( DiffTmpName, DiffFileName );
      RenameOK = rtl::sysutils_p3::FileExists( DiffFileName );
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
