/**
 *
 * GAMS - General Algebraic Modeling System C++ API
 *
 * Copyright (c) 2017-2025 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2025 GAMS Development Corp. <support@gams.com>
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

/*
   Use this command to compile the example:
   cl xp_example2.cpp ../C/api/gdxcc.c ../C/api/optcc.c -I../C/api
 */

/*
  This program performs the following steps:
     1. Generate a gdx file with demand data
     2. Calls GAMS to solve a simple transportation model
        (The GAMS model writes the solution to a gdx file)
     3. The solution is read from the gdx file
 */

#include "gclgms.h"
#include "gdx.hpp"
#include "optcc.h"

#include <cstdlib>
#include <cstring>

#include <iostream>
#include <array>

using namespace gdx;
using namespace std::literals::string_literals;

class WriteSolveReadExample
{
   std::string ErrMsg;
   TGXFileObj pgdx { ErrMsg };
   optHandle_t opt {};

   std::array<char, GMS_SSSIZE> msg {};

   void throwGDXError( const TGXFileObj &gdx, int i, const std::string &s );

public:
   explicit WriteSolveReadExample( const std::string &sysdir );
   int WriteModelData( const std::string &gdxOutFileName );
   int CallGams( const std::string &sysdir, const std::string &model );
   int ReadSolutionData( const char *fngdxfile );
};

void WriteSolveReadExample::throwGDXError( const TGXFileObj &gdx, int i, const std::string &s )
{
   gdx.gdxErrorStr( i, msg.data() );
   throw std::runtime_error( s + "% failed: "s + msg.data() );
}

WriteSolveReadExample::WriteSolveReadExample( const std::string &sysdir )
{
   if( !ErrMsg.empty() )
      throw std::runtime_error( "Could not create gdx object "s + ErrMsg );
   if( !optCreateD( &opt, sysdir.c_str(), msg.data(), static_cast<int>( msg.size() ) ) )
      throw std::runtime_error( "Could not create opt object: "s + msg.data() );
}

int WriteSolveReadExample::WriteModelData( const std::string &gdxOutFileName )
{
   int status;
   pgdx.gdxOpenWrite( gdxOutFileName.c_str(), "xp_Example2", status );
   if( status )
   {
      throwGDXError( pgdx, status, "gdxOpenWrite" );
      return 1;
   }

   if( !pgdx.gdxDataWriteStrStart( "Demand", "Demand Data", 1, GMS_DT_PAR, 0 ) )
   {
      throwGDXError( pgdx, pgdx.gdxGetLastError(), "gdxDataWriteStrStart" );
      return 1;
   }

   // Initialize some GDX data structure
   gdxStrIndex_t strIndex;
   gdxStrIndexPtrs_t sp;
   GDXSTRINDEXPTRS_INIT( strIndex, sp );

   gdxValues_t v;
   const std::array<std::pair<std::string, double>, 3> demands {
           { { "New-York"s, 324.0 },
             { "Chicago"s, 299.0 },
             { "Topeka"s, 274.0 } } };
   for( const auto &[cityName, demandAmount]: demands )
   {
      std::strcpy( sp[0], cityName.c_str() );
      v[GMS_VAL_LEVEL] = demandAmount;
      pgdx.gdxDataWriteStr( const_cast<const char **>( sp ), v );
   }

   if( !pgdx.gdxDataWriteDone() )
   {
      throwGDXError( pgdx, pgdx.gdxGetLastError(), "gdxDataWriteDone" );
      return 1;
   }

   if( pgdx.gdxClose() )
   {
      throwGDXError( pgdx, pgdx.gdxGetLastError(), "gdxClose" );
      return 1;
   }

   return 0;
}

int WriteSolveReadExample::CallGams( const std::string &sysdir, const std::string &model )
{
   if( optReadDefinition( opt, ( sysdir + "/optgams.def"s ).c_str() ) )
   {
      int itype;
      for( int i {}; i < optMessageCount( opt ); i++ )
      {
         optGetMessage( opt, i + 1, msg.data(), &itype );
         std::cout << msg.data() << std::endl;
      }
      return 1;
   }

   optSetStrStr( opt, "input", model.c_str() );
   optSetIntStr( opt, "logoption", 2 );
   optWriteParameterFile( opt, "gams.opt" );

   return std::system( ( "\""s + sysdir + "/gams\" dummy pf=gams.opt"s ).c_str() );
}

int WriteSolveReadExample::ReadSolutionData( const char *fngdxfile )
{
   int status;
   pgdx.gdxOpenRead( fngdxfile, status );
   if( status )
      throwGDXError( pgdx, status, "gdxOpenRead" );

   std::array<char, GMS_SSSIZE> VarName {};
   std::strcpy( VarName.data(), "result" );
   int VarNr;
   if( 0 == pgdx.gdxFindSymbol( VarName.data(), VarNr ) )
   {
      std::cout << "Could not find variable >" << VarName.data() << "<" << std::endl;
      return 1;
   }

   int dim, varType;
   pgdx.gdxSymbolInfo( VarNr, VarName.data(), dim, varType );
   if( 2 != dim || GMS_DT_VAR != varType )
   {
      std::cout << VarName.data() << " is not a two dimensional variable" << std::endl;
      return 1;
   }

   int NrRecs;
   if( !pgdx.gdxDataReadStrStart( VarNr, NrRecs ) )
      throwGDXError( pgdx, pgdx.gdxGetLastError(), "gdxDataReadStrStart" );

   // Initialize some GDX data structure
   gdxStrIndex_t strIndex;
   gdxStrIndexPtrs_t sp;
   GDXSTRINDEXPTRS_INIT( strIndex, sp );

   int FDim;
   gdxValues_t v;
   while( pgdx.gdxDataReadStr( sp, v, FDim ) )
   {
      // skip level = 0.0 is default
      if( v[GMS_VAL_LEVEL] == 0.0 ) continue;
      for( int i {}; i < dim; i++ )
         std::cout << sp[i] << ( ( i < dim - 1 ) ? "." : "" );
      std::cout << " = " << v[GMS_VAL_LEVEL] << std::endl;
   }
   std::cout << "All solution values shown" << std::endl;

   pgdx.gdxDataReadDone();

   if( ( status = pgdx.gdxGetLastError() ) )
      throwGDXError( pgdx, status, "GDX" );

   if( pgdx.gdxClose() )
      throwGDXError( pgdx, pgdx.gdxGetLastError(), "gdxClose" );

   return 0;
}

int main( int argc, char *argv[] )
{
   if( argc < 2 || argc > 3 )
      throw std::runtime_error( "xp_Example2: Incorrect number of parameters, first arg is sysDir, second arg is model to execute (optional)" );

   std::string defModel { "../GAMS/model2.gms"s },
           model { argc > 2 ? argv[2] : defModel },
           sysdir { argv[1] };

   std::cout << "Loading objects from GAMS system directory: " << sysdir << std::endl;

   // Create objects
   WriteSolveReadExample wsre { sysdir };
   if( wsre.WriteModelData( "demanddata.gdx" ) )
      throw std::runtime_error( "Model data not written" );
   if( wsre.CallGams( sysdir, model ) )
      throw std::runtime_error( "Call to GAMS failed" );
   if( wsre.ReadSolutionData( "results.gdx" ) )
      throw std::runtime_error( "Could not read solution back" );

   return 0;
}
