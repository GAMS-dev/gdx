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
#include <fstream>
#include <memory>
#include <filesystem>
#include <map>
#include <limits>
#include <cstring>

#include "gdxdump.h"
#include "../library/common.h"
#include "../library/short_string.h"
#include "../../gdlib/utils.h"
#include "../../gdlib/strutilx.h"

// Global constants
#include "../../../generated/gclgms.h"
// GDX library interface
#include "../../../generated/gdxcc.h"

namespace gdxdump
{

// TODO: Remove these three arrays when they are no longer used
const static std::array<std::string, 5> valsTypTxt { "L", "M", "LO", "UP", "SCALE" };
const static std::array<std::string, 10> varsTypTxt { "unknown ", "binary  ", "integer ", "positive", "negative", "free    ", "sos1    ", "sos2    ", "semicont", "semiint " };
const static std::array<std::string, 7> svTxt = { "Undf", "NA", "+Inf", "-Inf", "Eps", "0", "AcroN" };

static std::ostream &fo = std::cout;
static std::ofstream OutputFile;
static gdxHandle_t PGX = nullptr;
static char Delim, DecimalSep;
static bool ShowHdr, ShowData, CDim, FilterDef, bEpsOut, bNaOut, bPinfOut, bMinfOut, bUndfOut, bZeroOut, bHeader, bFullEVRec, bCSVSetText;
static TOutFormat OutFormat;
static TDblFormat dblFormat;
static int LineCount;
static std::string EpsOut, NaOut, PinfOut, MinfOut, UndfOut, ZeroOut, Header;

char QQ( const std::string &s )
{
   return s.find( '\'' ) == std::string::npos ? '\'' : '\"';
}

std::string QQCSV( const std::string &s )
{
   if( Delim == '\t' )
      return s;
   auto k = s.find( '\"' );
   if( k == std::string::npos )
      return "\"" + s + "\"";
   std::string res = s.substr( 0, k + 1 ) + '\"',// two quotes
           ws = s.substr( k + 1, GMS_SSSIZE - 1 );
   do {
      k = ws.find( '\"' );
      if( k == std::string::npos )
         break;
      res += ws.substr( 0, k + 1 ) + '\"';// two quotes
      ws = ws.substr( k + 1 );
   } while( true );
   return '\"' + res + ws + '\"';
}

// Function is commented out in the Delphi source code
// std::string QUEL( const std::string &S )
// {
//    return {};
// }

void WriteQuotedCommon( const std::string &S, const std::function<bool( char )> &isSpecialPredicate, const int i, bool G )
{
   for( int k { i }; k < static_cast<int>( S.length() ); k++ )
   {
      const char c = S[k];
      if( isSpecialPredicate( c ) || ( c == '.' && k + 1 < static_cast<int>( S.length() ) && S[k + 1] == '.' ) )
      {
         G = false;
         break;
      }
   }
   if( G )
      fo << ' ' << S;
   else
   {
      const auto QChar = QQ( S );
      fo << ' ' << QChar << S << QChar;
   }
}

// Write explanatory text with quotes if needed
void WriteQText( const std::string &S )
{
   WriteQuotedCommon( S, []( const char c ) {
      return utils::in( c, ',', ';', '/', '$', '=', '\'', '\"' );
   } );
}

// Check if UEL associated text needs to be quoted
void WriteQUELText( const std::string &S )
{
   WriteQuotedCommon( S, []( const char c ) {
      return utils::in( c, ' ', ',', ';', '/', '$', '=', '*' );
   } );
}

void WriteUEL( const std::string &S )
{
   const auto QChar = QQ( S );
   fo << QChar << S << QChar;
}

void WriteUELTable( const std::string &name )
{
   int N, NrUel;
   gdxSystemInfo( PGX, &N, &NrUel );
   if( OutFormat != TOutFormat::fmt_csv )
   {
      if( NrUel > 0 )
         fo << "Set " << name << " /" << '\n';
      else
      {
         fo << "$onEmpty" << '\n'
            << "Set " << name << "(*) / /;" << '\n'
            << "$offEmpty" << '\n';
      }
   }
   for( N = 1; N <= NrUel; N++ )
   {
      library::short_string s {};
      int UMap;
      gdxUMUelGet( PGX, N, s.data(), &UMap );
      fo << "  ";
      WriteUEL( s.data() );
      if( OutFormat == TOutFormat::fmt_csv )
         fo << '\n';
      else
         fo << ( N < NrUel ? " ," : " /;" ) << '\n';
   }
}

// TODO: Remove and use specialValueStr from the library instead
std::string specialValueStrClassic( const int i )
{
   if( i < 0 || i >= GMS_SVIDX_MAX )
   {
      assertWithMessage( false, "Unknown type" );
      return "Unknown";
   }
   else
      return svTxt[i];
}

char hexDigit( const uint8_t b )
{
   if( b < 10 )
      return static_cast<char>( '0' + b );
   else
      return static_cast<char>( 'a' + b - 10 );
}

static int64_t signMask;
static int64_t expoMask;
static int64_t mantMask;
static TI64Rec t64 {};
static bool bigEndian;

void initDblUtilValues()
{
   t64.i64 = 1;
   bigEndian = t64.bytes[7] == 1;
   // Do this until P3 accepts large constants like $80000000000000000
   signMask = 0x80000000;
   signMask = signMask << 32;
   expoMask = 0x7ff00000;
   expoMask = expoMask << 32;
   mantMask = ~( signMask | expoMask );
}

std::string DblToStrHex( const double x )
{
   TI64Rec xi { x };
   uint8_t c;
   std::string result = "0x";

   if( bigEndian )
   {
      for( int i {}; i < 8; i++ )
      {
         c = xi.bytes[i];
         result += hexDigit( c / 16 );
         result += hexDigit( c & 0x0F );
      }
   }
   else
   {
      for( int i { 7 }; i >= 0; i-- )
      {
         c = xi.bytes[i];
         result += hexDigit( c / 16 );
         result += hexDigit( c & 0x0F );
      }
   }
   return result;
}

void dblDecomp( const double x, bool &isNeg, uint32_t &expo, int64_t &mant )
{
   TI64Rec xi { x };
   isNeg = ( xi.i64 & signMask ) == signMask;
   expo = ( xi.i64 & expoMask ) >> 52;
   mant = xi.i64 & mantMask;
}

std::string mFormat( int64_t m )
{
   if( m == 0 )
      return "0";

   // TI64Rec xi { .i64 = m };
   int64_t mask, m2;
   int shiftCount;
   uint8_t b;
   std::string result;

   mask = 0x000f0000;
   mask = mask << 32;
   shiftCount = 48;
   while( m != 0 )
   {
      m2 = ( m & mask ) >> shiftCount;
      b = m2;
      result += hexDigit( b );
      m = m & ~mask;
      mask = mask >> 4;
      shiftCount -= 4;
   }
   return result;
}

/**
 * hexponential is the format returned by Java's toHexString(double),
 * c.f. http://java.sun.com/j2se/1.5.0/docs/api/java/lang/Double.html
 * hexBytes spits out the bytes in network (i.e. big-endian) byte order
*/

std::string DblToStrHexponential( const double x )
{
   bool isNeg;
   uint32_t expo;
   int64_t mant;
   std::string result;

   dblDecomp( x, isNeg, expo, mant );
   if( isNeg )
      result += '-';
   if( expo == 0 )
   {
      if( mant == 0 )
         result += "0x0.0p0";
      else
         result += "0x0." + mFormat( mant ) + "p-1022";
   }
   else if( expo < 2047 )
      result += "0x1." + mFormat( mant ) + 'p' + std::to_string( static_cast<int>( expo ) - 1023 );
   else
   {
      if( mant == 0 )
         result += "Infinity";
      else
         result += "NaN";
   }
   return result;
}

void WrVal( const double V )
{
   library::short_string acrname {};
   if( gdxAcronymName( PGX, V, acrname.data() ) != 0 )
      fo << acrname.data();
   else
   {
      int iSV;
      gdxMapValue( PGX, V, &iSV );
      if( iSV != sv_normal )
      {
         if( bEpsOut && iSV == sv_valeps )
            fo << EpsOut;
         else if( bNaOut && iSV == sv_valna )
            fo << NaOut;
         else if( bPinfOut && iSV == sv_valpin )
            fo << PinfOut;
         else if( bMinfOut && iSV == sv_valmin )
            fo << MinfOut;
         else if( bUndfOut && iSV == sv_valund )
            fo << UndfOut;
         else
            fo << specialValueStrClassic( iSV );
      }
      else if( bZeroOut && V == 0 )
         fo << ZeroOut;
      else
         switch( dblFormat )
         {
            case TDblFormat::dbl_normal:
               fo << gdlib::strutilx::DblToStrSep( V, DecimalSep );
               break;
            case TDblFormat::dbl_hexBytes:
               fo << DblToStrHex( V );
               break;
            case TDblFormat::dbl_hexponential:
               fo << DblToStrHexponential( V );
               break;
            default:
               break;
         }
   }
}

static int BadUELs = 0;

std::string GetUELAsString( const int N )
{
   library::short_string res {};
   int IDum;
   if( !gdxUMUelGet( PGX, N, res.data(), &IDum ) )
   {
      BadUELs++;
      return "L__" + std::to_string( N );
   }
   return res.data();
}

std::string GetUel4CSV( const int N )
{
   return QQCSV( GetUELAsString( N ) );
}

bool WriteSymbolAsItem( const int SyNr, const bool DomInfo )
{
   bool result = true;
   library::short_string SyId {};
   int SyDim, SyTyp;
   gdxSymbolInfo( PGX, SyNr, SyId.data(), &SyDim, &SyTyp );
   int SyCnt, SyUser;
   library::short_string SyTxt {};
   gdxSymbolInfoX( PGX, SyNr, &SyCnt, &SyUser, SyTxt.data() );
   fo << '\"' << SyId.data() << "\"." << SyDim << ".\"" << library::gdxDataTypStr( SyTyp ) << '\"';
   if( DomInfo )
   {
      std::string Domain;
      if( SyDim > 0 )
      {
         gdxStrIndex_t DomainArray;
         gdxStrIndexPtrs_t DomainArrayPtrs;
         GDXSTRINDEXPTRS_INIT( DomainArray, DomainArrayPtrs );
         gdxSymbolGetDomainX( PGX, SyNr, DomainArrayPtrs );
         for( int D {}; D < SyDim; D++ )
         {
            if( D > 0 )
               Domain += ", ";
            Domain += DomainArray[D];
            constexpr int MaxNameLen = 63;
            if( static_cast<int>( Domain.length() ) > MaxNameLen )
            {
               Domain = "Domain exceeds " + std::to_string( MaxNameLen ) + " characters";
               result = false;
               break;
            }
         }
      }
      fo << ".\"" << Domain << '\"';
   }
   else if( !SyTxt.empty() )
   {
      // TODO: Works like this, but implementation is different than in Delphi code
      // const char qChar = SyTxt.string().find( '\"' ) == 0 ? '\"' : '\'';
      constexpr char qChar = '\"';
      fo << ' ' << qChar << SyTxt.data() << qChar;
   }
   return result;
}

void WriteSymbolsAsSet( const bool DomInfo )
{
   if( DomInfo )
   {
      fo << "alias (Symbol, Dim, Type, Domain, *);" << '\n'
         << "set    gdxitemsDI(Symbol,Dim,Type,Domain)  Items in the GDX file /" << '\n';
   }
   else
   {
      fo << "alias (Symbol, Dim, Type, *);" << '\n'
         << "set    gdxitems(Symbol,Dim,Type)  Items in the GDX file /" << '\n';
   }
   int NrSy, NrUel;
   gdxSystemInfo( PGX, &NrSy, &NrUel );
   for( int N { 1 }; N <= NrSy; N++ )
   {
      WriteSymbolAsItem( N, DomInfo );
      if( N < NrSy )
         fo << ',';
      fo << '\n';
   }
   fo << "/;" << '\n';
}

void printErrorMessage( const std::string &message, const bool printError )
{
   // TODO: Possible improvement for later, but currently results in problems with the tests
   // std::cerr << ( printError ? "Error: " : "" ) << message << std::endl;
   std::cout << message << std::endl;
}

void assertWithMessage( const bool expression, const std::string &message )
{
   if( !expression )
      printErrorMessage( message );
   assert( expression );
}

// TODO: Remove and use valTypStr from the library instead
std::string valTypStrClassic( const int i )
{
   if( i < 0 || i >= GMS_VAL_MAX )
   {
      assertWithMessage( false, "Unknown type" );
      return "Unknown";
   }
   else
      return valsTypTxt[i];
}

// TODO: Remove and use varTypStr from the library instead
std::string varTypStrClassic( const int i )
{
   if( i < 0 || i >= GMS_VARTYPE_MAX )
   {
      assertWithMessage( false, "Unknown type" );
      return "Unknown";
   }
   else
      return varsTypTxt[i];
}

void WriteSymbol( const int SyNr )
{
   library::short_string SyName {}, S {};
   std::string SubTypeName;
   int ADim, iATyp, ACount, AUser, IDum, NRec, FDim;
   gdxSyType ATyp;
   bool IsScalar, FrstWrite;
   gdxUelIndex_t Keys {};
   gdxValues_t Vals {};
   std::array<double, GMS_VAL_MAX> DefaultValues {};

   auto WriteItem = [&IsScalar, &ATyp, &Vals, &DefaultValues, &FrstWrite, &ADim, &SyName, &Keys, &S, &IDum, &ACount]( const uint8_t &ValNr ) {
      if( !IsScalar && ATyp != dt_set && ( FilterDef && Vals[ValNr] == DefaultValues[ValNr] ) && !( ATyp == dt_equ && ( ValNr == GMS_VAL_LOWER || ValNr == GMS_VAL_UPPER ) ) )
         return;
      if( FrstWrite )
         FrstWrite = false;
      else if( OutFormat == TOutFormat::fmt_gamsbas )
         fo << " ;" << '\n';
      else
      {
         fo << ", ";
         if( !( ( ATyp == dt_var || ATyp == dt_equ ) && ADim == 0 ) )
            fo << '\n';
      }
      if( OutFormat == TOutFormat::fmt_gamsbas )
      {
         if( LineCount == 6 )
            fo << "$offListing" << '\n';
         fo << ' ' << SyName.data() << '.' << valTypStrClassic( ValNr ) << ' ';
      }
      if( ADim > 0 )
      {
         if( OutFormat == TOutFormat::fmt_gamsbas )
            fo << '(';
         for( int D {}; D < ADim; D++ )
         {
            WriteUEL( GetUELAsString( Keys[D] ) );
            if( D < ADim - 1 )
               fo << Delim;
         }
         if( OutFormat == TOutFormat::fmt_gamsbas )
            fo << ')';
      }
      switch( ATyp )
      {
         case dt_set:
            if( Vals[GMS_VAL_LEVEL] != 0 )
            {
               gdxGetElemText( PGX, static_cast<int>( Vals[GMS_VAL_LEVEL] ), S.data(), &IDum );
               WriteQUELText( S.data() );
            }
            break;
         case dt_par:
            fo << ' ';
            WrVal( Vals[ValNr] );
            break;
         case dt_equ:
         case dt_var:
            if( OutFormat == TOutFormat::fmt_gamsbas )
               fo << " = ";
            else
            {
               if( ADim > 0 )
                  fo << Delim;
               fo << valTypStrClassic( ValNr ) << ' ';
            }
            WrVal( Vals[ValNr] );
            break;
         default:
            fo << "Oops";
            break;
      }
      ACount--;
      LineCount++;
   };

   auto WriteComments = [&SyNr]() {
      library::short_string S {};
      for( int N { 1 }; N <= std::numeric_limits<int>::max(); N++ )
      {
         if( gdxSymbolGetComment( PGX, SyNr, N, S.data() ) == 0 )
            break;
         fo << "* " << S.data() << '\n';
      }
   };

   gdxUelIndex_t DomainNrs {};
   gdxStrIndex_t DomainIDs;
   gdxStrIndexPtrs_t DomainIDsPtrs;
   GDXSTRINDEXPTRS_INIT( DomainIDs, DomainIDsPtrs );
   library::short_string A2Name {}, setName {};
   int A2Dim, iA2Typ, setDim, setType;

   gdxSymbolInfo( PGX, SyNr, SyName.data(), &ADim, &iATyp );
   ATyp = static_cast<gdxSyType>( iATyp );
   if( ( ATyp == dt_set || ATyp == dt_par ) && OutFormat == TOutFormat::fmt_gamsbas )
      return;
   if( ShowHdr )
      fo << '\n';
   // if( false )
   //    fo << "$onText" << '\n';
   BadUELs = 0;
   IsScalar = ADim == 0 && ATyp == dt_par;
   gdxSymbolInfoX( PGX, SyNr, &ACount, &AUser, S.data() );
   // fo << "The sub-type = " << AUser << '\n';
   switch( ATyp )
   {
      case dt_set:
         DefaultValues[GMS_VAL_LEVEL] = 0;
         if( AUser == GMS_SETTYPE_SINGLETON )
            SubTypeName = "Singleton";
         break;
      case dt_var:
         if( AUser < 0 || AUser > GMS_VARTYPE_SEMIINT )
            AUser = GMS_VARTYPE_FREE;
         std::copy( std::begin( gmsDefRecVar[AUser] ), std::end( gmsDefRecVar[AUser] ), std::begin( DefaultValues ) );
         if( AUser != GMS_VARTYPE_UNKNOWN )
            SubTypeName = varTypStrClassic( AUser );
         break;
      case dt_equ:
         if( AUser < GMS_EQUTYPE_E || AUser > GMS_EQUTYPE_B )
            AUser = GMS_EQUTYPE_E;
         std::copy( std::begin( gmsDefRecEqu[AUser] ), std::end( gmsDefRecEqu[AUser] ), std::begin( DefaultValues ) );
         if( AUser == GMS_EQUTYPE_B )
            SubTypeName = "Logic";
         break;
      default:
         DefaultValues[GMS_VAL_LEVEL] = 0;
         break;
   }
   if( ShowHdr )
   {
      if( IsScalar )
         fo << "Scalar";
      else
      {
         if( !SubTypeName.empty() )
            fo << SubTypeName << ' ';
         fo << library::gdxDataTypStrL( ATyp );
      }
      if( ATyp != dt_alias )
         fo << ' ' << SyName.data();
      else
      {
         fo << " (" << SyName.data();
         if( AUser == 0 )
            A2Name = "*";
         else
            gdxSymbolInfo( PGX, AUser, A2Name.data(), &A2Dim, &iA2Typ );
         fo << ", " << A2Name.data() << ");" << '\n';
      }
      if( ATyp != dt_alias )
      {
         if( ADim >= 1 )
         {
            gdxSymbolGetDomain( PGX, SyNr, DomainNrs );
            for( int D {}; D < ADim; D++ )
            {
               if( DomainNrs[D] <= 0 )
                  strcpy( DomainIDsPtrs[D], "*" );
               else
               {
                  gdxSymbolInfo( PGX, DomainNrs[D], setName.data(), &setDim, &setType );
                  strcpy( DomainIDsPtrs[D], setName.data() );
               }
            }
            fo << '(';
            for( int D {}; D < ADim; D++ )
            {
               fo << DomainIDs[D];
               if( D < ADim - 1 )
                  fo << ',';
               else
                  fo << ')';
            }
         }
         if( !S.empty() )
            WriteQText( S.data() );
      }
   }
   if( ACount == 0 && ShowData )
   {
      if( IsScalar )
      {
         fo << " / 0.0 /;" << '\n';
         WriteComments();
      }
      else if( ShowHdr && ATyp != dt_alias )
      {
         fo << " / /;" << '\n';
         WriteComments();
      }
   }
   else
   {
      if( ShowHdr && ShowData )
      {
         fo << " /";
         if( ADim > 0 )
            fo << '\n';
      }
      WriteComments();
      if( !ShowData )
      {
         if( ATyp != dt_alias )
         {
            if( ACount == 0 )
            {
               fo << " ; !!empty" << '\n'
                  << "$loadDC " << SyName.data() << " !!empty" << '\n';
            }
            else
            {
               fo << " ;" << '\n'
                  << "$loadDC " << SyName.data() << '\n';
            }
         }
      }
      else
      {
         gdxDataReadRawStart( PGX, SyNr, &NRec );
         FrstWrite = true;
         while( gdxDataReadRaw( PGX, Keys, Vals, &FDim ) != 0 )
         {
            switch( OutFormat )
            {
               case TOutFormat::fmt_gamsbas:
                  if( ATyp == dt_equ )
                     WriteItem( GMS_VAL_MARGINAL );
                  else
                  {
                     WriteItem( GMS_VAL_LEVEL );
                     WriteItem( GMS_VAL_MARGINAL );
                  }
                  break;
               case TOutFormat::fmt_csv:
                  assertWithMessage( false, "No CSV processing" );
                  break;
               case TOutFormat::fmt_normal:
                  WriteItem( GMS_VAL_LEVEL );
                  if( ATyp == dt_var || ATyp == dt_equ )
                  {
                     WriteItem( GMS_VAL_MARGINAL );
                     // if( true )
                     // {
                     WriteItem( GMS_VAL_LOWER );
                     WriteItem( GMS_VAL_UPPER );
                     // }
                     WriteItem( GMS_VAL_SCALE );
                  }
                  break;
               default:
                  break;
            }
         }
         gdxDataReadDone( PGX );
         if( OutFormat == TOutFormat::fmt_gamsbas )
            fo << " ;" << '\n';
         else if( ShowHdr )
            fo << " /;" << '\n';
      }
   }
   // if( false )
   //    fo << "$offText" << '\n';
   if( BadUELs > 0 )
      fo << "**** " << BadUELs << " reference(s) to unique elements without a string representation";
}

int64_t delphiRound( const double x )
{
   if( x >= 0 )
      return static_cast<int64_t>( x + 0.5 );
   else
      return static_cast<int64_t>( x - 0.5 );
}

void WriteSymbolCSV( const int SyNr )
{
   int ADim;
   gdxStrIndex_t DomS;
   gdxStrIndexPtrs_t DomSPtrs;
   GDXSTRINDEXPTRS_INIT( DomS, DomSPtrs );

   auto GetDomainNames = [&ADim, &DomSPtrs, &SyNr]() {
      gdxStrIndex_t gdxDomS;
      gdxStrIndexPtrs_t gdxDomSPtrs;
      GDXSTRINDEXPTRS_INIT( gdxDomS, gdxDomSPtrs );
      library::short_string s {};
      bool Done;
      int Nr;

      gdxSymbolGetDomainX( PGX, SyNr, gdxDomSPtrs );
      Nr = 1;
      for( int D {}; D < ADim; D++ )
      {
         s = gdxDomSPtrs[D];
         if( s == "*" )
         {
            s = "Dim" + std::to_string( D + 1 );
            strcpy( gdxDomSPtrs[D], s.data() );
         }
         while( true )
         {
            Done = true;
            for( int DD {}; DD < D - 1; DD++ )
            {
               if( s == DomSPtrs[DD] )
               {
                  Done = false;
                  break;
               }
            }
            if( Done )
               break;
            s = std::string { gdxDomS[D] } + '_' + std::to_string( Nr );
            Nr++;
         }
         strcpy( DomSPtrs[D], s.data() );
      }
   };

   library::short_string SyName {}, S {};
   int iATyp, NRec, FDim, IDum, Col, ColCnt, NrSymb, NrUEL, HighIndex, Indx;
   gdxSyType ATyp;
   std::unique_ptr<bool[]> CSVCols;
   std::unique_ptr<int[]> CSVUels;
   gdxUelIndex_t Keys {};
   gdxValues_t Vals {};

   BadUELs = 0;
   gdxSystemInfo( PGX, &NrSymb, &NrUEL );
   gdxSymbolInfo( PGX, SyNr, SyName.data(), &ADim, &iATyp );
   ATyp = static_cast<gdxSyType>( iATyp );
   GetDomainNames();
   if( ADim < 2 )
      CDim = false;
   if( !CDim )
   {
      if( bHeader )
      {
         if( !Header.empty() )
            fo << Header << '\n';
      }
      else
      {
         if( ShowHdr )
         {
            for( int D {}; D < ADim; D++ )
            {
               fo << QQCSV( DomS[D] );
               if( D < ADim - 1 )
                  fo << Delim;
            }
            if( !( ATyp == dt_set || ATyp == dt_alias ) )
            {
               if( ADim == 0 )
                  fo << QQCSV( "Val" );
               else
                  fo << Delim << QQCSV( "Val" );
            }
            else if( bCSVSetText )
               fo << Delim << QQCSV( "Text" );
            if( ( ATyp == dt_var || ATyp == dt_equ ) && bFullEVRec )
            {
               fo << Delim << QQCSV( "Marginal" )
                  << Delim << QQCSV( "Lower" )
                  << Delim << QQCSV( "Upper" )
                  << Delim << QQCSV( "Scale" );
            }
            fo << '\n';
         }
      }
      gdxDataReadRawStart( PGX, SyNr, &NRec );
      while( gdxDataReadRaw( PGX, Keys, Vals, &FDim ) != 0 )
      {
         if( ADim == 0 )
         {
            WrVal( Vals[GMS_VAL_LEVEL] );
            if( ( ATyp == dt_var || ATyp == dt_equ ) && bFullEVRec )
            {
               fo << Delim;
               WrVal( Vals[GMS_VAL_MARGINAL] );
               fo << Delim;
               WrVal( Vals[GMS_VAL_LOWER] );
               fo << Delim;
               WrVal( Vals[GMS_VAL_UPPER] );
               fo << Delim;
               WrVal( Vals[GMS_VAL_SCALE] );
            }
         }
         else
            for( int D {}; D < ADim; D++ )
            {
               fo << GetUel4CSV( Keys[D] );
               if( D < ADim - 1 )
                  fo << Delim;
               else if( !( ATyp == dt_set || ATyp == dt_alias ) )
               {
                  fo << Delim;
                  WrVal( Vals[GMS_VAL_LEVEL] );
                  if( ( ATyp == dt_var || ATyp == dt_equ ) && bFullEVRec )
                  {
                     fo << Delim;
                     WrVal( Vals[GMS_VAL_MARGINAL] );
                     fo << Delim;
                     WrVal( Vals[GMS_VAL_LOWER] );
                     fo << Delim;
                     WrVal( Vals[GMS_VAL_UPPER] );
                     fo << Delim;
                     WrVal( Vals[GMS_VAL_SCALE] );
                  }
               }
               else if( bCSVSetText )
               {
                  if( Vals[GMS_VAL_LEVEL] != 0 )
                  {
                     gdxGetElemText( PGX, static_cast<int>( delphiRound( Vals[GMS_VAL_LEVEL] ) ), S.data(), &IDum );
                     fo << Delim << QQCSV( S.data() );
                  }
                  else
                     fo << Delim;
               }
            }
         fo << '\n';
      }
   }
   else
   {
      CSVCols = std::make_unique<bool[]>( NrUEL );
      std::fill_n( CSVCols.get(), NrUEL, false );
      HighIndex = 0;
      ColCnt = 0;
      gdxDataReadRawStart( PGX, SyNr, &NRec );
      while( gdxDataReadRaw( PGX, Keys, Vals, &FDim ) != 0 )
      {
         Indx = Keys[ADim - 1];
         if( !CSVCols[Indx - 1] )
         {
            CSVCols[Indx - 1] = true;
            ColCnt++;
            if( Indx > HighIndex )
               HighIndex = Indx;
         }
      }
      gdxDataReadDone( PGX );

      CSVUels = std::make_unique<int[]>( ColCnt );
      std::fill_n( CSVUels.get(), ColCnt, 0 );
      Col = 0;
      for( Indx = 0; Indx < HighIndex; Indx++ )
      {
         if( CSVCols[Indx] )
         {
            CSVUels[Col] = Indx + 1;
            Col++;
         }
      }
      assertWithMessage( Col == ColCnt, "Col != ColCnt" );
      if( bHeader )
      {
         if( !Header.empty() )
            fo << Header << '\n';
      }
      else if( ShowHdr )
      {
         for( int D {}; D < ADim - 1; D++ )
            fo << QQCSV( DomS[D] ) << Delim;
         for( Col = 0; Col < ColCnt; Col++ )
         {
            fo << GetUel4CSV( CSVUels[Col] );
            if( Col < ColCnt - 1 )
               fo << Delim;
         }
         fo << '\n';
      }
      gdxDataReadRawStart( PGX, SyNr, &NRec );
      bool EoFData = gdxDataReadRaw( PGX, Keys, Vals, &FDim ) == 0;
      while( !EoFData )
      {
         for( int D {}; D < ADim - 1; D++ )
         {
            fo << GetUel4CSV( Keys[D] );
            if( D < ADim - 2 )
               fo << Delim;
         }
         Col = -1;
         do {
            Indx = Keys[ADim - 1];
            while( Col < ColCnt )
            {
               Col++;
               if( CSVUels[Col] >= Indx )
                  break;
               fo << Delim;
            }
            if( CSVUels[Col] == Indx )
            {
               fo << Delim;
               if( ATyp == dt_set || ATyp == dt_alias )
                  fo << 'Y';
               else
                  WrVal( Vals[GMS_VAL_LEVEL] );
               EoFData = gdxDataReadRaw( PGX, Keys, Vals, &FDim ) == 0;
               if( FDim < ADim || EoFData )
               {
                  while( Col < ColCnt - 1 )
                  {
                     fo << Delim;
                     Col++;
                  }
                  break;
               }
            }
         } while( true );
         fo << '\n';
      }
      gdxDataReadDone( PGX );
   }
   if( BadUELs > 0 )
      fo << "**** " << BadUELs << " reference(s) to unique elements without a string representation" << '\n';
}

int getIntegerWidth( const int number )
{
   return static_cast<int>( std::string { std::to_string( number ) }.length() );
}

void WriteSymbolInfo()
{
   int ADim, iATyp, NrSy, NrUel, w1, w2, w3, ACount, AUserInfo;
   library::short_string AName {}, AExplText {};
   std::map<library::short_string, int> SL;

   gdxSystemInfo( PGX, &NrSy, &NrUel );
   w1 = getIntegerWidth( NrSy );
   w2 = static_cast<int>( std::string { "Symbol" }.length() );
   w3 = static_cast<int>( std::string { "Records" }.length() );
   for( int N { 1 }; N <= NrSy; N++ )
   {
      gdxSymbolInfo( PGX, N, AName.data(), &ADim, &iATyp );
      gdxSymbolInfoX( PGX, N, &ACount, &AUserInfo, AExplText.data() );
      if( static_cast<int>( AName.length() ) > w2 )
         w2 = AName.length();
      if( getIntegerWidth( ACount ) > w3 )
         w3 = getIntegerWidth( ACount );
      SL.insert( { AName, N } );
   }
   fo << gdlib::strutilx::PadLeft( " ", w1 ) << ' '
      << gdlib::strutilx::PadRight( "Symbol", w2 )
      << " Dim" << " Type "
      << gdlib::strutilx::PadRight( "Records", w3 )
      << "  " << "Explanatory text" << '\n';

   int N {};
   for( const auto &pair: SL )
   {
      gdxSymbolInfo( PGX, pair.second, AName.data(), &ADim, &iATyp );
      gdxSymbolInfoX( PGX, pair.second, &ACount, &AUserInfo, AExplText.data() );
      fo << gdlib::strutilx::PadLeft( std::to_string( N + 1 ), w1 ) << ' ' << gdlib::strutilx::PadRight( AName.data(), w2 ) << ' '
         << gdlib::strutilx::PadLeft( std::to_string( ADim ), 3 ) << ' ' << gdlib::strutilx::PadLeft( library::gdxDataTypStr( iATyp ), 4 ) << ' '
         << gdlib::strutilx::PadLeft( std::to_string( ACount ), w3 ) << "  " << AExplText.data() << '\n';
      N++;
   }
}

void WriteDomainInfo()
{
   const std::array<std::string, 4> StrDInfo { "N/A", "None", "Relaxed", "Regular" };

   library::short_string AName {};
   int ADim, iATyp, NrSy, NrUel, w1, dinfo;
   std::map<library::short_string, int> SL;
   gdxStrIndex_t DomainIDs;
   gdxStrIndexPtrs_t DomainIDsPtrs;
   GDXSTRINDEXPTRS_INIT( DomainIDs, DomainIDsPtrs );

   gdxSystemInfo( PGX, &NrSy, &NrUel );
   w1 = getIntegerWidth( NrSy );
   if( w1 < 4 )
      w1 = 4;
   fo << gdlib::strutilx::PadLeft( "SyNr", w1 )
      << "  Type" << "  DomInf " << "Symbol" << '\n';
   for( int N { 1 }; N <= NrSy; N++ )
   {
      gdxSymbolInfo( PGX, N, AName.data(), &ADim, &iATyp );
      SL.insert( { AName, N } );
   }
   for( const auto &pair: SL )
   {
      gdxSymbolInfo( PGX, pair.second, AName.data(), &ADim, &iATyp );
      fo << gdlib::strutilx::PadLeft( std::to_string( pair.second ), w1 ) << ' ' << gdlib::strutilx::PadLeft( library::gdxDataTypStr( iATyp ), 5 );
      dinfo = gdxSymbolGetDomainX( PGX, pair.second, DomainIDsPtrs );
      fo << ' ' << gdlib::strutilx::PadLeft( StrDInfo[dinfo], 7 ) << ' ' << AName.data();
      if( ADim > 0 )
      {
         fo << '(';
         for( int D {}; D < ADim; D++ )
         {
            if( D > 0 )
               fo << ", ";
            fo << DomainIDs[D];
         }
         fo << ')';
      }
      fo << '\n';
   }
}

void WriteSetText()
{
   int nText, lo, hi, mid, idummy, rc, textIdx, mxTextIdx, nSyms, symDim, symTyp, nRecs, fDim;
   library::short_string s {};
   gdxUelIndex_t keys {};
   gdxValues_t vals {};

   lo = 0;
   rc = gdxGetElemText( PGX, lo, s.data(), &idummy );
   assertWithMessage( 1 == rc, "Did not find text in position 0" );
   hi = std::numeric_limits<int>::max();
   rc = gdxGetElemText( PGX, hi, s.data(), &idummy );
   assertWithMessage( 0 == rc, "Found text in position high(integer)" );
   while( lo + 1 < hi )
   {
      mid = lo + ( hi - lo ) / 2;
      rc = gdxGetElemText( PGX, mid, s.data(), &idummy );
      if( 1 == rc )
         lo = mid;
      else
         hi = mid;
   }
   assertWithMessage( lo + 1 == hi, "Impossible end to binary search in WriteSetText" );
   nText = lo + 1;

   mxTextIdx = 0;
   gdxSystemInfo( PGX, &nSyms, &idummy );
   for( int iSym {}; iSym < nSyms; iSym++ )
   {
      gdxSymbolInfo( PGX, iSym, s.data(), &symDim, &symTyp );
      if( static_cast<gdxSyType>( symTyp ) != dt_set )
         continue;
      // fo << "Found set " << s << '\n';
      gdxDataReadRawStart( PGX, iSym, &nRecs );
      while( gdxDataReadRaw( PGX, keys, vals, &fDim ) != 0 )
      {
         textIdx = static_cast<int>( delphiRound( vals[GMS_VAL_LEVEL] ) );
         if( mxTextIdx < textIdx )
            mxTextIdx = textIdx;
      }
      gdxDataReadDone( PGX );
   }
   // fo << '\n';
   fo << "Count of set text strings in GDX: " << nText << '\n'
      << "max 0-based textIdx found in GDX: " << mxTextIdx << '\n'
      << "   idx   text" << '\n';
   for( textIdx = 0; textIdx < nText; textIdx++ )
   {
      if( textIdx == 0 )
         s.clear();
      else
         gdxGetElemText( PGX, textIdx, s.data(), &idummy );
      fo << gdlib::strutilx::PadLeft( std::to_string( textIdx ), 6 ) << "   \"" << s.data() << '\"' << '\n';
   }
}

void Usage()
{
   std::cout << "gdxdump: Write GDX file in ASCII" << '\n'
             // TODO: Replace blank line with function output
             // << gdlGetAuditLine() << '\n'
             << '\n'
             << "Usage:" << '\n'
             << "gdxdump <filename> <options>" << '\n'
             << "<options>" << '\n'
             << "   -V or -Version        Write version info of input file only" << '\n'
             << "   Output=<filename>     Write output to file" << '\n'
             << "   Symb=<identifier>     Select a single identifier" << '\n'
             << "   UelTable=<identifier> Include all unique elements" << '\n'
             << "   Delim=[period, comma, tab, blank, semicolon]" << '\n'
             << "                         Specify a dimension delimiter" << '\n'
             << "   DecimalSep=[period, comma]" << '\n'
             << "                         Specify a decimal separator" << '\n'
             << "   NoHeader              Suppress writing of the headers" << '\n'
             << "   NoData                Write headers only; no data" << '\n'
             << "   CSVAllFields          When writing CSV write all variable/equation fields" << '\n'
             << "   CSVSetText            When writing CSV write set element text" << '\n'
             << "   Symbols               Get a list of all symbols" << '\n'
             << "   DomainInfo            Get a list of all symbols showing domain information" << '\n'
             << "   SymbolsAsSet          Get a list of all symbols as data for a set" << '\n'
             << "   SymbolsAsSetDI        Get a list of all symbols as data for a set includes domain information" << '\n'
             << "   SetText               Show the list of set text (aka associated text)" << '\n'
             << "   Format=[normal, gamsbas, csv]" << '\n'
             << "   dFormat=[normal, hexponential, hexBytes]" << '\n'
             << "   CDim=[Y, N]           Use last dimension as column headers" << '\n'
             << "                         (for CSV format only; default=N)" << '\n'
             << "   FilterDef=[Y, N]      Filter default values; default=Y" << '\n'
             << "   EpsOut=<string>       String to be used when writing the value for EPS;               default=EPS" << '\n'
             << "   NaOut=<string>        String to be used when writing the value for Not Available;     default=NA" << '\n'
             << "   PinfOut=<string>      String to be used when writing the value for Positive Infinity; default=+Inf" << '\n'
             << "   MinfOut=<string>      String to be used when writing the value for Negative Infinity; default=-Inf" << '\n'
             << "   UndfOut=<string>      String to be used when writing the value for Undefined;         default=Undf" << '\n'
             << "   ZeroOut=<string>      String to be used when writing the value for Zero;              default=0" << '\n'
             << "   Header=<string>       New header for CSV output format" << std::endl;
}

static int ParamCount, ParamNr;
static const char **ParamStr;
static std::string ParamHold;

std::string NextParam()
{
   std::string result;

   if( !ParamHold.empty() )
   {
      result = ParamHold;
      ParamHold.clear();
   }
   else if( ParamNr <= ParamCount )
   {
      result = ParamStr[ParamNr];
      ParamNr++;
      auto k = result.find( '=' );
      if( k != std::string::npos )
      {
         ParamHold = result.substr( k + 1 );
         result.erase( k + 1 );
      }
      else if( ParamNr <= ParamCount )
      {
         std::string nextParam = ParamStr[ParamNr];
         if( nextParam.length() > 1 && nextParam.substr( 1, 1 ) == "=" )
         {
            result += '=';
            ParamHold = nextParam.substr( 2 );
            ParamNr++;
         }
      }
   }
   if( result == "\'\'" )
      return {};
   return result;
}

void WriteAcronyms()
{
   int Cnt, Indx;
   library::short_string sName {}, sText {};

   Cnt = gdxAcronymCount( PGX );
   if( Cnt <= 0 )
      return;

   fo << '\n';
   for( int N {}; N < Cnt; N++ )
   {
      gdxAcronymGetInfo( PGX, N, sName.data(), sText.data(), &Indx );
      fo << "Acronym " << sName.data();
      if( !sText.empty() )
         WriteQText( sText.data() );
      fo << ';' << '\n';
   }
}

int main( const int argc, const char *argv[] )
{
   library::short_string s {}, Symb {};
   std::string InputFile, DLLStr, UELSetName, OutputName;
   int ErrNr, ExitCode;
   bool ListAllSymbols, ListSymbolsAsSet, ListSymbolsAsSetDI, UsingIDE, VersionOnly, DomainInfo, showSetText;

   // for( int N {}; N < argc; N++ )
   //    std::cout << "Parameter " << N << ": |" << argv[N] << '|' << std::endl;

   ParamCount = argc - 1;
   ParamStr = argv;

   // TODO: Remove?
   // gdlSetSystemName( 'GDXDUMP' );
   // if( ParamStr[1] == "AUDIT" )
   // {
   //    std::cout << gdlGetAuditLine() << std::endl;
   //    return 0;
   // }

   Delim = '\0';
   DecimalSep = '\0';
   ShowHdr = true;
   ShowData = true;
   ListAllSymbols = false;
   ListSymbolsAsSet = false;
   ListSymbolsAsSetDI = false;
   DomainInfo = false;
   showSetText = false;
   OutFormat = TOutFormat::fmt_none;
   CDim = false;
   dblFormat = TDblFormat::dbl_none;
   UsingIDE = false;
   ExitCode = 0;
   VersionOnly = false;
   FilterDef = true;
   bEpsOut = false;
   bNaOut = false;
   bPinfOut = false;
   bMinfOut = false;
   bUndfOut = false;
   bZeroOut = false;
   bHeader = false;
   bFullEVRec = false;
   bCSVSetText = false;

   if( ParamCount == 0 )
   {
      // show usage
      ExitCode = 1;
   }
   else
   {
      InputFile = ParamStr[1];
      ParamNr = 2;
      ParamHold.clear();

      while( ParamNr <= ParamCount )
      {
         s = NextParam();
         s.to_upper_case();
         if( s == "SYMB" || s == "SYMB=" )
         {
            if( !Symb.empty() )
            {
               printErrorMessage( "Only one single symbol can be specified" );
               ExitCode = 1;
               break;
            }
            Symb = NextParam();
            if( Symb.empty() )
            {
               printErrorMessage( "Symbol missing" );
               ExitCode = 1;
               break;
            }
            continue;
         }
         if( s == "UELTABLE" || s == "UELTABLE=" )
         {
            if( !UELSetName.empty() )
            {
               printErrorMessage( "Only one name for the UEL table can be specified" );
               ExitCode = 1;
               break;
            }
            UELSetName = NextParam();
            if( UELSetName.empty() )
            {
               printErrorMessage( "UELSetName missing" );
               ExitCode = 1;
               break;
            }
            continue;
         }
         if( s == "DELIM" || s == "DELIM=" )
         {
            if( Delim != '\0' )
            {
               printErrorMessage( "Only one delimiter can be specified" );
               ExitCode = 1;
               break;
            }
            s = NextParam();
            s.to_upper_case();
            if( s == "TAB" )
               Delim = '\t';
            else if( s == "COMMA" )
               Delim = ',';
            else if( s == "PERIOD" )
               Delim = '.';
            else if( s == "BLANK" )
               Delim = ' ';
            else if( s == "SEMICOLON" )
               Delim = ';';
            else
            {
               printErrorMessage( "Unrecognized delimiter" );
               ExitCode = 1;
               break;
            }
            continue;
         }
         if( s == "DECIMALSEP" || s == "DECIMALSEP=" )
         {
            if( DecimalSep != '\0' )
            {
               printErrorMessage( "Only one Decimal separator character can be specified" );
               ExitCode = 1;
               break;
            }
            s = NextParam();
            s.to_upper_case();
            if( s == "PERIOD" )
               DecimalSep = '.';
            else if( s == "COMMA" )
               DecimalSep = ',';
            else
            {
               printErrorMessage( "Unrecognized Decimal separator character" );
               ExitCode = 1;
               break;
            }
            continue;
         }
         if( s == "SYMBOLS" )
         {
            ListAllSymbols = true;
            continue;
         }
         if( s == "DOMAININFO" )
         {
            DomainInfo = true;
            continue;
         }
         if( s == "SETTEXT" )
         {
            showSetText = true;
            continue;
         }
         if( s == "SYMBOLSASSET" )
         {
            ListSymbolsAsSet = true;
            continue;
         }
         if( s == "SYMBOLSASSETDI" )
         {
            ListSymbolsAsSetDI = true;
            continue;
         }
         if( s == "NOHEADER" )
         {
            ShowHdr = false;
            continue;
         }
         if( s == "NODATA" )
         {
            ShowData = false;
            continue;
         }
         if( s == "CSVALLFIELDS" )
         {
            bFullEVRec = true;
            continue;
         }
         if( s == "CSVSETTEXT" )
         {
            bCSVSetText = true;
            continue;
         }
         if( s == "FORMAT" || s == "FORMAT=" )
         {
            if( OutFormat != TOutFormat::fmt_none )
            {
               printErrorMessage( "Only one format can be specified" );
               ExitCode = 1;
               break;
            }
            s = NextParam();
            s.to_upper_case();
            if( s == "NORMAL" )
               OutFormat = TOutFormat::fmt_normal;
            else if( s == "GAMSBAS" )
               OutFormat = TOutFormat::fmt_gamsbas;
            else if( s == "CSV" )
               OutFormat = TOutFormat::fmt_csv;
            else
            {
               printErrorMessage( "Unrecognized format" );
               ExitCode = 1;
               break;
            }
            continue;
         }
         if( s == "DFORMAT" || s == "DFORMAT=" )
         {
            if( dblFormat != TDblFormat::dbl_none )
            {
               printErrorMessage( "Only one dformat can be specified" );
               ExitCode = 1;
               break;
            }
            s = NextParam();
            s.to_upper_case();
            if( s == "NORMAL" )
               dblFormat = TDblFormat::dbl_none;
            else if( s == "HEXBYTES" )
               dblFormat = TDblFormat::dbl_hexBytes;
            else if( s == "HEXPONENTIAL" )
               dblFormat = TDblFormat::dbl_hexponential;
            else
            {
               printErrorMessage( "Unrecognized dformat" );
               ExitCode = 1;
               break;
            }
            continue;
         }
         if( s == "OUTPUT" || s == "OUTPUT=" )
         {
            if( !OutputName.empty() )
            {
               printErrorMessage( "Only one output file can be specified" );
               ExitCode = 1;
               break;
            }
            OutputName = NextParam();
            if( OutputName.empty() )
            {
               printErrorMessage( "Output file missing" );
               ExitCode = 1;
               break;
            }
            continue;
         }
         if( s == "IDE" || s == "IDE=" )
         {
            s = NextParam();
            if( s.empty() )
            {
               printErrorMessage( "Value missing for IDE parameter" );
               ExitCode = 1;
               break;
            }
            UsingIDE = s.front() == 'Y' || s.front() == 'y' || s.front() == '1';
            continue;
         }
         if( s == "FILTERDEF" || s == "FILTERDEF=" )
         {
            s = NextParam();
            if( s.empty() )
            {
               printErrorMessage( "Value missing for FilterDef parameter" );
               ExitCode = 1;
               break;
            }
            FilterDef = s.front() == 'Y' || s.front() == 'y' || s.front() == '1';
            continue;
         }
         if( s == "CDIM" || s == "CDIM=" )
         {
            s = NextParam();
            if( s.empty() )
            {
               printErrorMessage( "Value missing for CDim parameter" );
               ExitCode = 1;
               break;
            }
            CDim = s.front() == 'Y' || s.front() == 'y' || s.front() == '1';
            continue;
         }
         if( s == "-V" || s == "-VERSION" )
         {
            VersionOnly = true;
            continue;
         }
         if( s == "EPSOUT" || s == "EPSOUT=" )
         {
            EpsOut = NextParam();
            bEpsOut = true;
            continue;
         }
         if( s == "NAOUT" || s == "NAOUT=" )
         {
            NaOut = NextParam();
            bNaOut = true;
            continue;
         }
         if( s == "PINFOUT" || s == "PINFOUT=" )
         {
            PinfOut = NextParam();
            bPinfOut = true;
            continue;
         }
         if( s == "MINFOUT" || s == "MINFOUT=" )
         {
            MinfOut = NextParam();
            bMinfOut = true;
            continue;
         }
         if( s == "UNDFOUT" || s == "UNDFOUT=" )
         {
            UndfOut = NextParam();
            bUndfOut = true;
            continue;
         }
         if( s == "ZEROOUT" || s == "ZEROOUT=" )
         {
            ZeroOut = NextParam();
            bZeroOut = true;
            continue;
         }
         if( s == "HEADER" || s == "HEADER=" )
         {
            Header = NextParam();
            bHeader = true;
            continue;
         }
         printErrorMessage( "Unrecognized option: " + s.string(), false );
         ExitCode = 1;
         break;
      }
   }

   // The following line has been moved to fix errors with gotos
   std::filesystem::path InputFilePath( InputFile );

   if( ExitCode != 0 )
   {
      Usage();
      goto End;
   }

   if( OutFormat == TOutFormat::fmt_none )
      OutFormat = TOutFormat::fmt_normal;
   if( dblFormat == TDblFormat::dbl_none )
      dblFormat = TDblFormat::dbl_normal;
   if( Delim == '\0' )
   {
      if( OutFormat == TOutFormat::fmt_csv )
         Delim = ',';
      else
         Delim = '.';
   }
   if( DecimalSep == '\0' )
      DecimalSep = '.';
   if( OutFormat == TOutFormat::fmt_csv )
      ShowData = true;
   if( Delim == DecimalSep && Delim != '.' )
   {
      printErrorMessage( "Delimiter and Decimal separator characters should be different" );
      ExitCode = 1;
      goto End;
   }
   if( OutFormat == TOutFormat::fmt_csv && Symb.empty() )
   {
      printErrorMessage( "Symbol not specified when writing a CSV file" );
      ExitCode = 1;
      goto End;
   }

   if( !std::filesystem::exists( InputFilePath ) && InputFilePath.extension().string().empty() )
   {
      InputFilePath.replace_extension( std::filesystem::path( "gdx" ) );
      InputFile = InputFilePath.string();
   }
   if( !std::filesystem::exists( InputFilePath ) )
   {
      printErrorMessage( "GDX file not found: " + InputFile, false );
      ExitCode = 2;
      goto End;
   }

   if( Symb.empty() )
      ShowHdr = true;
   if( OutFormat == TOutFormat::fmt_gamsbas )
   {
      ShowHdr = false;
      ShowData = true;
   }

   {
      library::short_string error_message {};

      if( !gdxGetReady( error_message.data(), error_message.length() ) )
      {
         printErrorMessage( "Error loading GDX library: " + error_message.string(), false );
         ExitCode = 3;
         goto End;
      }

      if( !gdxCreate( &PGX, error_message.data(), error_message.length() ) )
      {
         printErrorMessage( "Error using GDX library: " + error_message.string(), false );
         ExitCode = 3;
         goto End;
      }
   }

   gdxOpenRead( PGX, InputFile.data(), &ErrNr );
   if( ErrNr != 0 )
   {
      gdxErrorStr( PGX, ErrNr, s.data() );
      printErrorMessage( "Problem reading GDX file: " + s.string(), false );
      ExitCode = 4;
      goto End;
   }

   ErrNr = gdxGetLastError( PGX );
   if( ErrNr != 0 )
   {
      gdxErrorStr( PGX, ErrNr, s.data() );
      printErrorMessage( "Problem reading GDX file: " + s.string(), false );
      ExitCode = 5;
      goto End;
   }

   if( VersionOnly )
   {
      library::short_string FileStr {}, ProduceStr {};
      gdxFileVersion( PGX, FileStr.data(), ProduceStr.data() );
      int NrSy, NrUel, FileVer, ComprLev;
      gdxSystemInfo( PGX, &NrSy, &NrUel );
      gdxFileInfo( PGX, &FileVer, &ComprLev );
      std::cout << "*  File version   : " << FileStr.data() << '\n'
                << "*  Producer       : " << ProduceStr.data() << '\n'
                << "*  File format    : " << gdlib::strutilx::IntToNiceStrW( FileVer, 4 ).data() << '\n'
                << "*  Compression    : " << gdlib::strutilx::IntToNiceStrW( ComprLev, 4 ).data() << '\n'
                << "*  Symbols        : " << gdlib::strutilx::IntToNiceStrW( NrSy, 4 ).data() << '\n'
                << "*  Unique Elements: " << gdlib::strutilx::IntToNiceStrW( NrUel, 4 ).data() << std::endl;
      goto AllDone;
   }

   if( !OutputName.empty() )
   {
      OutputFile.open( OutputName );
      if( !OutputFile.is_open() )
      {
         ErrNr = errno;
         printErrorMessage( "Error opening output file: " + OutputName, false );
         printErrorMessage( "Error: " + std::to_string( ErrNr ) + " = " + strerror( errno ), false );
         ExitCode = 6;
         goto End;
      }
      fo.rdbuf( OutputFile.rdbuf() );
   }

   if( !ShowData )
   {
      fo << '\n'
         << "$gdxIn " << InputFile << '\n';
   }
   if( OutFormat != TOutFormat::fmt_csv )
      WriteAcronyms();
   if( !UELSetName.empty() )
   {
      if( OutFormat == TOutFormat::fmt_csv )
      {
         WriteUELTable( {} );
         goto AllDone;
      }
      fo << '\n';
      WriteUELTable( UELSetName );
   }
   if( ListAllSymbols )
   {
      WriteSymbolInfo();
      goto AllDone;
   }
   if( ListSymbolsAsSet )
   {
      WriteSymbolsAsSet( false );
      goto AllDone;
   }
   if( ListSymbolsAsSetDI )
   {
      WriteSymbolsAsSet( true );
      goto AllDone;
   }
   if( DomainInfo )
   {
      WriteDomainInfo();
      goto AllDone;
   }
   if( showSetText )
   {
      WriteSetText();
      goto AllDone;
   }

   LineCount = 0;
   if( !Symb.empty() )
   {
      int N;
      if( gdxFindSymbol( PGX, Symb.data(), &N ) != 0 )
      {
         if( OutFormat == TOutFormat::fmt_csv )
            WriteSymbolCSV( N );
         else
         {
            WriteSymbol( N );
            if( !ShowHdr )
               fo << '\n';
         }
      }
      else
      {
         fo << "Symbol not found: " << Symb.data() << '\n';
         ExitCode = 6;
      }
   }
   else
   {
      if( OutFormat == TOutFormat::fmt_normal )
         ShowHdr = true;
      fo << "$onEmpty" << '\n';
      if( !ShowData )
      {
         fo << "$offEolCom" << '\n'
            << "$eolCom !!" << '\n';
      }
      int NrSy, NrUel;
      gdxSystemInfo( PGX, &NrSy, &NrUel );
      for( int N { 1 }; N <= NrSy; N++ )
         WriteSymbol( N );
      fo << '\n'
         << "$offEmpty" << '\n';
   }
   if( OutFormat == TOutFormat::fmt_gamsbas && LineCount > 6 )
      fo << "$onListing" << '\n';

AllDone:
   if( PGX )
   {
      if( gdxClose( PGX ) != 0 )
         printErrorMessage( "Problem closing GDX file", false );
      gdxFree( &PGX );
   }

   if( UsingIDE && !OutputName.empty() )
      std::cout << "GDXDump file written: " << OutputName << "[FIL:\"" << OutputName << "\",0,0]" << std::endl;

End:
   if( OutputFile.is_open() )
      OutputFile.close();

   return ExitCode;
}

}// namespace gdxdump

int main( const int argc, const char *argv[] )
{
   gdxdump::initDblUtilValues();
   return gdxdump::main( argc, argv );
}
