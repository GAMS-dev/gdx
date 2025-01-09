/**
 *
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

/*
   Use this command to compile the example:
   cl xp_example1.cpp ../C/api/gdxcc.c -I../C/api
 */

#include <string>
#include <array>

#include <cstring>
#include <cstdlib>
#include <iostream>

#include "gdx.hpp"

using namespace std::literals::string_literals;
using namespace gdx;

class SimpleWriteReadExample
{
   gdxStrIndexPtrs_t Indx {};
   gdxStrIndex_t IndxXXX {};
   gdxValues_t Values {};

   void ReportGDXError();
   void WriteData( const char *s, double V );
   static void ReportIOError( int N, const std::string &msg );

   std::string ErrMsg;
   TGXFileObj pgx { ErrMsg };

public:
   explicit SimpleWriteReadExample();
   ~SimpleWriteReadExample();
   void writeDemandData();
   void readFromFile( const std::string &gdxInputFilename );
};

void SimpleWriteReadExample::ReportGDXError()
{
   std::array<char, GMS_SSSIZE> S {};
   std::string ostr = "**** Fatal GDX Error\n"s;
   TGXFileObj::gdxErrorStr( pgx.gdxGetLastError(), S.data() );
   ostr += "**** "s + S.data();
   throw std::runtime_error( ostr );
}

void SimpleWriteReadExample::ReportIOError( int N, const std::string &msg )
{
   throw std::runtime_error( "**** Fatal I/O Error = "s + std::to_string( N ) + " when calling "s + msg );
}

void SimpleWriteReadExample::WriteData( const char *s, double V )
{
   std::strcpy( Indx[0], s );
   Values[GMS_VAL_LEVEL] = V;
   pgx.gdxDataWriteStr( const_cast<const char **>(Indx), Values );
}

SimpleWriteReadExample::SimpleWriteReadExample()
{
   if( !ErrMsg.empty() )
      throw std::runtime_error( "**** Could not load GDX library\n**** "s + ErrMsg );

   std::array<char, GMS_SSSIZE> Msg {};
   TGXFileObj::gdxGetDLLVersion( Msg.data() );
   std::cout << "Using GDX DLL version: " << Msg.data() << std::endl;

   GDXSTRINDEXPTRS_INIT( IndxXXX, Indx );
}

SimpleWriteReadExample::~SimpleWriteReadExample()
{
   if( int ErrNr = pgx.gdxClose() )
      ReportIOError( ErrNr, "gdxClose" );
}

void SimpleWriteReadExample::writeDemandData()
{
   // Write demand data
   int ErrNr;
   pgx.gdxOpenWrite( "demanddata.gdx", "xp_example1", ErrNr );
   if( ErrNr ) ReportIOError( ErrNr, "gdxOpenWrite" );
   if( !pgx.gdxDataWriteStrStart( "Demand", "Demand data", 1, GMS_DT_PAR, 0 ) )
      ReportGDXError();
   WriteData( "New-York", 324.0 );
   WriteData( "Chicago", 299.0 );
   WriteData( "Topeka", 274.0 );
   if( !pgx.gdxDataWriteDone() ) ReportGDXError();
   std::cout << "Demand data written by example1" << std::endl;
}

void SimpleWriteReadExample::readFromFile( const std::string &gdxInputFilename )
{
   int ErrNr;
   pgx.gdxOpenRead( gdxInputFilename.c_str(), ErrNr );
   if( ErrNr ) ReportIOError( ErrNr, "gdxOpenRead" );
   std::array<char, GMS_SSSIZE> Msg {}, Producer {};
   pgx.gdxFileVersion( Msg.data(), Producer.data() );
   std::cout << "GDX file written using version: " << Msg.data() << std::endl;
   std::cout << "GDX file written by: " << Producer.data() << std::endl;

   int VarNr;
   if( !pgx.gdxFindSymbol( "x", VarNr ) )
   {
      std::cout << "**** Could not find variable X" << std::endl;
      exit( 1 );
   }

   std::array<char, GMS_SSSIZE> VarName {};
   int Dim, VarTyp;
   pgx.gdxSymbolInfo( VarNr, VarName.data(), Dim, VarTyp );
   if( Dim != 2 || GMS_DT_VAR != VarTyp )
   {
      std::cout << "**** X is not a two dimensional variable: "
                << Dim << ":" << VarTyp << std::endl;
      exit( 1 );
   }

   int NrRecs, N;
   if( !pgx.gdxDataReadStrStart( VarNr, NrRecs ) ) ReportGDXError();
   std::cout << "Variable X has " << NrRecs << " records" << std::endl;
   while( pgx.gdxDataReadStr( Indx, Values, N ) )
   {
      // skip level 0.0 is default
      if( Values[GMS_VAL_LEVEL] == 0.0 ) continue;
      for( int D {}; D < Dim; D++ ) std::cout << ( D ? '.' : ' ' ) << Indx[D];
      std::cout << " = " << Values[GMS_VAL_LEVEL] << '\n';
   }
   std::cout << "All solution values shown" << std::endl;
   pgx.gdxDataReadDone();
}

void showUsage();
void showUsage()
{
   std::cout << "Usage: ./xp_example1 [xyz.gdx]" << std::endl
             << "Where optional GDX filename loads data from that file" << std::endl
             << "Otherwise a GDX file is written to disk" << std::endl;
}

int main( int argc, char **argv )
{
   if( argc < 1 || argc > 2 )
   {
      std::cout << "**** xp_Example1: incorrect number of parameters" << std::endl;
      showUsage();
      exit( 1 );
   }

   SimpleWriteReadExample swre;
   if( argc == 1 ) swre.writeDemandData();
   else
      swre.readFromFile( argv[1] );
   return 0;
}
