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

#if defined( _WIN32 )
#include <Windows.h>
#undef max
#endif

#if defined( GXFILE_DUMMY )
#include "dummy/gdxdummy.h"
#else
#include "gclgms.h"// for GMS_SSSIZE, GMS_VAL_LEVEL
#include "../gdx.h"   // for TGXFileObj, TgdxValues
#endif

#include "doctest.h"// for ResultBuilder, Expressi...

#include <algorithm> // for fill_n, find, copy
#include <cstdlib>   // for system, setenv, unsetenv
#include <cstring>   // for strcmp, memcpy
#include <filesystem>// for remove, exists
#include <iostream>  // for char_traits, operator<<
#include <limits>    // for numeric_limits
#include <list>      // for list, operator!=, _List...
#include <map>       // for map, operator!=, _Rb_tr...
#include <tuple>     // for tuple
#include <utility>   // for pair

#include "gdxtests.h"

using namespace std::literals::string_literals;
using namespace gdx;

namespace gdx::tests::gdxtests
{
TEST_SUITE_BEGIN( "GDX object tests" );

static std::string gamsToolCall( const std::string &toolName )
{
   std::string pf, sf;
   // when running in "gms test" directory (in nightly build) the executable is located inside the system directory
#if defined( _WIN32 )
   if( std::filesystem::exists( toolName + ".exe"s ) )
   {
      pf = ".\\"s;
      sf = ".exe"s;
   }
#else
   if( std::filesystem::exists( toolName ) )
      pf = "./"s;
#endif
   return pf + toolName + sf;
}

static bool hasGAMSinstalled()
{
   int rc { std::system( ( gamsToolCall( "gamslib"s ) + " trnsport"s ).c_str() ) };
   std::cout << "gamslib RC="s << rc << std::endl;
   if( std::filesystem::exists( "trnsport.gms" ) )
   {
      std::filesystem::remove( "trnsport.gms" );
      return true;
   }
   return false;
}

#if !defined( _WIN32 )
static bool setEnvironmentVariableUnix( const std::string &name, const std::string &value = ""s )
{
   if( name.empty() ) return false;
   if( value.empty() )
   {// delete name from env
      unsetenv( name.c_str() );
      return true;
   }
   return !setenv( name.c_str(), value.c_str(), 1 );
}
#endif

bool setEnvironmentVar( const std::string &name, const std::string &val )
{
#if defined( _WIN32 ) || defined( _WIN64 )
   return SetEnvironmentVariableA( name.c_str(), val.c_str() );
#else
   return setEnvironmentVariableUnix( name, val );
#endif
}


void unsetEnvironmentVar( const std::string &name )
{
#if defined( _WIN32 ) || defined( _WIN64 )
   SetEnvironmentVariableA( name.c_str(), nullptr );
#else
   setEnvironmentVariableUnix( name );
#endif
}

void basicTest( const std::function<void( TGXFileObj & )> &cb )
{
   static std::string ErrMsg;
   TGXFileObj pgx { ErrMsg };
   cb( pgx );
}

void testRead( const std::string &filename, const std::function<void( TGXFileObj & )> &cb )
{
   if( filename.empty() ) return;

   {
      std::string ErrMsg;
      TGXFileObj pgx { ErrMsg };
      int ErrNr, rc;
      rc = pgx.gdxOpenRead( filename.c_str(), ErrNr );
      REQUIRE( rc );
      cb( pgx );
      rc = pgx.gdxClose();
   }
}

void testWrite( const std::string &filename, const std::function<void( TGXFileObj & )> &cb )
{
   if( filename.empty() ) return;

   {
      std::string ErrMsg;
      TGXFileObj pgx { ErrMsg };
      int ErrNr;
      REQUIRE( pgx.gdxOpenWrite( filename.c_str(), "gdxtest", ErrNr ) );
      cb( pgx );
      pgx.gdxClose();
   }
}

TEST_CASE( "Simple setup and teardown of a GDX object" )
{
   basicTest( []( TGXFileObj &pgx ) {
      REQUIRE_EQ( 0, pgx.gdxErrorCount() );
      REQUIRE_EQ( 0, pgx.gdxGetMemoryUsed() );
      int symbolCount, uelCount;
      REQUIRE( pgx.gdxSystemInfo( symbolCount, uelCount ) );
      REQUIRE_EQ( 0, symbolCount );
      REQUIRE_EQ( 0, uelCount );
   } );
}

TEST_CASE( "Check DLL version" )
{
   basicTest( []( TGXFileObj &pgx ) {
      std::array<char, GMS_SSSIZE> versionStr {};
      REQUIRE( pgx.gdxGetDLLVersion( versionStr.data() ) );
      std::string expectedPrefix { "GDX Library"s };
      versionStr[expectedPrefix.length()] = '\0';
      REQUIRE( !std::strcmp( expectedPrefix.c_str(), versionStr.data() ) );
   } );
}

TEST_CASE( "Just create a file" )
{
   const std::string fn { "create.gdx" };
   basicTest( [&]( TGXFileObj &pgx ) {
      REQUIRE_EQ( 0, pgx.gdxErrorCount() );

      int ErrNr;
      REQUIRE_FALSE( pgx.gdxOpenWrite( "", "gdxtest", ErrNr ) );
      REQUIRE_NE( 0, ErrNr );
      REQUIRE_EQ( 0, pgx.gdxErrorCount() );
      char msg[GMS_SSSIZE];
      REQUIRE( pgx.gdxErrorStr( pgx.gdxGetLastError(), msg ) );
      REQUIRE( !strcmp( "File name is empty", msg ) );

      int fileVer, comprLev;

      REQUIRE( pgx.gdxOpenWrite( fn.c_str(), "gdxtest", ErrNr ) );
      REQUIRE_FALSE( ErrNr );
      REQUIRE_EQ( 0, pgx.gdxErrorCount() );

      REQUIRE( pgx.gdxFileInfo( fileVer, comprLev ) );
      REQUIRE_EQ( 7, fileVer );
      REQUIRE_EQ( 0, comprLev );

      std::array<char, GMS_SSSIZE> fileStr {}, producerStr {};
      REQUIRE( pgx.gdxFileVersion( fileStr.data(), producerStr.data() ) );
      std::string fs { fileStr.data() }, ps { producerStr.data() }, expFsPrefix { "GDX Library" };
      REQUIRE_EQ( "gdxtest"s, ps );
      REQUIRE_EQ( expFsPrefix, fs.substr( 0, expFsPrefix.length() ) );

      pgx.gdxClose();
      REQUIRE( std::filesystem::exists( fn ) );

      std::filesystem::remove( fn );
      REQUIRE( pgx.gdxOpenWriteEx( fn.c_str(), "gdxtest", 1, ErrNr ) );
      REQUIRE_FALSE( ErrNr );
      REQUIRE_EQ( 0, pgx.gdxErrorCount() );

      REQUIRE_FALSE( pgx.gdxOpenWriteEx( fn.c_str(), "gdxtest", 1, ErrNr ) );

      REQUIRE( pgx.gdxFileInfo( fileVer, comprLev ) );
      REQUIRE_EQ( 7, fileVer );
      REQUIRE_EQ( 1, comprLev );

      pgx.gdxClose();
      REQUIRE( std::filesystem::exists( fn ) );

      REQUIRE( pgx.gdxFileInfo( fileVer, comprLev ) );
      REQUIRE_EQ( 0, fileVer );
      REQUIRE_EQ( 0, comprLev );
   } );
   std::filesystem::remove( fn );
}

TEST_CASE( "Creating a file should also work when closing was forgotten" )
{
   std::string ErrMsg, fn { "tmp.gdx"s };
   int ErrNr;
   {
      TGXFileObj pgx { ErrMsg };
      REQUIRE( pgx.gdxOpenWrite( fn.c_str(), "gdxtest", ErrNr ) );
   }
   REQUIRE( std::filesystem::exists( fn ) );
   std::filesystem::remove( fn );
}

TEST_CASE( "Test trying to open a file for reading that does not exist" )
{
   basicTest( [&]( TGXFileObj &pgx ) {
      auto checksAfterOpenAttempt = [&]( int ErrNr ) {
         REQUIRE_EQ( 2, ErrNr );
         char errMsg[GMS_SSSIZE];
         REQUIRE( pgx.gdxErrorStr( ErrNr, errMsg ) );
         REQUIRE( !strcmp( "No such file or directory", errMsg ) );
         pgx.gdxClose();
      };
      int ErrNr;
      REQUIRE_FALSE( pgx.gdxOpenRead( "doesNotExist", ErrNr ) );
      checksAfterOpenAttempt( ErrNr );
      REQUIRE_FALSE( pgx.gdxOpenReadEx( "doesNotExist", /*gmsstrm::fmOpenRead*/ 0, ErrNr ) );
      checksAfterOpenAttempt( ErrNr );
   } );
}

TEST_CASE( "Test renaming an UEL" )
{
   std::string fn { "rename_uel.gdx" };
   basicTest( [&]( TGXFileObj &pgx ) {
      REQUIRE_EQ( -1, pgx.gdxRenameUEL( "", "" ) );

      int ErrNr;
      REQUIRE( pgx.gdxOpenWrite( fn.c_str(), "gdxtest", ErrNr ) );

      REQUIRE( pgx.gdxUELRegisterStrStart() );
      int uelNr;
      REQUIRE( pgx.gdxUELRegisterStr( "myuel", uelNr ) );
      REQUIRE( pgx.gdxUELRegisterDone() );

      char queriedUel[GMS_SSSIZE];
      int uelMap;
      REQUIRE( pgx.gdxUMUelGet( 1, queriedUel, uelMap ) );
      REQUIRE_EQ( "myuel"s, queriedUel );

      REQUIRE_FALSE( pgx.gdxRenameUEL( "myuel", "newname" ) );
      REQUIRE( pgx.gdxUMUelGet( 1, queriedUel, uelMap ) );
      REQUIRE_EQ( "newname"s, queriedUel );

      std::string tooLong( 64, 'x' );                                       // max uel length is 63
      REQUIRE_EQ( -100017, pgx.gdxRenameUEL( "newname", tooLong.c_str() ) );// ERR_BADUELSTR

      REQUIRE_EQ( 2, pgx.gdxRenameUEL( "doesNotExist", "anything" ) );
      REQUIRE_EQ( 3, pgx.gdxRenameUEL( "newname", "newname" ) );

      std::string justOkLength( 63, 'x' );
      REQUIRE_EQ( 0, pgx.gdxRenameUEL( "newname", justOkLength.c_str() ) );
      REQUIRE( pgx.gdxUMUelGet( 1, queriedUel, uelMap ) );
      REQUIRE_EQ( justOkLength, queriedUel );

      pgx.gdxClose();
   } );
   std::filesystem::remove( fn );
}

TEST_CASE( "Test get current active symbol dimension" )
{
   std::string fn { "sym_dim.gdx" };
   basicTest( [&]( TGXFileObj &pgx ) {
      int ErrNr;
      REQUIRE( pgx.gdxOpenWrite( fn.c_str(), "gdxtest", ErrNr ) );
      REQUIRE( pgx.gdxDataWriteRawStart( "mysym", "Some explanatory text.", 2, dt_par, 0 ) );
      REQUIRE_EQ( 2, pgx.gdxCurrentDim() );
      REQUIRE( pgx.gdxDataWriteDone() );
      REQUIRE_EQ( 5, pgx.gdxSymbMaxLength() );
      pgx.gdxClose();
   } );
   std::filesystem::remove( fn );
}

TEST_CASE( "Test adding uels (raw mode)" )
{
   std::string filename { "uel.gdx"s };
   testWrite( filename, []( TGXFileObj &pgx ) {
      REQUIRE( pgx.gdxUELRegisterRawStart() );
      REQUIRE( pgx.gdxUELRegisterRaw( "" ) );
      REQUIRE( pgx.gdxUELRegisterRaw( "New-York" ) );
      std::string stillOk( 63, 'i' ), tooLong( 64, 'i' );
      REQUIRE( pgx.gdxUELRegisterRaw( stillOk.c_str() ) );
      REQUIRE_FALSE( pgx.gdxUELRegisterRaw( tooLong.c_str() ) );
      REQUIRE( pgx.gdxUELRegisterDone() );
   } );
   testRead( filename, []( TGXFileObj &pgx ) {
      int uelCnt, highMap, uelMap;
      char uel[GMS_SSSIZE];
      REQUIRE( pgx.gdxUMUelInfo( uelCnt, highMap ) );
      REQUIRE_EQ( 0, highMap );
      REQUIRE_EQ( 3, uelCnt );
      REQUIRE( pgx.gdxUMUelGet( 1, uel, uelMap ) );
      REQUIRE_EQ( '\0', uel[0] );
      REQUIRE_EQ( -1, uelMap );
      REQUIRE( pgx.gdxUMUelGet( 2, uel, uelMap ) );
      REQUIRE( !strcmp( "New-York", uel ) );
      REQUIRE_EQ( -1, uelMap );
      REQUIRE_FALSE( pgx.gdxGetUEL( 2, uel ) );
      REQUIRE( strcmp( "New-York", uel ) );
      REQUIRE_FALSE( pgx.gdxUMUelGet( 23, uel, uelMap ) );
      REQUIRE_EQ( "?L__23"s, uel );
      REQUIRE_EQ( -1, uelMap );
      int uelNr;
      REQUIRE( pgx.gdxUMFindUEL( "New-York", uelNr, uelMap ) );
      REQUIRE_EQ( 2, uelNr );
      REQUIRE_EQ( -1, uelMap );
   } );
   std::filesystem::remove( filename );
}

TEST_CASE( "Test adding uels (string mode)" )
{
   auto filename { "uel.gdx"s };
   testWrite( filename, []( TGXFileObj &pgx ) {
      REQUIRE( pgx.gdxUELRegisterStrStart() );
      int uelNr;
      REQUIRE( pgx.gdxUELRegisterStr( "TheOnlyUEL", uelNr ) );
      REQUIRE_EQ( 1, uelNr );
      std::string stillOk( 63, 'i' ), tooLong( 64, 'i' );
      REQUIRE( pgx.gdxUELRegisterStr( stillOk.c_str(), uelNr ) );
      REQUIRE_FALSE( pgx.gdxUELRegisterStr( tooLong.c_str(), uelNr ) );
      REQUIRE( pgx.gdxUELRegisterDone() );
   } );
   testRead( filename, []( TGXFileObj &pgx ) {
      int uelCnt, highMap, uelMap;
      char uel[GMS_SSSIZE];
      REQUIRE( pgx.gdxUMUelInfo( uelCnt, highMap ) );
      REQUIRE_EQ( 0, highMap );
      REQUIRE_EQ( 2, uelCnt );
      REQUIRE( pgx.gdxUMUelGet( 1, uel, uelMap ) );
      REQUIRE( !strcmp( "TheOnlyUEL", uel ) );
      REQUIRE_EQ( -1, uelMap );
      REQUIRE_FALSE( pgx.gdxGetUEL( 1, uel ) );
      REQUIRE( strcmp( "TheOnlyUEL", uel ) );
   } );
   std::filesystem::remove( filename );
}

TEST_CASE( "Test adding uels (mapped mode)" )
{
   auto filename { "uel_mapped.gdx"s };
   testWrite( filename, []( TGXFileObj &pgx ) {
      REQUIRE( pgx.gdxUELRegisterMapStart() );
      REQUIRE_FALSE( pgx.gdxUELRegisterMap( 3, std::string( 64, 'i' ).c_str() ) );
      REQUIRE( pgx.gdxUELRegisterMap( 3, "TheOnlyUEL" ) );
      // registering twice with same uel nr should not do anything
      REQUIRE( pgx.gdxUELRegisterMap( 3, "TheOnlyUEL" ) );
      // but with different number it should fail
      REQUIRE_FALSE( pgx.gdxUELRegisterMap( 42, "TheOnlyUEL" ) );
      REQUIRE( pgx.gdxUELRegisterMap( 8, std::string( 63, 'i' ).c_str() ) );
      REQUIRE( pgx.gdxUELRegisterDone() );
   } );
   testRead( filename, []( TGXFileObj &pgx ) {
      int uelCnt, highMap, uelMap;
      char uel[GMS_SSSIZE];

      REQUIRE( pgx.gdxUMUelGet( 1, uel, uelMap ) );
      REQUIRE( !strcmp( "TheOnlyUEL", uel ) );
      REQUIRE_EQ( -1, uelMap );
      REQUIRE( pgx.gdxUMUelInfo( uelCnt, highMap ) );
      REQUIRE_EQ( 0, highMap );
      REQUIRE_EQ( 2, uelCnt );

      REQUIRE( pgx.gdxUELRegisterMapStart() );
      REQUIRE( pgx.gdxUELRegisterMap( 3, "TheOnlyUEL" ) );
      REQUIRE( pgx.gdxUELRegisterDone() );

      REQUIRE( pgx.gdxUMUelGet( 1, uel, uelMap ) );
      REQUIRE( !strcmp( "TheOnlyUEL", uel ) );
      REQUIRE_EQ( 3, uelMap );

      REQUIRE( pgx.gdxUMUelInfo( uelCnt, highMap ) );
      REQUIRE_EQ( 3, highMap );
      REQUIRE_EQ( 2, uelCnt );

      REQUIRE( pgx.gdxGetUEL( 3, uel ) );
      REQUIRE( !strcmp( "TheOnlyUEL", uel ) );
   } );
   std::filesystem::remove( filename );
}

TEST_CASE( "Test write and read record raw" )
{
   std::string fn { "rwrecordraw.gdx" };
   int key;
   TgdxValues values {};
   const int nMany { 800 };
   testWrite( fn, [&]( TGXFileObj &pgx ) {
      REQUIRE( pgx.gdxUELRegisterRawStart() );
      // register UELs i1*i800
      for( int i {}; i < nMany; i++ )
      {
         std::string uel { "i"s + std::to_string( i + 1 ) };
         REQUIRE( pgx.gdxUELRegisterRaw( uel.c_str() ) );
      }
      REQUIRE( pgx.gdxUELRegisterDone() );
      // Called out of context with garbage
      REQUIRE_FALSE( pgx.gdxDataWriteRaw( nullptr, nullptr ) );

      // First symbol
      // parameter mysym(*) 'This is my symbol' / i1 3.141, i2 42.1987, i3 na, i4 inf, i5 -inf, i6 eps /;
      REQUIRE( pgx.gdxDataWriteRawStart( "mysym", "This is my symbol!", 1, dt_par, 0 ) );

      key = 1;
      values[GMS_VAL_LEVEL] = 3.141;
      REQUIRE( pgx.gdxDataWriteRaw( &key, values.data() ) );

      key = 2;
      values[GMS_VAL_LEVEL] = 42.1987;
      REQUIRE( pgx.gdxDataWriteRaw( &key, values.data() ) );
      key = 3;
      values[GMS_VAL_LEVEL] = std::numeric_limits<double>::quiet_NaN();
      REQUIRE( pgx.gdxDataWriteRaw( &key, values.data() ) );

      key = 4;
      values[GMS_VAL_LEVEL] = std::numeric_limits<double>::infinity();
      REQUIRE( pgx.gdxDataWriteRaw( &key, values.data() ) );

      key = 5;
      values[GMS_VAL_LEVEL] = -std::numeric_limits<double>::infinity();
      REQUIRE( pgx.gdxDataWriteRaw( &key, values.data() ) );

      REQUIRE( pgx.gdxDataWriteDone() );

      // Second symbol
      // scalar myscalar 'This is a scalar!' /23/;
      REQUIRE( pgx.gdxDataWriteRawStart( "myscalar", "This is a scalar!", 0, dt_par, 0 ) );
      key = 0;
      values[GMS_VAL_LEVEL] = 23.0;
      REQUIRE( pgx.gdxDataWriteRaw( &key, values.data() ) );
      REQUIRE( pgx.gdxDataWriteDone() );

      // Third symbol
      // parameter mysymbig 'This is another symbol' /i1 42.1987, i2 43.1987, i3 44.1987, ... /
      REQUIRE( pgx.gdxDataWriteRawStart( "mysymbig", "This is another symbol!", 1, dt_par, 0 ) );
      for( int i {}; i < nMany; i++ )
      {
         key = 1 + i;
         values[GMS_VAL_LEVEL] = 42.1987 + i;
         REQUIRE( pgx.gdxDataWriteRaw( &key, values.data() ) );
      }
      REQUIRE( pgx.gdxDataWriteDone() );

      // Fourth symbol
      // scalar emptyscalar 'A scalar without any records';
      REQUIRE( pgx.gdxDataWriteRawStart( "emptyscalar", "A scalar without any records", 0, dt_par, 0 ) );
      REQUIRE( pgx.gdxDataWriteDone() );

      // Fifth symbol
      // variable emptyvar 'A variable without any records';
      REQUIRE( pgx.gdxDataWriteRawStart( "emptyvar", "A variable without any records", 0, dt_var, GMS_VARTYPE_INTEGER ) );
      REQUIRE( pgx.gdxDataWriteDone() );

      // Sixth symbol
      // equation emptyequ 'An equation without any records';
      REQUIRE( pgx.gdxDataWriteRawStart( "emptyequ", "An equation without any records", 0, dt_equ, GMS_EQUTYPE_L ) );
      REQUIRE( pgx.gdxDataWriteDone() );
   } );
   testRead( fn, [&]( TGXFileObj &pgx ) {
      char uel[GMS_SSSIZE];
      int uelMap;
      REQUIRE( pgx.gdxUMUelGet( 1, uel, uelMap ) );
      REQUIRE( !strcmp( "i1", uel ) );
      REQUIRE_EQ( -1, uelMap );
      REQUIRE( pgx.gdxUMUelGet( 2, uel, uelMap ) );
      REQUIRE( !strcmp( "i2", uel ) );
      REQUIRE_EQ( -1, uelMap );

      int NrRecs;
      REQUIRE( pgx.gdxDataReadRawStart( 1, NrRecs ) );
      REQUIRE_EQ( 5, NrRecs );

      int dimFrst;
      REQUIRE( pgx.gdxDataReadRaw( &key, values.data(), dimFrst ) );
      REQUIRE_EQ( 1, key );
      REQUIRE_EQ( 3.141, values[GMS_VAL_LEVEL] );

      REQUIRE( pgx.gdxDataReadRaw( &key, values.data(), dimFrst ) );
      REQUIRE_EQ( 2, key );
      REQUIRE_EQ( 42.1987, values[GMS_VAL_LEVEL] );

      REQUIRE( pgx.gdxDataReadRaw( &key, values.data(), dimFrst ) );
      REQUIRE_EQ( 3, key );
      REQUIRE_EQ( GMS_SV_NA, values[GMS_VAL_LEVEL] );

      REQUIRE( pgx.gdxDataReadRaw( &key, values.data(), dimFrst ) );
      REQUIRE_EQ( 4, key );
      REQUIRE_EQ( GMS_SV_PINF, values[GMS_VAL_LEVEL] );

      REQUIRE( pgx.gdxDataReadRaw( &key, values.data(), dimFrst ) );
      REQUIRE_EQ( 5, key );
      REQUIRE_EQ( GMS_SV_MINF, values[GMS_VAL_LEVEL] );

      REQUIRE( pgx.gdxDataReadDone() );

      // Scalar with single record storing its specific value of 23
      REQUIRE( pgx.gdxDataReadRawStart( 2, NrRecs ) );
      REQUIRE_EQ( 1, NrRecs );
      REQUIRE( pgx.gdxDataReadRaw( &key, values.data(), dimFrst ) );
      REQUIRE_EQ( 23, values[GMS_VAL_LEVEL] );
      REQUIRE( pgx.gdxDataReadDone() );

      // Empty scalar defaults to value 0
      REQUIRE( pgx.gdxDataReadRawStart( 4, NrRecs ) );
      REQUIRE_EQ( 1, NrRecs );
      REQUIRE( pgx.gdxDataReadRaw( &key, values.data(), dimFrst ) );
      REQUIRE_EQ( 0, values[GMS_VAL_LEVEL] );
      REQUIRE_EQ( 0, dimFrst );
      REQUIRE( pgx.gdxDataReadDone() );

      // Empty variable
      REQUIRE( pgx.gdxDataReadRawStart( 5, NrRecs ) );
      REQUIRE( pgx.gdxDataReadRaw( &key, values.data(), dimFrst ) );
      REQUIRE_EQ( std::array { 0.0, 0.0, 0.0, GMS_SV_PINF, 1.0 }, values );
      REQUIRE( pgx.gdxDataReadDone() );

      // Empty equation
      REQUIRE( pgx.gdxDataReadRawStart( 6, NrRecs ) );
      REQUIRE( pgx.gdxDataReadRaw( &key, values.data(), dimFrst ) );
      REQUIRE_EQ( std::array { 0.0, 0.0, GMS_SV_MINF, 0.0, 1.0 }, values );
      REQUIRE( pgx.gdxDataReadDone() );

      REQUIRE( pgx.gdxDataReadRawStart( 3, NrRecs ) );
      REQUIRE_EQ( nMany, NrRecs );
      for( int i {}; i < nMany; i++ )
      {
         REQUIRE( pgx.gdxDataReadRaw( &key, values.data(), dimFrst ) );
         REQUIRE_EQ( 1 + i, key );
         REQUIRE_EQ( 42.1987 + i, values[GMS_VAL_LEVEL] );
      }
      REQUIRE( pgx.gdxDataReadDone() );

      auto recordCallback = []( const int *keys, const double *vals ) {
         static std::array expVals { 3.141, 42.1987, GMS_SV_NA, GMS_SV_PINF, GMS_SV_MINF };
         static int cnt {};
         REQUIRE_EQ( cnt + 1, keys[0] );
         REQUIRE_EQ( expVals[cnt++], vals[GMS_VAL_LEVEL] );
      };
      REQUIRE( pgx.gdxDataReadRawFast( 1, recordCallback, NrRecs ) );

      auto recordCallbackEx = []( const int *keys, const double *vals, int afdim, void *uptr ) {
         static_cast<std::list<std::tuple<int, double, int>> *>( uptr )->emplace_back( keys[0], vals[0], afdim );
         return 1;
      };
      std::list<std::tuple<int, double, int>>
              collectedRecords {},
              expectedRecords { { 1, 3.141, 1 }, { 2, 42.1987, 1 }, { 3, GMS_SV_NA, 1 }, { 4, GMS_SV_PINF, 1 }, { 5, GMS_SV_MINF, 1 } };
      REQUIRE( pgx.gdxDataReadRawFastEx( 1, recordCallbackEx, NrRecs, static_cast<void *>( &collectedRecords ) ) );
      REQUIRE_EQ( expectedRecords, collectedRecords );

      auto recordCallbackFiltered = []( const int *keys, const double *vals, void *uptr ) {
         REQUIRE( uptr );
         REQUIRE_EQ( 1, keys[0] );
         REQUIRE_EQ( 3.141, vals[0] );
         return 1;
      };
      std::string uelFilterName { "i1"s };
      std::array<const char *, 1> uelFilterNames { uelFilterName.c_str() };
      REQUIRE( pgx.gdxDataReadRawFastFilt( 1, uelFilterNames.data(), recordCallbackFiltered ) );

      REQUIRE( pgx.gdxDataReadRawStart( 2, NrRecs ) );
      REQUIRE_EQ( 1, NrRecs );
      REQUIRE( pgx.gdxDataReadDone() );
   } );
   std::filesystem::remove( fn );
}

TEST_CASE( "Test write and read record in string mode" )
{
   std::string fn { "rwrecordstr.gdx" };
   StrIndexBuffers keyNames;
   TgdxValues values {};
   std::string stillOk( 255, 'b' );
   stillOk.front() = 'a';
   stillOk.back() = 'c';
   testWrite( fn, [&]( TGXFileObj &pgx ) {
      std::string tooLongForSymName( GLOBAL_UEL_IDENT_SIZE, 'b' );
      tooLongForSymName.front() = 'a';
      tooLongForSymName.back() = 'c';
      REQUIRE_FALSE( pgx.gdxDataWriteStrStart( tooLongForSymName.c_str(), "ok", 1, dt_par, 0 ) );

      REQUIRE( pgx.gdxDataWriteStrStart( "mysym", stillOk.c_str(), 1, dt_par, 0 ) );
      values[GMS_VAL_LEVEL] = 3.141;

      char empty = '\0';
      const char *emptyPtr = &empty;
      REQUIRE( pgx.gdxDataWriteStr( &emptyPtr, values.data() ) );

      keyNames[0] = "TheOnlyUEL"s;
      const char *keyptrs[] = { keyNames[0].c_str() };
      REQUIRE( pgx.gdxDataWriteStr( keyptrs, values.data() ) );

      std::string almostTooLongButStillOk( 63, 'i' );
      keyNames[0] = almostTooLongButStillOk;
      keyptrs[0] = keyNames[0].c_str();
      REQUIRE( pgx.gdxDataWriteStr( keyptrs, values.data() ) );

      std::string oneCharTooLong( 64, 'i' );
      keyNames[0] = oneCharTooLong;
      keyptrs[0] = keyNames[0].c_str();
      REQUIRE_FALSE( pgx.gdxDataWriteStr( keyptrs, values.data() ) );

      REQUIRE( pgx.gdxDataWriteDone() );

      std::string tooLongForExplTxt( GMS_SSSIZE, 'b' );
      tooLongForExplTxt.front() = 'a';
      tooLongForExplTxt.back() = 'c';
      REQUIRE( pgx.gdxDataWriteStrStart( "mysym2", tooLongForExplTxt.c_str(), 1, dt_par, 0 ) );
      REQUIRE( pgx.gdxDataWriteDone() );

      REQUIRE( pgx.gdxDataWriteStrStart( almostTooLongButStillOk.c_str(), stillOk.c_str(), 0, dt_par, 0 ) );
      REQUIRE( pgx.gdxDataWriteDone() );
   } );
   testRead( fn, [&]( TGXFileObj &pgx ) {
      int NrRecs, UserInfo;
      char ExplTxt[GMS_SSSIZE];
      REQUIRE( pgx.gdxSymbolInfoX( 1, NrRecs, UserInfo, ExplTxt ) );
      REQUIRE_EQ( 3, NrRecs );
      REQUIRE_EQ( stillOk, ExplTxt );

      REQUIRE( pgx.gdxDataReadStrStart( 1, NrRecs ) );
      REQUIRE_EQ( 3, NrRecs );

      int dimFrst;
      REQUIRE( pgx.gdxDataReadStr( keyNames.ptrs(), values.data(), dimFrst ) );
      REQUIRE( keyNames[0].str().empty() );
      REQUIRE_EQ( 3.141, values[GMS_VAL_LEVEL] );

      REQUIRE( pgx.gdxDataReadStr( keyNames.ptrs(), values.data(), dimFrst ) );
      REQUIRE_EQ( "TheOnlyUEL"s, keyNames[0].str() );
      REQUIRE_EQ( 3.141, values[GMS_VAL_LEVEL] );

      REQUIRE( pgx.gdxDataReadStr( keyNames.ptrs(), values.data(), dimFrst ) );
      REQUIRE_EQ( std::string( 63, 'i' ), keyNames[0].str() );
      REQUIRE_EQ( 3.141, values[GMS_VAL_LEVEL] );

      // Nothing left!
      REQUIRE_FALSE( pgx.gdxDataReadStr( keyNames.ptrs(), values.data(), dimFrst ) );
      REQUIRE( pgx.gdxDataReadDone() );

      REQUIRE( pgx.gdxSymbolInfoX( 2, NrRecs, UserInfo, ExplTxt ) );
      REQUIRE_EQ( 0, NrRecs );
      REQUIRE_EQ( 255, std::strlen( ExplTxt ) );
      REQUIRE_EQ( "String overflow: a"s + std::string( 255 - "String overflow: a..."s.length(), 'b' ) + "..."s, ExplTxt );
   } );
   std::filesystem::remove( fn );
}

TEST_CASE( "Test getting special values" )
{
   std::array<double, GMS_SVIDX_MAX> specialValuesFromPort {};
   std::string ErrMsg;
   {
      gdx::TGXFileObj pgx { ErrMsg };
      REQUIRE( pgx.gdxGetSpecialValues( specialValuesFromPort.data() ) );
   }
}

TEST_CASE( "Test setting and resetting special values" )
{
   basicTest( [&]( TGXFileObj &pgx ) {
      std::array<double, GMS_SVIDX_MAX> moddedSpecVals {}, queriedSpecVals {};
      REQUIRE( pgx.gdxGetSpecialValues( moddedSpecVals.data() ) );
      moddedSpecVals[gdx::TgdxIntlValTyp::vm_valpin] = 0.0;
      REQUIRE( pgx.gdxSetSpecialValues( moddedSpecVals.data() ) );
      REQUIRE( pgx.gdxGetSpecialValues( queriedSpecVals.data() ) );
      REQUIRE_EQ( 0.0, queriedSpecVals[gdx::TgdxIntlValTyp::vm_valpin] );
      REQUIRE( pgx.gdxResetSpecialValues() );
      REQUIRE( pgx.gdxGetSpecialValues( queriedSpecVals.data() ) );
      REQUIRE_EQ( GMS_SV_PINF, queriedSpecVals[gdx::TgdxIntlValTyp::vm_valpin] );
      std::copy( queriedSpecVals.begin(), queriedSpecVals.end(), moddedSpecVals.begin() );
      moddedSpecVals[gdx::TgdxIntlValTyp::vm_valpin] = 0.0;
      REQUIRE( pgx.gdxSetReadSpecialValues( moddedSpecVals.data() ) );
   } );
}

void writeMappedRecordsOutOfOrder( TGXFileObj &pgx )
{
   // very irregular user uel nr -> uel name mapping
   const std::map<int, std::string> userUelMapping {
           { 3, "z" },
           { 8, "a" },
           { 1, "y" },
           { 10, "b" } };
   // also weird record write ordering
   const std::array<int, 4> randomOrder { 8, 10, 1, 3 };

   REQUIRE( pgx.gdxUELRegisterMapStart() );
   for( const auto &pair: userUelMapping )
   {
      REQUIRE( pgx.gdxUELRegisterMap( pair.first, pair.second.c_str() ) );
   }
   REQUIRE( pgx.gdxUELRegisterDone() );

   REQUIRE( pgx.gdxDataWriteMapStart( "irregularSym", "So out of order!", 1, dt_par, 0 ) );
   int key;
   TgdxValues values {};
   for( const auto ix: randomOrder )
   {
      key = ix;
      values[GMS_VAL_LEVEL] = 3.141 * ix;
      REQUIRE( pgx.gdxDataWriteMap( &key, values.data() ) );
   }
   REQUIRE( pgx.gdxDataWriteDone() );
   REQUIRE_EQ( 0, pgx.gdxErrorCount() );
   REQUIRE_EQ( 0, pgx.gdxDataErrorCount() );
}

TEST_CASE( "Test writing mapped records out of order" )
{
   std::string fn { "mapped_outoforder.gdx" };
   testWrite( fn, writeMappedRecordsOutOfOrder );
   std::filesystem::remove( fn );
}

TEST_CASE( "Test write and read record mapped - out of order" )
{
   std::string fn { "rwrecordmapped_ooo.gdx" };
   int key;
   TgdxValues values {};
   testWrite( fn, [&]( TGXFileObj &pgx ) {
      int uelCnt, highMap;
      REQUIRE( pgx.gdxUMUelInfo( uelCnt, highMap ) );
      REQUIRE_EQ( 0, uelCnt );
      REQUIRE_EQ( 0, highMap );
      REQUIRE( pgx.gdxUELRegisterMapStart() );
      REQUIRE( pgx.gdxUELRegisterMap( 3, "First" ) );
      REQUIRE( pgx.gdxUELRegisterMap( 4, "Second" ) );
      REQUIRE( pgx.gdxUELRegisterMap( 5, "Third" ) );
      REQUIRE( pgx.gdxUELRegisterMap( 2, "Fourth" ) );
      REQUIRE( pgx.gdxUELRegisterDone() );
      char uel[GMS_SSSIZE];
      REQUIRE( pgx.gdxGetUEL( 3, uel ) );
      REQUIRE( !strcmp( "First", uel ) );
      REQUIRE( pgx.gdxDataWriteMapStart( "mysym", "Single record", 1, dt_par, 0 ) );
      key = 3;
      values[GMS_VAL_LEVEL] = 3.141;
      REQUIRE( pgx.gdxDataWriteMap( &key, values.data() ) );
      REQUIRE( pgx.gdxDataWriteDone() );

      // User UEL indices non-increasing
      REQUIRE( pgx.gdxDataWriteMapStart( "mysym2", "Four records", 1, dt_par, 0 ) );
      std::array<int, 4> expKey { 3, 4, 5, 2 };
      for( int i: expKey )
         REQUIRE( pgx.gdxDataWriteMap( &i, values.data() ) );
      REQUIRE( pgx.gdxDataWriteDone() );

      constexpr char c { 'i' };
      std::string notTooLongSymID( GLOBAL_UEL_IDENT_SIZE - 1, c ),
              notTooLongExpl( GMS_SSSIZE - 1, c ),
              tooLongSymID( GLOBAL_UEL_IDENT_SIZE, c ),
              tooLongExpl( GMS_SSSIZE, c );

      REQUIRE( pgx.gdxDataWriteMapStart( notTooLongSymID.c_str(), notTooLongExpl.c_str(), 0, dt_par, 0 ) );
      REQUIRE( pgx.gdxDataWriteDone() );

      REQUIRE_EQ( 0, pgx.gdxErrorCount() );
      REQUIRE_EQ( 0, pgx.gdxDataErrorCount() );

      REQUIRE_FALSE( pgx.gdxDataWriteMapStart( tooLongSymID.c_str(), notTooLongExpl.c_str(), 0, dt_par, 0 ) );
      REQUIRE_FALSE( pgx.gdxDataWriteMapStart( notTooLongSymID.c_str(), tooLongExpl.c_str(), 0, dt_par, 0 ) );

      REQUIRE_EQ( 2, pgx.gdxErrorCount() );
      REQUIRE_EQ( 0, pgx.gdxDataErrorCount() );
   } );
   testRead( fn, [&]( TGXFileObj &pgx ) {
      char uel[GMS_SSSIZE];
      int uelMap;
      REQUIRE( pgx.gdxUMUelGet( 1, uel, uelMap ) );
      REQUIRE( !strcmp( "First", uel ) );
      REQUIRE_EQ( -1, uelMap );

      REQUIRE( pgx.gdxUELRegisterMapStart() );
      REQUIRE( pgx.gdxUELRegisterMap( 3, "First" ) );
      REQUIRE( pgx.gdxUELRegisterMap( 4, "Second" ) );
      REQUIRE( pgx.gdxUELRegisterMap( 5, "Third" ) );
      REQUIRE( pgx.gdxUELRegisterMap( 2, "Fourth" ) );
      REQUIRE( pgx.gdxUELRegisterDone() );

      REQUIRE( pgx.gdxGetUEL( 3, uel ) );
      REQUIRE( !strcmp( "First", uel ) );

      REQUIRE( pgx.gdxUMUelGet( 1, uel, uelMap ) );
      REQUIRE( !strcmp( "First", uel ) );
      REQUIRE_EQ( 3, uelMap );

      int NrRecs;
      REQUIRE( pgx.gdxDataReadMapStart( 1, NrRecs ) );
      REQUIRE_EQ( 1, NrRecs );
      int dimFrst;
      REQUIRE( pgx.gdxDataReadMap( 1, &key, values.data(), dimFrst ) );
      REQUIRE_EQ( 0, pgx.gdxDataErrorCount() );
      REQUIRE_EQ( 3, key );
      REQUIRE_EQ( 3.141, values[GMS_VAL_LEVEL] );
      REQUIRE_EQ( 1, dimFrst );
      REQUIRE( pgx.gdxDataReadDone() );

      REQUIRE( pgx.gdxDataReadMapStart( 2, NrRecs ) );
      REQUIRE_EQ( 4, NrRecs );
      // unordered mapped write comes out ordered
      std::array<int, 4> expKey { 2, 3, 4, 5 };
      for( int i { 1 }; i <= (int) expKey.size(); i++ )
      {
         REQUIRE( pgx.gdxDataReadMap( i, &key, values.data(), dimFrst ) );
         REQUIRE_EQ( expKey[i - 1], key );
      }
      REQUIRE_EQ( 0, pgx.gdxDataErrorCount() );
      REQUIRE( pgx.gdxDataReadDone() );

      std::string tooLong( 64, 'x' );
      REQUIRE_FALSE( pgx.gdxUELRegisterMap( 6, tooLong.c_str() ) );
   } );
   std::filesystem::remove( fn );
}

TEST_CASE( "Test write and read record mapped - in order" )
{
   std::string fn { "rwrecordmapped_io.gdx" };
   int key;
   TgdxValues values {};
   testWrite( fn, [&]( TGXFileObj &pgx ) {
      int uelCnt, highMap;
      REQUIRE( pgx.gdxUMUelInfo( uelCnt, highMap ) );
      REQUIRE_EQ( 0, uelCnt );
      REQUIRE_EQ( 0, highMap );
      REQUIRE( pgx.gdxUELRegisterMapStart() );
      REQUIRE( pgx.gdxUELRegisterMap( 5, "First" ) );
      REQUIRE( pgx.gdxUELRegisterMap( 6, "Second" ) );
      REQUIRE( pgx.gdxUELRegisterMap( 7, "Third" ) );
      REQUIRE( pgx.gdxUELRegisterMap( 8, "Fourth" ) );
      REQUIRE( pgx.gdxUELRegisterDone() );
      char uel[GMS_SSSIZE];
      REQUIRE( pgx.gdxGetUEL( 5, uel ) );
      REQUIRE( !strcmp( "First", uel ) );

      // User UEL indices increasing
      REQUIRE( pgx.gdxDataWriteMapStart( "mysym", "Four records", 1, dt_par, 0 ) );
      std::array<int, 4> expKey { 5, 6, 7, 8 };
      for( int i: expKey )
         REQUIRE( pgx.gdxDataWriteMap( &i, values.data() ) );
      REQUIRE( pgx.gdxDataWriteDone() );

      REQUIRE_EQ( 0, pgx.gdxErrorCount() );
      REQUIRE_EQ( 0, pgx.gdxDataErrorCount() );
   } );
   testRead( fn, [&]( TGXFileObj &pgx ) {
      char uel[GMS_SSSIZE];
      int uelMap;
      REQUIRE( pgx.gdxUMUelGet( 1, uel, uelMap ) );
      REQUIRE( !strcmp( "First", uel ) );
      REQUIRE_EQ( -1, uelMap );

      REQUIRE( pgx.gdxUELRegisterMapStart() );
      REQUIRE( pgx.gdxUELRegisterMap( 5, "First" ) );
      REQUIRE( pgx.gdxUELRegisterMap( 6, "Second" ) );
      REQUIRE( pgx.gdxUELRegisterMap( 7, "Third" ) );
      REQUIRE( pgx.gdxUELRegisterMap( 8, "Fourth" ) );
      REQUIRE( pgx.gdxUELRegisterDone() );

      REQUIRE( pgx.gdxGetUEL( 5, uel ) );
      REQUIRE( !strcmp( "First", uel ) );

      REQUIRE( pgx.gdxUMUelGet( 1, uel, uelMap ) );
      REQUIRE( !strcmp( "First", uel ) );
      REQUIRE_EQ( 5, uelMap );

      int NrRecs;
      REQUIRE( pgx.gdxDataReadMapStart( 1, NrRecs ) );
      REQUIRE_EQ( 4, NrRecs );
      // should still be ordered
      std::array<int, 4> expKey { 5, 6, 7, 8 };
      for( int i { 1 }; i <= (int) expKey.size(); i++ )
      {
         int dimFrst;
         REQUIRE( pgx.gdxDataReadMap( i, &key, values.data(), dimFrst ) );
         REQUIRE_EQ( expKey[i - 1], key );
      }
      REQUIRE_EQ( 0, pgx.gdxDataErrorCount() );
      REQUIRE( pgx.gdxDataReadDone() );
   } );
   std::filesystem::remove( fn );
}

void domainSetGetTestSetupPrefix( TGXFileObj &pgx )
{
   int numSyms, numUels;
   REQUIRE( pgx.gdxSystemInfo( numSyms, numUels ) );
   REQUIRE_EQ( 0, numUels );
   REQUIRE_EQ( 0, numSyms );

   REQUIRE( pgx.gdxDataWriteStrStart( "i", "", 1, dt_set, 0 ) );
   REQUIRE( pgx.gdxDataWriteDone() );
   REQUIRE( pgx.gdxSystemInfo( numSyms, numUels ) );
   REQUIRE_EQ( 1, numSyms );

   REQUIRE( pgx.gdxDataWriteStrStart( "j", "", 1, dt_set, 0 ) );
   REQUIRE( pgx.gdxDataWriteDone() );

   REQUIRE( pgx.gdxAddAlias( "k", "i" ) );

   REQUIRE( pgx.gdxSystemInfo( numSyms, numUels ) );
   REQUIRE_EQ( 3, numSyms );

   REQUIRE( pgx.gdxDataWriteStrStart( "newd", "Same domain as d", 2, dt_par, 0 ) );
}

void commonSetGetDomainTests( const std::vector<std::string> &domainNames,
                              const std::vector<int> &domainIndices )
{

   assert( domainNames.size() == 2 );
   assert( domainIndices.size() == 2 );

   const std::array<std::string, 2> fns {
           "setgetdomainx.gdx"s,
           "setgetdomain.gdx"s,
   };

   TgdxStrIndex newSymDomainNames {};
   std::array<int, 2> newSymDomainIndices {};
   for( int i {}; i < (int) domainNames.size(); i++ )
   {
      newSymDomainNames[i] = domainNames[i];
      newSymDomainIndices[i] = domainIndices[i];
   }

   auto domainSetGetTestSetupSuffix = []( TGXFileObj &pgx ) {
      int numSyms, numUels;
      REQUIRE( pgx.gdxDataWriteDone() );
      REQUIRE( pgx.gdxSystemInfo( numSyms, numUels ) );
      REQUIRE_EQ( 4, numSyms );

      int newSymNr;
      REQUIRE( pgx.gdxFindSymbol( "newd", newSymNr ) );
      REQUIRE_EQ( 4, newSymNr );
   };

   auto testSetGetDomainX = [&]( TGXFileObj &pgx ) {
      StrIndexBuffers sib { &newSymDomainNames };
      REQUIRE( pgx.gdxSymbolSetDomainX( 4, sib.cptrs() ) );
      StrIndexBuffers domainIds {};
      REQUIRE( pgx.gdxSymbolGetDomainX( 4, domainIds.ptrs() ) );
      REQUIRE_EQ( newSymDomainNames.front(), domainIds[0].str() );
      REQUIRE_EQ( newSymDomainNames[1], domainIds[1].str() );
   };

   auto testSetGetDomain = [&]( TGXFileObj &pgx ) {
      domainSetGetTestSetupPrefix( pgx );
      StrIndexBuffers sib { &newSymDomainNames };
      REQUIRE( pgx.gdxSymbolSetDomain( sib.cptrs() ) );
      domainSetGetTestSetupSuffix( pgx );
      std::array<int, 2> domainSyNrs {};
      REQUIRE( pgx.gdxSymbolGetDomain( 4, domainSyNrs.data() ) );
      REQUIRE_EQ( newSymDomainIndices[0], domainSyNrs[0] );
      REQUIRE_EQ( newSymDomainIndices[1], domainSyNrs[1] );
   };

   testWrite( fns[0], [&]( TGXFileObj &pgx ) {
      domainSetGetTestSetupPrefix( pgx );
      domainSetGetTestSetupSuffix( pgx );
      testSetGetDomainX( pgx );
   } );

   testRead( fns[0], [&]( TGXFileObj &pgx ) {
      StrIndexBuffers domainIds {};
      REQUIRE( pgx.gdxSymbolGetDomainX( 4, domainIds.ptrs() ) );
      REQUIRE_EQ( newSymDomainNames[0], domainIds[0].str() );
      REQUIRE_EQ( newSymDomainNames[1], domainIds[1].str() );
   } );

   testWrite( fns[1], [&]( TGXFileObj &pgx ) {
      testSetGetDomain( pgx );
   } );

   for( const auto &fn: fns )
      std::filesystem::remove( fn );
}

TEST_CASE( "Test writing with set/get domain normal and variant" )
{
   commonSetGetDomainTests( { "i", "j" }, { 1, 2 } );
   commonSetGetDomainTests( { "i", "i" }, { 1, 1 } );
   commonSetGetDomainTests( { "i", "k" }, { 1, 3 } );
}

TEST_CASE( "Test adding a set alias" )
{
   const std::string fn { "add_alias.gdx"s };

   auto queryAliasSymbolInfo = []( TGXFileObj &pgx, const std::string &aliasName, int aliasSymNr, const std::string &refSymName, int referencedSymNr ) {
      int aliasIx;
      REQUIRE( pgx.gdxFindSymbol( aliasName.c_str(), aliasIx ) );
      REQUIRE_EQ( aliasSymNr, aliasIx );
      int numRecords, userInfo, dim, typ;
      std::array<char, GMS_SSSIZE> explText {}, symbolName {};
      REQUIRE( pgx.gdxSymbolInfoX( aliasSymNr, numRecords, userInfo, explText.data() ) );
      REQUIRE( pgx.gdxSymbolInfo( aliasSymNr, symbolName.data(), dim, typ ) );
      REQUIRE_EQ( 0, numRecords );
      REQUIRE_EQ( referencedSymNr, userInfo );// symbol index of references symbol
      REQUIRE_EQ( "Aliased with "s + refSymName, explText.data() );
      REQUIRE_EQ( aliasName, symbolName.data() );
      REQUIRE_EQ( 1, dim );
      REQUIRE_EQ( dt_alias, typ );
   };

   constexpr int elemCnt { 10 };
   StrIndexBuffers sib {};
   TgdxValues vals {};

   testWrite( fn, [&]( TGXFileObj &pgx ) {
      // set i /i1*i10/;
      REQUIRE( pgx.gdxDataWriteStrStart( "i", "A set", 1, dt_set, 0 ) );
      for( int i {}; i < elemCnt; i++ )
      {
         sib.front() = "i"s + std::to_string( i + 1 );
         REQUIRE( pgx.gdxDataWriteStr( sib.cptrs(), vals.data() ) );
      }
      REQUIRE( pgx.gdxDataWriteDone() );

      int numSymbols, numUels, symbolCountBefore;
      REQUIRE( pgx.gdxSystemInfo( symbolCountBefore, numUels ) );
      REQUIRE_EQ( 10, numUels );
      REQUIRE_EQ( 1, symbolCountBefore );

      // alias(i,aliasI);
      REQUIRE( pgx.gdxAddAlias( "i", "aliasI" ) );
      REQUIRE( pgx.gdxSystemInfo( numSymbols, numUels ) );
      REQUIRE_EQ( 10, numUels );
      REQUIRE_EQ( symbolCountBefore + 1, numSymbols );
      // aliasI is symbol nr. 2 referencing symbol i with nr. 1
      queryAliasSymbolInfo( pgx, "aliasI", 2, "i", 1 );

      // alias(*, aliasUniverse);
      REQUIRE( pgx.gdxAddAlias( "*", "aliasUniverse" ) );
      REQUIRE( pgx.gdxSystemInfo( numSymbols, numUels ) );
      REQUIRE_EQ( 10, numUels );
      REQUIRE_EQ( symbolCountBefore + 2, numSymbols );
      // aliasUniverse is symbol nr. 3 referencing symbol * (universe) with nr. 0
      queryAliasSymbolInfo( pgx, "aliasUniverse", 3, "*", 0 );
   } );

   testRead( fn, [&]( TGXFileObj &pgx ) {
      queryAliasSymbolInfo( pgx, "aliasI", 2, "i", 1 );

      int numRecords;
      REQUIRE( pgx.gdxDataReadRawStart( 2, numRecords ) );
      REQUIRE_EQ( 10, numRecords );
      int key, dimFrst;
      for( int i {}; i < numRecords; i++ )
      {
         REQUIRE( pgx.gdxDataReadRaw( &key, vals.data(), dimFrst ) );
         REQUIRE_EQ( i + 1, key );
      }
      REQUIRE( pgx.gdxDataReadDone() );

      REQUIRE( pgx.gdxDataReadRawStart( 3, numRecords ) );
      REQUIRE_EQ( 10, numRecords );
      for( int i {}; i < numRecords; i++ )
      {
         REQUIRE( pgx.gdxDataReadRaw( &key, vals.data(), dimFrst ) );
         REQUIRE_EQ( i + 1, key );
      }
      REQUIRE( pgx.gdxDataReadDone() );
   } );

   std::filesystem::remove( fn );
}

TEST_CASE( "Test creating and querying element text for sets" )
{
   const std::string fn { "elemtxt.gdx" };
   int elemNode;
   char elemTxt[GMS_SSSIZE];
   TgdxUELIndex keys;
   TgdxValues values;
   testWrite( fn, [&]( TGXFileObj &pgx ) {
      int txtNr;
      REQUIRE( pgx.gdxAddSetText( "", txtNr ) );
      REQUIRE( pgx.gdxUELRegisterRawStart() );
      REQUIRE( pgx.gdxUELRegisterRaw( "onlyuel" ) );
      REQUIRE( pgx.gdxUELRegisterDone() );
      // set i 'expl' /onlyuel 'set text'/;
      REQUIRE( pgx.gdxDataWriteRawStart( "i", "expl", 1, dt_set, 0 ) );
      keys[0] = 1;
      values[GMS_VAL_LEVEL] = 1;// this determines first record/elem of set i has set text with textnr=1
      REQUIRE( pgx.gdxDataWriteRaw( keys.data(), values.data() ) );
      REQUIRE( pgx.gdxDataWriteDone() );
      REQUIRE( pgx.gdxAddSetText( "set text", txtNr ) );
      REQUIRE_EQ( 1, txtNr );
      // Check adding twice: Should give same text number!
      REQUIRE( pgx.gdxAddSetText( "set text", txtNr ) );
      REQUIRE_EQ( 1, txtNr );
      REQUIRE( pgx.gdxGetElemText( txtNr, elemTxt, elemNode ) );
      REQUIRE_EQ( "set text"s, elemTxt );
      REQUIRE_EQ( 0, elemNode );
      REQUIRE_FALSE( pgx.gdxSetTextNodeNr( 200, 42 ) );
      REQUIRE( pgx.gdxSetTextNodeNr( 1, 23 ) );
      REQUIRE( pgx.gdxGetElemText( 1, elemTxt, elemNode ) );
      REQUIRE_EQ( "set text"s, elemTxt );
      REQUIRE_EQ( 23, elemNode );
      REQUIRE( pgx.gdxSetHasText( 1 ) );
      // Set texts are case-sensitive so adding it with different casing should be a new text!
      REQUIRE( pgx.gdxAddSetText( "sEt text", txtNr ) );
      REQUIRE_EQ( 2, txtNr );
      REQUIRE( pgx.gdxAddSetText( "   ", txtNr ) );
      REQUIRE_EQ( 3, txtNr );
   } );
   testRead( fn, [&]( TGXFileObj &pgx ) {
      REQUIRE( pgx.gdxGetElemText( 1, elemTxt, elemNode ) );
      REQUIRE_EQ( "set text"s, elemTxt );
      REQUIRE_EQ( 0, elemNode );
      REQUIRE( pgx.gdxSetHasText( 1 ) );
      int nrRecs;
      REQUIRE( pgx.gdxDataReadRawStart( 1, nrRecs ) );
      REQUIRE_EQ( 1, nrRecs );
      int dimFrst;
      REQUIRE( pgx.gdxDataReadRaw( keys.data(), values.data(), dimFrst ) );
      REQUIRE( pgx.gdxDataReadDone() );
      pgx.gdxGetElemText( static_cast<int>( values[GMS_VAL_LEVEL] ), elemTxt, elemNode );
      REQUIRE_EQ( "set text"s, elemTxt );
      REQUIRE_EQ( 0, elemNode );
   } );
   std::filesystem::remove( fn );
}

TEST_CASE( "Test invalid raw writing error processing" )
{
   const std::string fn { "tmpfile.gdx"s };
   int key;
   TgdxValues values {};
   basicTest( [&]( TGXFileObj &pgx ) {
      if( std::filesystem::exists( fn ) )
         std::filesystem::remove( fn );
      int errNr;
      REQUIRE( pgx.gdxOpenWrite( fn.c_str(), "gdxinterfacetests", errNr ) );
      REQUIRE( pgx.gdxUELRegisterRawStart() );
      REQUIRE( pgx.gdxUELRegisterRaw( "onlyuel1" ) );
      REQUIRE( pgx.gdxUELRegisterRaw( "onlyuel2" ) );
      REQUIRE( pgx.gdxUELRegisterRaw( "onlyuel3" ) );
      REQUIRE( pgx.gdxUELRegisterDone() );
      REQUIRE( pgx.gdxDataWriteRawStart( "i", "expl", 1, dt_set, 0 ) );
      key = 3;
      REQUIRE( pgx.gdxDataWriteRaw( &key, values.data() ) );
      key = 1;
      REQUIRE_FALSE( pgx.gdxDataWriteRaw( &key, values.data() ) );
      REQUIRE( pgx.gdxDataWriteDone() );
      REQUIRE_EQ( 1, pgx.gdxErrorCount() );
      REQUIRE_EQ( 1, pgx.gdxDataErrorCount() );
      REQUIRE( pgx.gdxDataErrorRecord( 1, &key, values.data() ) );
      REQUIRE_EQ( 1, key );
      REQUIRE( pgx.gdxDataErrorRecordX( 1, &key, values.data() ) );
      REQUIRE_EQ( 1, key );
      int ec = pgx.gdxGetLastError();
      char errMsg[GMS_SSSIZE];
      REQUIRE( pgx.gdxErrorStr( ec, errMsg ) );
      REQUIRE_EQ( "Data not sorted when writing raw"s, errMsg );
      pgx.gdxClose();
   } );
   std::filesystem::remove( fn );
}

std::string acquireGDXforModel( const std::string &model )
{
   const std::string model_fn = model + ".gms"s,
                     log_fn = model + "Log.txt"s;
   std::string gdxfn = model + ".gdx"s;// non-const so we get automatic move
   int rc = std::system( ( gamsToolCall( "gamslib"s ) + " "s + model + " > gamslibLog.txt"s ).c_str() );
   REQUIRE_FALSE( rc );
   std::filesystem::remove( "gamslibLog.txt" );
   REQUIRE( std::filesystem::exists( model_fn ) );
   rc = std::system( ( gamsToolCall( "gams"s ) + " "s + model_fn + " gdx=default lo=0 o=lf > " + log_fn ).c_str() );
   REQUIRE_FALSE( rc );
   std::filesystem::remove( log_fn );
   std::filesystem::remove( model_fn );
   std::filesystem::remove( "lf" );
   REQUIRE( std::filesystem::exists( gdxfn ) );
   return gdxfn;
}

void testReadModelGDX( const std::string &model, const std::function<void( TGXFileObj & )> &func )
{
   if( !hasGAMSinstalled() )
   {
      std::cout << "Warning: No GAMS system found. Unable to acquire model \"" << model << "\" skipping test!" << std::endl;
      return;
   }
   const std::string gdxfn = acquireGDXforModel( model );
   testRead( gdxfn, func );
   std::filesystem::remove( gdxfn );
}

TEST_CASE( "Test reading/extracting data from gamslib/trnsport example" )
{
   if( !hasGAMSinstalled() )
   {
      std::cout << "Skipping test since GAMS system is not found!" << std::endl;
      return;
   }
   const std::array expectedSymbolNames {
           "a"s, "b"s, "c"s,
           "cost"s, "d"s, "demand"s,
           "f"s, "i"s, "j"s, "supply"s,
           "x"s, "z"s };
   const std::array expectedUels {
           "seattle"s, "san-diego"s, "new-york"s, "chicago"s, "topeka"s };
   const std::string gdxfn = acquireGDXforModel( "trnsport"s );
   testRead( gdxfn, [&]( TGXFileObj &pgx ) {
      REQUIRE_GT( pgx.gdxGetMemoryUsed(), 0 );
      int numSymbols, numUels;
      pgx.gdxSystemInfo( numSymbols, numUels );
      std::list<std::string> uelsSeen {}, symbolsSeen {};
      for( int i { 1 }; i <= numUels; i++ )
      {
         char uel[GMS_SSSIZE];
         int uelMap;
         REQUIRE( pgx.gdxUMUelGet( i, uel, uelMap ) );
         REQUIRE_NE( uel[0], '\0' );
         uelsSeen.emplace_back( uel );
      }
      REQUIRE_EQ( expectedUels.size(), uelsSeen.size() );
      for( const auto &uel: expectedUels )
         REQUIRE_NE( std::find( uelsSeen.begin(), uelsSeen.end(), uel ), uelsSeen.end() );

      for( int i { 1 }; i <= numSymbols; i++ )
      {
         char name[GMS_SSSIZE];
         int dim, typ;
         REQUIRE( pgx.gdxSymbolInfo( i, name, dim, typ ) );
         REQUIRE_GE( dim, 0 );
         REQUIRE_GE( typ, 0 );
         REQUIRE_NE( name[0], '\0' );
         symbolsSeen.emplace_back( name );

         char explanatoryText[GMS_SSSIZE];
         int userInfo, numRecords;
         REQUIRE( pgx.gdxSymbolInfoX( i, numRecords, userInfo, explanatoryText ) );
         if( numRecords )
         {
            int numRecords2;
            REQUIRE( pgx.gdxDataReadStrStart( i, numRecords2 ) );
            REQUIRE_EQ( numRecords, numRecords2 );
            StrIndexBuffers keyNames;
            TgdxValues values;
            int dimFirst;
            for( int j {}; j < numRecords; j++ )
            {
               REQUIRE( pgx.gdxDataReadStr( keyNames.ptrs(), values.data(), dimFirst ) );
               REQUIRE_GE( dimFirst, 0 );
               for( int d {}; d < dim; d++ )
                  REQUIRE( !keyNames[d].empty() );
            }
            REQUIRE( pgx.gdxDataReadDone() );
         }
      }
      REQUIRE_EQ( expectedSymbolNames.size(), symbolsSeen.size() );
      for( const auto &expName: expectedSymbolNames )
         REQUIRE_NE( std::find( symbolsSeen.begin(), symbolsSeen.end(), expName ), symbolsSeen.end() );

      int symNr;
      REQUIRE( pgx.gdxFindSymbol( "i", symNr ) );
      REQUIRE_EQ( 1, symNr );
      REQUIRE_EQ( 1, pgx.gdxSymbolDim( 1 ) );

      REQUIRE( pgx.gdxFindSymbol( "*", symNr ) );
      REQUIRE_EQ( 0, symNr );

      // d(i,j) is symbol 5
      REQUIRE( pgx.gdxFindSymbol( "d", symNr ) );
      REQUIRE_EQ( 5, symNr );
      std::array<int, 2> domainSyNrs {};
      REQUIRE( pgx.gdxSymbolGetDomain( 5, domainSyNrs.data() ) );
      REQUIRE_EQ( 2, pgx.gdxSymbolDim( 5 ) );
      // first domain of d(i,j) is i (symbol 1)
      REQUIRE_EQ( 1, domainSyNrs[0] );
      // second domain of d(i,j) is j (symbol 2)
      REQUIRE_EQ( 2, domainSyNrs[1] );

      StrIndexBuffers domainIds;
      REQUIRE( pgx.gdxSymbolGetDomainX( 5, domainIds.ptrs() ) );
      REQUIRE_EQ( "i"s, domainIds[0].str() );
      REQUIRE_EQ( "j"s, domainIds[1].str() );

      auto domIndexCallback = []( int rawIndex, int mappedIndex, void *uptr ) {
         ( (std::list<std::pair<int, int>> *) uptr )->emplace_back( rawIndex, mappedIndex );
      };

      int nrElem {};
      std::list<std::pair<int, int>> callArgs {};
      REQUIRE( pgx.gdxGetDomainElements( 5, 1, gdx::DOMC_EXPAND, domIndexCallback, nrElem, (void *) &callArgs ) );
      const std::list<std::pair<int, int>> expectedCallArgs {
              { 1, 1 },
              { 2, 2 } };
      REQUIRE_EQ( 2, nrElem );
      REQUIRE_EQ( expectedCallArgs, callArgs );

      callArgs.clear();
      nrElem = 0;
      REQUIRE( pgx.gdxGetDomainElements( 5, 1, gdx::DOMC_EXPAND, nullptr, nrElem, (void *) &callArgs ) );
      REQUIRE_EQ( 2, nrElem );
      REQUIRE( callArgs.empty() );

      REQUIRE_EQ( 6, pgx.gdxSymbMaxLength() );
   } );
   std::filesystem::remove( gdxfn );
}

TEST_CASE( "Tests related to universe" )
{
   basicTest( []( TGXFileObj &pgx ) {
      int errNr;
      auto fn = "universe_tests.gdx"s;
      REQUIRE( pgx.gdxOpenWrite( fn.c_str(), "gdxinterfacetests", errNr ) );
      REQUIRE( pgx.gdxDataWriteStrStart( "i", "A set", 1, dt_set, 0 ) );
      StrIndexBuffers keys {};
      TgdxValues vals {};
      for( int i = 1; i <= 8; i++ )
      {
         keys[0] = "uel_" + std::to_string( i );
         const char *keyptrs[] = { keys[0].c_str() };
         REQUIRE( pgx.gdxDataWriteStr( keyptrs, vals.data() ) );
      }
      REQUIRE( pgx.gdxDataWriteDone() );

      int symNr;
      REQUIRE( pgx.gdxFindSymbol( "*", symNr ) );
      REQUIRE_EQ( 0, symNr );

      char symName[GMS_SSSIZE];
      int dim, typ;
      REQUIRE( pgx.gdxSymbolInfo( 0, symName, dim, typ ) );
      REQUIRE_EQ( "*"s, symName );
      REQUIRE_EQ( 1, dim );
      REQUIRE_EQ( dt_set, typ );

      int recCnt, userInfo;
      char explText[GMS_SSSIZE];
      REQUIRE( pgx.gdxSymbolInfoX( 0, recCnt, userInfo, explText ) );
      REQUIRE_EQ( 0, recCnt );
      REQUIRE_EQ( 0, userInfo );
      REQUIRE_EQ( "Universe"s, explText );

      REQUIRE_FALSE( pgx.gdxSymbolInfoX( 999, recCnt, userInfo, explText ) );
      REQUIRE_EQ( 0, recCnt );
      REQUIRE_EQ( 0, userInfo );
      REQUIRE_EQ( explText[0], '\0' );

      pgx.gdxClose();

      REQUIRE( pgx.gdxOpenReadEx( fn.c_str(), /*gmsstrm::fmOpenRead*/ 0, errNr ) );

      REQUIRE( pgx.gdxSymbolInfoX( 0, recCnt, userInfo, explText ) );
      REQUIRE( pgx.gdxFindSymbol( "*", symNr ) );
      REQUIRE_EQ( 8, recCnt );

      REQUIRE( pgx.gdxDataReadStrStart( 0, recCnt ) );
      int dimFirst;
      REQUIRE_EQ( 8, recCnt );
      for( int i = 1; i <= recCnt; i++ )
      {
         REQUIRE( pgx.gdxDataReadStr( keys.ptrs(), vals.data(), dimFirst ) );
         REQUIRE_EQ( "uel_" + std::to_string( i ), keys.front().str() );
      }
      REQUIRE( pgx.gdxDataReadDone() );

      pgx.gdxClose();

      std::filesystem::remove( fn );
   } );
}

TEST_CASE( "Test domain checking for subset" )
{
   const std::string fn { "xyz.gdx" };
   basicTest( [&]( TGXFileObj &pgx ) {
      int errnr;
      REQUIRE( pgx.gdxOpenWrite( fn.c_str(), "gdxinterfacetests", errnr ) );
      REQUIRE( pgx.gdxDataWriteStrStart( "i", "expl", 1, dt_set, 0 ) );
      std::string key;
      TgdxValues vals {};
      for( int i {}; i < 6; i++ )
      {
         key = "i"s + std::to_string( i + 1 );
         const char *keyptrs[] = { key.c_str() };
         REQUIRE( pgx.gdxDataWriteStr( keyptrs, vals.data() ) );
      }
      REQUIRE( pgx.gdxDataWriteDone() );

      REQUIRE( pgx.gdxDataWriteStrStart( "j", "subset of i", 1, dt_set, 0 ) );
      key = "i"s;
      const char *keyptrs[] = { key.c_str() };
      pgx.gdxSymbolSetDomain( keyptrs );
      //REQUIRE(pgx.gdxSymbolSetDomain(keys));
      std::array<int, 2> subset = { 2, 4 };
      for( int i: subset )
      {
         key = "i"s + std::to_string( i );
         keyptrs[0] = key.c_str();
         REQUIRE( pgx.gdxDataWriteStr( keyptrs, vals.data() ) );
      }

      // adding an uel not from superset should fail
      key = "not_in_i"s;
      keyptrs[0] = key.c_str();
      REQUIRE( pgx.gdxDataWriteStr( keyptrs, vals.data() ) );

      REQUIRE( pgx.gdxDataWriteDone() );

      REQUIRE_EQ( 1, pgx.gdxErrorCount() );
      char msg[GMS_SSSIZE];
      REQUIRE( pgx.gdxErrorStr( pgx.gdxGetLastError(), msg ) );
      REQUIRE_EQ( "Domain violation"s, msg );

      // check if error uels was correctly set (more specific domain violation details)
      REQUIRE_EQ( 1, pgx.gdxDataErrorCount() );
      std::array<int, 20> errRecKeys {};
      std::array<double, 5> errRecVals {};
      REQUIRE( pgx.gdxDataErrorRecord( 1, errRecKeys.data(), errRecVals.data() ) );
      char uelNotInSuperset[GMS_SSSIZE];
      int uelMap;
      REQUIRE( pgx.gdxUMUelGet( errRecKeys.front(), uelNotInSuperset, uelMap ) );
      REQUIRE_EQ( "not_in_i"s, uelNotInSuperset );
      pgx.gdxClose();
   } );
   std::filesystem::remove( fn );
}

TEST_CASE( "Test writing a duplicate uel in string mode" )
{
   basicTest( []( TGXFileObj &pgx ) {
      int errNr;
      auto fn = "dup.gdx"s;
      REQUIRE( pgx.gdxOpenWrite( fn.c_str(), "gdxinterfacetests", errNr ) );
      REQUIRE( pgx.gdxDataWriteStrStart( "i", "A set", 1, dt_set, 0 ) );
      TgdxStrIndex keys {};
      TgdxValues vals {};
      for( int i = 1; i <= 8; i++ )
      {
         keys[0] = "uel_" + std::to_string( i );
         const char *keyptrs[] = { keys[0].c_str() };
         REQUIRE( pgx.gdxDataWriteStr( keyptrs, vals.data() ) );
      }
      const char *keyptrs[] = { keys[0].c_str() };
      REQUIRE( pgx.gdxDataWriteStr( keyptrs, vals.data() ) );
      REQUIRE( pgx.gdxDataWriteDone() );
      REQUIRE_EQ( 1, pgx.gdxErrorCount() );
      char msg[GMS_SSSIZE];
      TGXFileObj::gdxErrorStr( pgx.gdxGetLastError(), msg );
      REQUIRE( !strcmp( "Duplicate keys", msg ) );
      pgx.gdxClose();
      std::filesystem::remove( fn );
   } );
}

TEST_CASE( "Test acronym facilities" )
{
   const auto fn { "acronyms.gdx"s };
   char acroName[GMS_SSSIZE], acroText[GMS_SSSIZE];
   int acroIndex;
   TgdxValues vals {};
   StrIndexBuffers sib;

   testWrite( fn, [&]( TGXFileObj &pgx ) {
      REQUIRE_EQ( 0, pgx.gdxAcronymCount() );

      // first acronym (ix=23): acronym myacr 'my acronym';
      REQUIRE( pgx.gdxAcronymAdd( "myacr", "my acronym", 23 ) );
      REQUIRE( pgx.gdxAcronymGetInfo( 1, acroName, acroText, acroIndex ) );
      REQUIRE_EQ( "myacr"s, acroName );
      REQUIRE_EQ( "my acronym"s, acroText );
      REQUIRE_EQ( 23, acroIndex );
      REQUIRE_EQ( 1, pgx.gdxAcronymCount() );

      // trying to create a symbol with same name as an acronym that already exists should fail!
      REQUIRE_FALSE( pgx.gdxDataWriteStrStart( "myacr", "sym with same name", 1, dt_set, 0 ) );

      // modify first acronym: change item name and explanatory text
      REQUIRE( pgx.gdxAcronymSetInfo( 1, "myacr_mod", "my acronym_mod", 23 ) );
      REQUIRE( pgx.gdxAcronymGetInfo( 1, acroName, acroText, acroIndex ) );
      REQUIRE_EQ( "myacr_mod"s, acroName );
      REQUIRE_EQ( "my acronym_mod"s, acroText );
      REQUIRE_EQ( 23, acroIndex );

      REQUIRE_FALSE( pgx.gdxAcronymGetInfo( 999, acroName, acroText, acroIndex ) );
      REQUIRE_EQ( acroName[0], '\0' );
      REQUIRE_EQ( acroText[0], '\0' );
      REQUIRE_EQ( 0, acroIndex );

      // second acornym (ix=2): acronym anotherOne 'my second acronym';
      REQUIRE( pgx.gdxAcronymAdd( "anotherOne", "my second acronym", 2 ) );
      REQUIRE_EQ( 2, pgx.gdxAcronymCount() );

      // get mapping for first acronym (ix=23)
      int orgIx, newIx, autoIx;
      REQUIRE( pgx.gdxAcronymGetMapping( 1, orgIx, newIx, autoIx ) );
      REQUIRE_EQ( 0, autoIx );
      REQUIRE_EQ( 23, newIx );
      REQUIRE_EQ( 23, orgIx );

      // Acronym index is stored in double value field as multiple, e.g. index n>=1 is n * 10^300
      REQUIRE_EQ( 0, pgx.gdxAcronymValue( 0 ) );
      REQUIRE_EQ( GMS_SV_ACR, pgx.gdxAcronymValue( 1 ) );
      REQUIRE_EQ( GMS_SV_ACR * 23, pgx.gdxAcronymValue( 23 ) );

      REQUIRE_EQ( 0, pgx.gdxAcronymIndex( 0 ) );
      REQUIRE_EQ( 1, pgx.gdxAcronymIndex( pgx.gdxAcronymValue( 1 ) ) );
      REQUIRE_EQ( 2, pgx.gdxAcronymIndex( pgx.gdxAcronymValue( 2 ) ) );

      // Acronym index must be >= 1
      REQUIRE_FALSE( pgx.gdxAcronymName( 0, acroName ) );
      REQUIRE_EQ( ""s, acroName );

      // Acronym with index=1 does not exist
      REQUIRE( pgx.gdxAcronymName( pgx.gdxAcronymValue( 1 ), acroName ) );
      REQUIRE_EQ( "UnknownAcronym1"s, acroName );

      // Get name of first acronym (ix=23)
      REQUIRE( pgx.gdxAcronymName( pgx.gdxAcronymValue( 23 ), acroName ) );
      REQUIRE_EQ( "myacr_mod"s, acroName );

      // Previous operations should not add any new acronym
      REQUIRE_EQ( 2, pgx.gdxAcronymCount() );

      // Create new 1-dim parameter symbol with second acronym "anotherOne" as first record
      // Add acronym mapped to 23 as second record
      // Add unknown acronym as value of third record
      REQUIRE( pgx.gdxDataWriteStrStart( "p", "magic parameter", 1, dt_par, 0 ) );
      const std::array<std::pair<std::string, int>, 4> entries { { { "i1"s, 2 },
                                                                   { "i2"s, 23 },
                                                                   { "i3"s, 8 },
                                                                   { "i4"s, 42 } } };
      for( const auto &[keyStr, acroVal]: entries )
      {
         sib.front() = keyStr;
         vals[GMS_VAL_LEVEL] = pgx.gdxAcronymValue( acroVal );
         REQUIRE( pgx.gdxDataWriteStr( sib.cptrs(), vals.data() ) );
      }
      REQUIRE( pgx.gdxDataWriteDone() );

      // Added UnknownACRO8 as third acronym with index=8
      // Added UnknownACRO42 as third acronym with index=42
      REQUIRE_EQ( 4, pgx.gdxAcronymCount() );

      // Can rename now but not change index
      REQUIRE( pgx.gdxAcronymSetInfo( 4, "Acro43", "", 42 ) );
   } );

   auto readMagicParameter = [&]( TGXFileObj &pgx, int acroNextNr ) {
      int nrRecs, key, dimFrst;
      REQUIRE( pgx.gdxDataReadRawStart( 1, nrRecs ) );
      REQUIRE_EQ( 4, nrRecs );
      const std::array<std::tuple<int, std::string, int>, 4> expectedKeyAcros { { { 1, "anotherOne"s, 2 },
                                                                                  { 2, "myacr_mod"s, 23 },
                                                                                  { 3, "UnknownACRO8", 8 },
                                                                                  { 4, "Acro43", 42 } } };
      for( const auto &triple: expectedKeyAcros )
      {
         int expKey = std::get<0>( triple );
         std::string expAcroName = std::get<1>( triple );
         int expAcroValue = std::get<2>( triple );
         if( acroNextNr )
         {
            expAcroValue = acroNextNr++;
            expAcroName = "UnknownAcronym"s + std::to_string( expAcroValue );
         }
         REQUIRE( pgx.gdxDataReadRaw( &key, vals.data(), dimFrst ) );
         REQUIRE_EQ( expKey, key );
         REQUIRE_EQ( expAcroValue, pgx.gdxAcronymIndex( vals[GMS_VAL_LEVEL] ) );
         REQUIRE( pgx.gdxAcronymName( pgx.gdxAcronymValue( expAcroValue ), acroName ) );
         REQUIRE_EQ( expAcroName, acroName );
      }
      REQUIRE( pgx.gdxDataReadDone() );
   };

   testRead( fn, [&]( TGXFileObj &pgx ) {
      // set and reset acronym next nr
      int oldNextAutoAcronym = pgx.gdxAcronymNextNr( 23 );
      REQUIRE_EQ( 0, oldNextAutoAcronym );
      REQUIRE_EQ( 23, pgx.gdxAcronymNextNr( 0 ) );
      REQUIRE_EQ( 4, pgx.gdxAcronymCount() );
      auto checkAcronym = [&]( int index, const std::string &expName, const std::string &expText, int expIndex ) {
         REQUIRE( pgx.gdxAcronymGetInfo( index, acroName, acroText, acroIndex ) );
         REQUIRE_EQ( expName, acroName );
         REQUIRE_EQ( expText, acroText );
         REQUIRE_EQ( expIndex, acroIndex );
      };
      checkAcronym( 1, "myacr_mod"s, "my acronym_mod"s, 23 );
      checkAcronym( 2, "anotherOne"s, "my second acronym"s, 2 );
      checkAcronym( 3, "UnknownACRO8"s, ""s, 8 );
      checkAcronym( 4, "Acro43"s, ""s, 42 );

      // Name not matching!
      REQUIRE_FALSE( pgx.gdxAcronymSetInfo( 2, "YesAnotherOne", "new text", 22 ) );
      // Index already taken
      //REQUIRE_FALSE(pgx.gdxAcronymSetInfo(2, "anotherOne", "new text", 23));
      // ok
      REQUIRE( pgx.gdxAcronymSetInfo( 2, "anotherOne", "new text", 43 ) );
      checkAcronym( 2, "anotherOne"s, "my second acronym"s, 2 );
   } );

   testRead( fn, [&]( TGXFileObj &pgx ) {
      REQUIRE_EQ( 0, pgx.gdxAcronymNextNr( 0 ) );
      readMagicParameter( pgx, 0 );
   } );

   testRead( fn, [&]( TGXFileObj &pgx ) {
      REQUIRE_EQ( 0, pgx.gdxAcronymNextNr( 82 ) );
      readMagicParameter( pgx, 82 );
   } );

   std::filesystem::remove( fn );
}

TEST_CASE( "Test comments addition and querying" )
{
   auto fn { "comments.gdx"s };
   char commentStrGot[GMS_SSSIZE];
   const auto commentStrExp { "A fancy comment!"s };
   testWrite( fn, [&]( TGXFileObj &pgx ) {
      REQUIRE( pgx.gdxDataWriteStrStart( "i", "expl text", 1, dt_set, 0 ) );
      REQUIRE( pgx.gdxDataWriteDone() );
      REQUIRE( pgx.gdxSymbolAddComment( 1, commentStrExp.c_str() ) );
      REQUIRE( pgx.gdxSymbolGetComment( 1, 1, commentStrGot ) );
      REQUIRE_EQ( commentStrExp, commentStrGot );
      REQUIRE_FALSE( pgx.gdxSymbolAddComment( -5, "should not work" ) );
      REQUIRE_FALSE( pgx.gdxSymbolAddComment( std::numeric_limits<int>::max(), "should not work" ) );
   } );
   testRead( fn, [&]( TGXFileObj &pgx ) {
      REQUIRE( pgx.gdxSymbolGetComment( 1, 1, commentStrGot ) );
      REQUIRE_EQ( commentStrExp, commentStrGot );
   } );
   std::filesystem::remove( fn );
}

TEST_CASE( "Test writing and reading set element texts more exhaustively" )
{
   std::string fn { "setelemtxt_wr.gdx" };
   // no set text
   testWrite( fn, [&]( TGXFileObj &pgx ) {
      REQUIRE( pgx.gdxUELRegisterRawStart() );
      REQUIRE( pgx.gdxUELRegisterRaw( "seattle" ) );
      REQUIRE( pgx.gdxUELRegisterDone() );

      pgx.gdxDataWriteRawStart( "i", "cities", 1, dt_set, 0 );
      TgdxValues vals {};
      vals[GMS_VAL_LEVEL] = 3.141;
      int key { 1 };
      pgx.gdxDataWriteRaw( &key, vals.data() );
      pgx.gdxDataWriteDone();
   } );
   testRead( fn, [&]( TGXFileObj &pgx ) {
      char txt[GMS_SSSIZE];
      int node;
      REQUIRE( pgx.gdxGetElemText( 0, txt, node ) );
      int hi { std::numeric_limits<int>::max() };
      REQUIRE_FALSE( pgx.gdxGetElemText( hi, txt, node ) );
      REQUIRE_FALSE( pgx.gdxGetElemText( 1, txt, node ) );
   } );
   std::filesystem::remove( fn );
}

TEST_CASE( "Debug issue with SymbolSetDomain and write raw domain check uncovered by emp_oa_gams_jams test" )
{
   std::string fn { "domaincheck.gdx" };
   testWrite( fn, [&]( TGXFileObj &pgx ) {
      REQUIRE( pgx.gdxStoreDomainSets() );
      pgx.gdxStoreDomainSetsSet( false );
      REQUIRE_FALSE( pgx.gdxStoreDomainSets() );
      pgx.gdxUELRegisterRawStart();
      for( int i = 0; i < 50; i++ )
         pgx.gdxUELRegisterRaw( ( "uel_"s + std::to_string( i + 1 ) ).c_str() );
      pgx.gdxUELRegisterDone();

      pgx.gdxDataWriteRawStart( "j", "", 1, dt_set, 0 );
      TgdxValues vals {};
      vals[GMS_VAL_LEVEL] = 3.141;
      int key;
      for( key = 17; key <= 32; key++ )
         pgx.gdxDataWriteRaw( &key, vals.data() );
      REQUIRE( pgx.gdxDataWriteDone() );
      REQUIRE( pgx.gdxDataWriteRawStart( "jb", "", 1, dt_set, 0 ) );
      TgdxStrIndex domainNames {};
      domainNames.front() = "j";
      StrIndexBuffers domainIds { &domainNames };
      REQUIRE( pgx.gdxSymbolSetDomain( domainIds.cptrs() ) );
      key = 17;
      REQUIRE( pgx.gdxDataWriteRaw( &key, vals.data() ) );
      REQUIRE( pgx.gdxDataWriteDone() );
   } );
   std::filesystem::remove( fn );
}

TEST_CASE( "Test writing and reading some special value scalars" )
{
   std::string fn { "sv_scalars.gdx" };
   testWrite( fn, [&]( TGXFileObj &pgx ) {
      TgdxValues vals {};
      vals[GMS_VAL_LEVEL] = 1e+300;
      REQUIRE( pgx.gdxDataWriteRawStart( "undef", "", 0, dt_par, 0 ) );
      REQUIRE( pgx.gdxDataWriteRaw( nullptr, vals.data() ) );
      REQUIRE( pgx.gdxDataWriteDone() );
   } );
   testRead( fn, [&]( TGXFileObj &pgx ) {
      int recCnt;
      REQUIRE( pgx.gdxDataReadRawStart( 1, recCnt ) );
      int dimFirst;
      TgdxValues vals {};
      pgx.gdxDataReadRaw( nullptr, vals.data(), dimFirst );
      REQUIRE( pgx.gdxDataReadDone() );
      double undef = vals[GMS_VAL_LEVEL];
      REQUIRE_LT( 1e+300 - undef, 0.1 );
   } );

   std::filesystem::remove( fn );
}

TEST_CASE( "Test open append to rename a single uel" )
{
   const auto fn { "append_rename.gdx"s }, prod { "gdxtest"s };
   int errNr;
   basicTest( [&]( TGXFileObj &pgx ) {
      pgx.gdxOpenWrite( fn.c_str(), prod.c_str(), errNr );
      pgx.gdxUELRegisterRawStart();
      pgx.gdxUELRegisterRaw( "a" );
      pgx.gdxUELRegisterDone();
      pgx.gdxClose();
      pgx.gdxOpenAppend( fn.c_str(), prod.c_str(), errNr );
      pgx.gdxRenameUEL( "a", "b" );
      pgx.gdxClose();
   } );
   basicTest( [&]( TGXFileObj &pgx ) {
      pgx.gdxOpenRead( fn.c_str(), errNr );
      int uelMap;
      char uelStr[GMS_SSSIZE];
      pgx.gdxUMUelGet( 1, uelStr, uelMap );
      REQUIRE_EQ( "b"s, uelStr );
      pgx.gdxClose();
   } );
   std::filesystem::remove( fn );
}

void testWithCompressConvert( bool compress, const std::string &convert )
{
   if( !hasGAMSinstalled() )
   {
      std::cout << "Skipping conversion test due to missing GAMS system" << std::endl;
      return;
   }
   const std::string fn { "conv_compr.gdx" };
   setEnvironmentVar( "GDXCOMPRESS", compress ? "1"s : "0"s );
   setEnvironmentVar( "GDXCONVERT", convert );
   testWrite( fn, [&]( TGXFileObj &pgx ) {
      TgdxValues vals {};
      REQUIRE( pgx.gdxDataWriteRawStart( "undef", "", 0, dt_par, 0 ) );
      REQUIRE( pgx.gdxDataWriteRaw( nullptr, vals.data() ) );
      REQUIRE( pgx.gdxDataWriteDone() );
   } );
   std::filesystem::remove( fn );
   unsetEnvironmentVar( "GDXCOMPRESS" );
   unsetEnvironmentVar( "GDXCONVERT" );
}

TEST_CASE( "Test convert and compress" )
{
   if( !hasGAMSinstalled() )
   {
      std::cout << "Skipping conversion test due to missing GAMS system" << std::endl;
      return;
   }
   testWithCompressConvert( false, "" );
#ifndef __APPLE__
   testWithCompressConvert( false, "v5" );
   testWithCompressConvert( true, "v5" );
   testWithCompressConvert( false, "v6" );
   testWithCompressConvert( true, "v6" );
#endif
   testWithCompressConvert( false, "v7" );
   testWithCompressConvert( true, "v7" );
}

TEST_CASE( "Test simple write/read with compression activated" )
{
   const auto fn { "rw_compression.gdx"s };
   const int cardinality { 1024 };
   setEnvironmentVar( "GDXCOMPRESS", "1"s );
   TgdxValues vals {};
   StrIndexBuffers sib;
   testWrite( fn, [&]( TGXFileObj &pgx ) {
      REQUIRE( pgx.gdxDataWriteStrStart( "i", "set", 1, dt_set, 0 ) );
      for( int i {}; i < cardinality; i++ )
      {
         sib.front() = "i"s + std::to_string( i + 1 );
         REQUIRE( pgx.gdxDataWriteStr( sib.cptrs(), vals.data() ) );
      }
      REQUIRE( pgx.gdxDataWriteDone() );
   } );
   testRead( fn, [&]( TGXFileObj &pgx ) {
      int numRecs;
      REQUIRE( pgx.gdxDataReadStrStart( 1, numRecs ) );
      REQUIRE_EQ( cardinality, numRecs );
      int dimFrst;
      for( int i {}; i < cardinality; i++ )
      {
         REQUIRE( pgx.gdxDataReadStr( sib.ptrs(), vals.data(), dimFrst ) );
         REQUIRE_EQ( "i"s + std::to_string( i + 1 ), sib.front().str() );
      }
      REQUIRE( pgx.gdxDataReadDone() );
   } );
   unsetEnvironmentVar( "GDXCOMPRESS" );
   std::filesystem::remove( fn );
}

TEST_CASE( "Test symbol index max UEL length" )
{
   testReadModelGDX( "trnsport"s, [&]( TGXFileObj &pgx ) {
      std::array<int, GLOBAL_MAX_INDEX_DIM> lengthInfo {};
      int maxUelLen = pgx.gdxSymbIndxMaxLength( 7, lengthInfo.data() );// c
      REQUIRE_EQ( 9, maxUelLen );                                      // len(san-diego)=9
      REQUIRE_EQ( 9, lengthInfo[0] );                                  // san-diego
      REQUIRE_EQ( 8, lengthInfo[1] );                                  // new-york
   } );
}

TEST_CASE( "Test UEL table get max uel length" )
{
   testReadModelGDX( "trnsport"s, [&]( TGXFileObj &pgx ) {
      REQUIRE_EQ( 9, pgx.gdxUELMaxLength() );
   } );
}

TEST_CASE( "Test symbol info out of range" )
{
   testReadModelGDX( "trnsport"s, [&]( TGXFileObj &pgx ) {
      char symId[GMS_SSSIZE];
      int dim, typ;
      REQUIRE_FALSE( pgx.gdxSymbolInfo( 99, symId, dim, typ ) );
      REQUIRE_EQ( symId[0], '\0' );
      REQUIRE_EQ( -1, dim );
      REQUIRE_EQ( dt_set, typ );
   } );
}

TEST_CASE( "Test filter" )
{
   testReadModelGDX( "trnsport"s, []( TGXFileObj &pgx ) {
      // uels: seattle 1, san-diego 2, new-york 3, chicago 4, topeka 5
      std::array<int, 2> filterAction { gdx::DOMC_EXPAND, gdx::DOMC_EXPAND }, keys {};
      TgdxValues values {};

      int nrRecs, dimFirst;
      REQUIRE( pgx.gdxDataReadFilteredStart( 5, filterAction.data(), nrRecs ) );// symbol 'd'
      REQUIRE_EQ( 6, nrRecs );
      REQUIRE( pgx.gdxDataReadMap( 1, keys.data(), values.data(), dimFirst ) );
      REQUIRE_EQ( 1, keys.front() );// seattle
      REQUIRE_EQ( 2, keys.back() ); // san-diego
      REQUIRE( pgx.gdxDataReadDone() );

      for( int fa { -2 }; fa <= 0; fa++ )
      {
         filterAction = { fa, fa };
         REQUIRE( pgx.gdxDataReadFilteredStart( 5, filterAction.data(), nrRecs ) );// symbol 'd'
         REQUIRE( pgx.gdxDataReadMap( 1, keys.data(), values.data(), dimFirst ) );
         REQUIRE( pgx.gdxDataReadDone() );
      }

      // filter with just (1,2) = (seattle,new-york)
      REQUIRE_FALSE( pgx.gdxFilterExists( 1 ) );
      REQUIRE( pgx.gdxFilterRegisterStart( 1 ) );
      REQUIRE( pgx.gdxFilterRegister( 1 ) );// seattle
      REQUIRE( pgx.gdxFilterRegister( 3 ) );// new-york
      REQUIRE( pgx.gdxFilterRegisterDone() );
      REQUIRE( pgx.gdxFilterExists( 1 ) );

      // empty filter
      REQUIRE_FALSE( pgx.gdxFilterExists( 2 ) );
      REQUIRE( pgx.gdxFilterRegisterStart( 2 ) );
      REQUIRE( pgx.gdxFilterRegisterDone() );
      REQUIRE( pgx.gdxFilterExists( 2 ) );

      // override empty filter with itself
      REQUIRE( pgx.gdxFilterRegisterStart( 2 ) );
      REQUIRE( pgx.gdxFilterRegisterDone() );
      REQUIRE( pgx.gdxFilterExists( 2 ) );

      // filter nr. 1
      filterAction[0] = filterAction[1] = 1;
      REQUIRE( pgx.gdxDataReadFilteredStart( 5, filterAction.data(), nrRecs ) );// symbol 'd'
      REQUIRE_EQ( 6, nrRecs );
      REQUIRE( pgx.gdxDataReadMap( 1, keys.data(), values.data(), dimFirst ) );
      REQUIRE_EQ( 1, keys.front() );// seattle
      REQUIRE_EQ( 3, keys.back() ); // new-york
      REQUIRE( pgx.gdxDataReadDone() );

      // filter nr. 2 (empty)
      filterAction[0] = filterAction[1] = 2;
      REQUIRE( pgx.gdxDataReadFilteredStart( 5, filterAction.data(), nrRecs ) );// symbol 'd'
      REQUIRE( pgx.gdxDataReadDone() );

      // invalid filter nr
      filterAction[0] = filterAction[1] = 23;
      REQUIRE_FALSE( pgx.gdxDataReadFilteredStart( 5, filterAction.data(), nrRecs ) );// symbol 'd'
   } );
}

TEST_CASE( "Test setting trace level" )
{
   std::string fn { "trace.gdx"s };
   testWrite( fn, [&]( TGXFileObj &pgx ) {
      REQUIRE( pgx.gdxSetTraceLevel( (int) gdx::TGXFileObj::TraceLevels::trl_all, "tracestr" ) );
      REQUIRE( pgx.gdxUELRegisterRawStart() );
      REQUIRE( pgx.gdxUELRegisterRaw( "TheOnlyUEL" ) );
      REQUIRE( pgx.gdxUELRegisterDone() );
      REQUIRE( pgx.gdxDataWriteRawStart( "mysym", "This is my symbol!", 1, dt_par, 0 ) );
      int key { 1 };
      TgdxValues values {};
      values[GMS_VAL_LEVEL] = 3.141;
      REQUIRE( pgx.gdxDataWriteRaw( &key, values.data() ) );
      REQUIRE( pgx.gdxDataWriteDone() );
      REQUIRE( pgx.gdxDataWriteRawStart( "myscalar", "This is a scalar!", 0, dt_par, 0 ) );
      REQUIRE( pgx.gdxDataWriteDone() );
   } );
   std::filesystem::remove( fn );
}

TEST_CASE( "Test classifying a map value as potential specval" )
{
   basicTest( []( TGXFileObj &pgx ) {
      int sv;
      REQUIRE_FALSE( pgx.gdxMapValue( 23, sv ) );
      REQUIRE_EQ( sv_normal, sv );
      REQUIRE( pgx.gdxMapValue( GMS_SV_UNDEF, sv ) );
      REQUIRE_EQ( sv_valund, sv );
      REQUIRE( pgx.gdxMapValue( GMS_SV_EPS, sv ) );
      REQUIRE_EQ( sv_valeps, sv );
   } );
}

TEST_CASE( "Test setting/getting auto convert flag" )
{
   if( !hasGAMSinstalled() )
   {
      std::cout << "Skipping conversion test due to missing GAMS system" << std::endl;
      return;
   }
   basicTest( []( TGXFileObj &pgx ) {
      // default is AutoConvert=1/true
      REQUIRE( pgx.gdxAutoConvert( 0 ) );
      REQUIRE_FALSE( pgx.gdxAutoConvert( 1 ) );
      REQUIRE( pgx.gdxAutoConvert( 1 ) );
   } );
   basicTest( []( TGXFileObj &pgx ) {
      setEnvironmentVar( "GDXCOMPRESS", "1"s );
      setEnvironmentVar( "GDXCONVERT", "v5" );
      int ErrNr;
      const std::string fn { "autoconv.gdx"s };
      REQUIRE( pgx.gdxAutoConvert( 0 ) );// disable auto convert
      REQUIRE( pgx.gdxOpenWriteEx( fn.c_str(), "gdxtest", 0, ErrNr ) );
      pgx.gdxClose();
      REQUIRE( pgx.gdxOpenRead( fn.c_str(), ErrNr ) );
      int fileVer, comprLevel;
      REQUIRE( pgx.gdxFileInfo( fileVer, comprLevel ) );
      REQUIRE_EQ( 7, fileVer );
      REQUIRE_EQ( 0, comprLevel );
      pgx.gdxClose();
      unsetEnvironmentVar( "GDXCOMPRESS" );
      unsetEnvironmentVar( "GDXCONVERT" );
      std::filesystem::remove( fn );
   } );
}

TEST_CASE( "Test reading methods with slices" )
{
   std::string fn { "slice.gdx"s }, f2 { "slice_port.gdx"s };
   testWrite( fn, [&]( TGXFileObj &pgx ) {
      StrIndexBuffers keys;
      TgdxValues values {};
      REQUIRE( pgx.gdxDataWriteStrStart( "i", "three element set", 1, dt_set, 0 ) );
      for( int i {}; i < 3; i++ )
      {
         keys.front() = "i"s + std::to_string( i + 1 );
         REQUIRE( pgx.gdxDataWriteStr( keys.cptrs(), values.data() ) );
      }
      REQUIRE( pgx.gdxDataWriteDone() );
      REQUIRE( pgx.gdxDataWriteStrStart( "p", "nine element 2D parameter", 2, dt_par, 0 ) );
      int ctr {1};
      for( int i {}; i < 3; i++ )
      {
         for( int j {}; j < 3; j++ )
         {
            keys[0] = "a"s + std::to_string( i + 1 );
            keys[1] = "b"s + std::to_string( j + 1 );
            values[GMS_VAL_LEVEL] = ctr++;
            REQUIRE( pgx.gdxDataWriteStr( keys.cptrs(), values.data() ) );
         }
      }
      REQUIRE( pgx.gdxDataWriteDone() );
   } );
   testRead( fn, [&]( TGXFileObj &pgx ) {
      TgdxUELIndex elemCountsPerDim {}, expectedCounts {}, keyIndices {};
      StrIndexBuffers uelNames, uelFilterStrs;
      int dim;

      // Read 3-element set "i" fully (all filters are empty str)
      expectedCounts.front() = 3;
      REQUIRE( pgx.gdxDataReadSliceStart( 1, elemCountsPerDim.data() ) );
      REQUIRE_EQ( expectedCounts, elemCountsPerDim );
      uelFilterStrs.front() = ""s;
      
      {
         auto recordCallback = []( const int *keyIndices, const double *vals ) {
            static int expKey {};
            REQUIRE_EQ( expKey, keyIndices[0] );
            expKey++;
            expKey %= 3;
         };
         REQUIRE( pgx.gdxDataReadSlice( uelFilterStrs.cptrs(), dim, recordCallback ) );
      }

      keyIndices.front() = 1;
      REQUIRE( pgx.gdxDataSliceUELS( keyIndices.data(), uelNames.ptrs() ) );
      REQUIRE_EQ( "i2"s, uelNames.front().str() );
      REQUIRE( pgx.gdxDataReadDone() );

      // For cartesian products {a1,a2,a3} x {b1,b2,b3} 2D parameter "p" only read tuples with (a2,*)
      expectedCounts[0] = expectedCounts[1] = 3;
      REQUIRE( pgx.gdxDataReadSliceStart( 2, elemCountsPerDim.data() ) );
      REQUIRE_EQ( expectedCounts, elemCountsPerDim );

      uelFilterStrs.front() = "XYZdoesNotExist"s;
      REQUIRE_FALSE( pgx.gdxDataReadSlice( uelFilterStrs.cptrs(), dim, nullptr ) );
      REQUIRE( pgx.gdxDataReadDone() );
      REQUIRE( pgx.gdxDataReadSliceStart( 2, elemCountsPerDim.data() ) );

      uelFilterStrs.front() = "a2"s;

      {
         auto recordCallback = []( const int *keyIndices, const double *vals ) {
            static int expKey {};
            REQUIRE_EQ( expKey, keyIndices[0] );
            REQUIRE_EQ( 4+expKey, vals[GMS_VAL_LEVEL] );
            expKey++;
         };
         REQUIRE( pgx.gdxDataReadSlice( uelFilterStrs.cptrs(), dim, recordCallback ) );
      }

      // Third record contained in slice (see above) has keys {a2,b3}
      keyIndices.front() = 2;
      REQUIRE( pgx.gdxDataSliceUELS( keyIndices.data(), uelNames.ptrs() ) );
      REQUIRE_EQ( "a2"s, uelNames[0].str() );
      REQUIRE_EQ( "b3"s, uelNames[1].str() );

      // Test invalid record index
      keyIndices.front() = 8;
      REQUIRE( pgx.gdxDataSliceUELS( keyIndices.data(), uelNames.ptrs() ) );
      REQUIRE_EQ( "a2"s, uelNames[0].str() );
      REQUIRE_EQ( "?"s, uelNames[1].str() );

      REQUIRE( pgx.gdxDataReadDone() );
   } );
   std::filesystem::remove( fn );
}

TEST_CASE( "Test domain violations more extensively" )
{
   std::string fn { "domviol.gdx"s };
   testWrite( fn, [&]( TGXFileObj &pgx ) {
      REQUIRE( pgx.gdxDataWriteStrStart( "i", "a set", 1, dt_set, 0 ) );
      std::string elem { "i"s };
      const char *key { elem.c_str() };
      std::array<double, GMS_VAL_MAX> vals {};
      REQUIRE( pgx.gdxDataWriteStr( &key, vals.data() ) );
      REQUIRE( pgx.gdxDataWriteDone() );

      const int paramDim { 3 };
      REQUIRE( pgx.gdxDataWriteStrStart( "p", "parameter", paramDim, dt_par, 0 ) );
      std::array<const char *, paramDim> domainIds {}, keys {};
      std::fill_n( domainIds.begin(), paramDim, key );
      int rc { pgx.gdxSymbolSetDomain( domainIds.data() ) };
      REQUIRE( rc );
      std::string nf { "notfound"s };
      std::fill_n( keys.begin(), paramDim, nf.c_str() );
      rc = pgx.gdxDataWriteStr( keys.data(), vals.data() );
      REQUIRE( rc );
      REQUIRE( pgx.gdxDataWriteDone() );
      REQUIRE_EQ( 1, pgx.gdxDataErrorCount() );

      std::array<int, GMS_MAX_INDEX_DIM> errorKeys {};
      REQUIRE( pgx.gdxDataErrorRecordX( 1, errorKeys.data(), vals.data() ) );
      for( int i {}; i < paramDim; i++ )
         REQUIRE_EQ( -2, errorKeys[i] );
   } );
   std::filesystem::remove( fn );
}


TEST_CASE( "Test fallback behavior when trying GDX operations when no file is open" )
{
   std::string msg;
   TGXFileObj pgx { msg };

   char text[GMS_SSSIZE];
   int nodeNr;
   REQUIRE_FALSE( pgx.gdxGetElemText( 1, text, nodeNr ) );
   REQUIRE_EQ( 0, nodeNr );
   REQUIRE_EQ( ""s, text );

   int uelCnt, highMap;
   REQUIRE_FALSE( pgx.gdxUMUelInfo( uelCnt, highMap ) );
   REQUIRE_EQ( 0, uelCnt );
   REQUIRE_EQ( 0, highMap );

   char uel[GMS_SSSIZE];
   REQUIRE_FALSE( pgx.gdxGetUEL( 1, uel ) );
   REQUIRE_EQ( ""s, uel );

   int uelNr, uelMap;
   REQUIRE_FALSE( pgx.gdxUMFindUEL( "i1", uelNr, uelMap ) );
   REQUIRE_EQ( -1, uelNr );
   REQUIRE_EQ( -1, uelMap );

   REQUIRE_EQ( -1, pgx.gdxRenameUEL( "i1", "i2" ) );

   int txtNr;
   REQUIRE_FALSE( pgx.gdxAddSetText( "test", txtNr ) );
   REQUIRE_EQ( 0, txtNr );

   REQUIRE_FALSE( pgx.gdxGetElemText( 3, text, nodeNr ) );
   REQUIRE_EQ( ""s, text );
   REQUIRE_EQ( 0, nodeNr );

   REQUIRE_FALSE( pgx.gdxSetTextNodeNr( 23, 42 ) );

   REQUIRE_FALSE( pgx.gdxSymbolGetComment( 23, 2, text ) );
   REQUIRE_EQ( ""s, text );
}

TEST_CASE( "Test reading reading GDX files in legacy versions (V5 and V6)" )
{
   if( !hasGAMSinstalled() )
   {
      std::cout << "Skipping legacy version GDX file read test due to missing GAMS system!" << std::endl;
      return;
   }
   // See: https://www.gams.com/latest/docs/T_GDXCOPY.html
#ifdef __APPLE__
   std::cout << "Skipping legacy version GDX file read test on macOS since gdxcopy does not support V5 and V6 there." << std::endl;
   return;
#endif
   const auto modelName { "trnsport"s };
   acquireGDXforModel( modelName );
   for( const auto &versSuff: { "V5"s, "V6U"s, "V6C"s } )
   {

      const auto outDir { "out"s + versSuff }, cmd { gamsToolCall( "gdxcopy"s ) + " -"s + versSuff + " "s + modelName + ".gdx "s + outDir };
      int rc { std::system( cmd.c_str() ) };
      if( !rc )
      {
         const auto convertedGDXfn { modelName + "_"s + versSuff + ".gdx"s };
         std::filesystem::rename( outDir + "/"s + modelName + ".gdx"s, convertedGDXfn );
         std::filesystem::remove( outDir );
         std::string ErrMsg;
         TGXFileObj pgx { ErrMsg };
         int ErrNr;
         REQUIRE( pgx.gdxOpenRead( convertedGDXfn.c_str(), ErrNr ) );
         int syCnt, uelCnt;
         REQUIRE( pgx.gdxSystemInfo( syCnt, uelCnt ) );
         REQUIRE_GT( syCnt, 0 );
         REQUIRE_GT( uelCnt, 0 );
         pgx.gdxClose();
         std::filesystem::remove( convertedGDXfn );
      }
   }
   std::filesystem::remove( "trnsport.gdx" );
}

TEST_CASE( "Re-create basic dc.gms (from idc01) test" )
{
   auto fn { "twosets.gdx"s };
   testWrite( fn, []( TGXFileObj &pgx ) {
      StrIndexBuffers sib, sib2;
      TgdxValues values {};

      REQUIRE( pgx.gdxDataWriteStrStart( "k", "", 1, dt_set, 0 ) );
      for( int i {}; i < 6; i++ )
      {
         sib.front() = "k"s + std::to_string( i + 3 );
         REQUIRE( pgx.gdxDataWriteStr( sib.cptrs(), values.data() ) );
      }
      REQUIRE( pgx.gdxDataWriteDone() );

      REQUIRE( pgx.gdxDataWriteStrStart( "kk", "", 1, dt_set, 0 ) );
      sib2.front() = "k"s;
      REQUIRE( pgx.gdxSymbolSetDomain( sib2.cptrs() ) );
      for( int i {}; i < 6; i++ )
      {
         sib.front() = "k"s + std::to_string( i + 3 );
         REQUIRE( pgx.gdxDataWriteStr( sib.cptrs(), values.data() ) );
      }
      REQUIRE( pgx.gdxDataWriteDone() );
   } );
   testRead( fn, []( TGXFileObj &pgx ) {
      int syNr;
      REQUIRE( pgx.gdxFindSymbol( "kk", syNr ) );
      REQUIRE_EQ( 2, syNr );
      REQUIRE( pgx.gdxUELRegisterMapStart() );
      for( int i { 1 }; i <= 3; i++ )
      {
         std::string s { "k" + std::to_string( i ) };
         REQUIRE( pgx.gdxUELRegisterMap( i, s.c_str() ) );
      }
      REQUIRE( pgx.gdxUELRegisterDone() );

      REQUIRE( pgx.gdxFilterRegisterStart( 138 ) );
      for( int i { 1 }; i <= 3; i++ )
         REQUIRE( pgx.gdxFilterRegister( i ) );
      REQUIRE( pgx.gdxFilterRegisterDone() );

      int numRecs;
      std::array<int, 20> filter {};
      filter.front() = 138;
      REQUIRE( pgx.gdxDataReadFilteredStart( 2, filter.data(), numRecs ) );
      REQUIRE_EQ( 1, numRecs );
      int dimFrst;
      TgdxValues values {};
      TgdxUELIndex keys {};
      int ctr {};
      while( pgx.gdxDataReadMap( 1, keys.data(), values.data(), dimFrst ) )
      {
         REQUIRE_EQ( 3, keys.front() );
         ctr++;
      }
      REQUIRE_EQ( 1, ctr );
      REQUIRE( pgx.gdxDataReadDone() );
   } );
   std::filesystem::remove( fn );
}

TEST_CASE( "Open append should report error for old GDX file versions" )
{
   if( !hasGAMSinstalled() )
   {
      std::cout << "Skipping test since GAMS system is not found!" << std::endl;
      return;
   }

#ifdef __APPLE__
   std::cout << "Skipping legacy version GDX file open append test on macOS since gdxcopy does not support V5 and V6 there." << std::endl;
   return;
#endif

   setEnvironmentVar( "GDXCONVERT", "v5" );
   testWrite( "f.gdx", []( TGXFileObj &obj ) {} );
   unsetEnvironmentVar( "GDXCONVERT" );

   {
      std::string errMsg;
      TGXFileObj pgx { errMsg };
      int errNr;
      REQUIRE( pgx.gdxOpenAppend( "f.gdx", "gdxtests", errNr ) );
      REQUIRE_GT( pgx.gdxErrorCount(), 0 );
      REQUIRE_EQ( -100060, pgx.gdxGetLastError() );
   }

   std::filesystem::remove( "f.gdx" );
}

}// namespace gdx::tests::gdxtests
