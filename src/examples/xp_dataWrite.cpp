/**
 *
 * GAMS - General Algebraic Modeling System C++ API
 *
 * Copyright (c) 2017-2026 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2026 GAMS Development Corp. <support@gams.com>
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
  Use this command to compile the example (Windows):
  cl xp_dataWrite.cpp ../C/api/gdxcc.c ../C/api/gmdcc.c -I../C/api
  Use this command to compile the example (Unix):
  g++ xp_dataWrite.cpp ../C/api/gdxcc.c ../C/api/gmdcc.c -I../C/api -ldl
*/

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <ctime>

#include <string>
#include <iostream>
#include <sstream>

#include "gdx.hpp"
#include "gmdcc.h"

using namespace std::literals::string_literals;

enum writeMode
{
   ordered,
   oneOOO,
   reversed
};

class worker
{
   std::unique_ptr<gdx::TGXFileObj> gdx {};
   gmdHandle_t gmdHandle {};
   static constexpr int size = 200;
   char iarrayC[size][GMS_SSSIZE] {};// GMS_UEL_IDENT_SIZE?
   char jarrayC[size][GMS_SSSIZE] {};
   char karrayC[size][GMS_SSSIZE] {};
   clock_t tstart {};
   int IndxI[GMS_MAX_INDEX_DIM] {};
   gdxStrIndex_t IndxC {};
   gdxStrIndexPtrs_t IndxCPtrs {};
   gdxValues_t Values {};

   static void ReportIOError( int N, const std::string &msg );

   void ReportGDXError(gdx::TGXFileObj &gdx) const;

   void ReportGMDError() const;

   void PrintDiff( const std::string &msg, clock_t tm ) const;

public:
   void initC( const std::string &sysDir );

   void useStr( const std::string &fileName, const std::string &txt, writeMode mode );

   void useRaw( const std::string &fileName, const std::string &txt );

   void useMap( const std::string &fileName, const std::string &txt, writeMode mode );

   void useGmd( const std::string &fileName, const std::string &txt );
};

void worker::ReportIOError( int N, const std::string &msg )
{
   std::cout << "**** Fatal I/O Error = " << N << " when calling " << msg << std::endl;
   exit( 1 );
}

void worker::ReportGDXError(gdx::TGXFileObj &gdx) const
{
   char s[GMS_SSSIZE];
   std::cout << "**** Fatal GDX Error" << std::endl;
   gdx.gdxErrorStr( gdx.gdxGetLastError(), s );
   std::cout << "**** " << s << std::endl;
   exit( 1 );
}

void worker::ReportGMDError() const
{
   char s[GMS_SSSIZE];
   std::cout << "**** Fatal GMD Error" << std::endl;
   gmdGetLastError( gmdHandle, s );
   std::cout << "**** " << s << std::endl;
   exit( 1 );
}

void worker::PrintDiff( const std::string &msg, clock_t tm ) const
{
   const float diff = ( (float) tm - (float) tstart ) / CLOCKS_PER_SEC;
   std::cout << msg << diff << std::endl;
}

void worker::initC( const std::string &sysDir )
{
   std::string msgStr;
   gdx = std::make_unique<gdx::TGXFileObj>( msgStr );
   if( !msgStr.empty() )
      throw std::runtime_error( "**** Could not load GDX (c) library\n**** \n"s );

   char msg[GMS_SSSIZE];
   gdx->gdxGetDLLVersion( msg );
   std::cout << "Using GDX DLL version: " << msg << std::endl;

   if( !gmdCreateD( &gmdHandle, sysDir.c_str(), msg, sizeof( msg ) ) )
   {
      std::cout << "**** Could not load GMD (c) library" << std::endl
                << "**** " << msg << std::endl;
      exit( 1 );
   }

   for( int i = 0; i < size; i++ )
   {
      snprintf( iarrayC[i], GMS_UEL_IDENT_SIZE, "i%d", i );
      snprintf( jarrayC[i], GMS_UEL_IDENT_SIZE, "j%d", i );
      snprintf( karrayC[i], GMS_UEL_IDENT_SIZE, "k%d", i );
   }

   GDXSTRINDEXPTRS_INIT( IndxC, IndxCPtrs );
   Values[GMS_VAL_LEVEL] = 11;
}

void worker::useStr( const std::string &fileName, const std::string &txt, writeMode mode )
{
   int ErrNr, dummy;
   tstart = clock();
   gdx->gdxOpenWrite( fileName.c_str(), "example", ErrNr );
   if( ErrNr ) ReportIOError( ErrNr, "gdxOpenWrite" );

   if( !gdx->gdxUELRegisterStrStart() ) ReportGDXError(*gdx);
   for( const auto &i: iarrayC )
      if( !gdx->gdxUELRegisterStr( i, dummy ) ) ReportGDXError(*gdx);
   for( const auto &j: jarrayC )
      if( !gdx->gdxUELRegisterStr( j, dummy ) ) ReportGDXError(*gdx);
   for( const auto &k: karrayC )
      if( !gdx->gdxUELRegisterStr( k, dummy ) ) ReportGDXError(*gdx);
   if( !gdx->gdxUELRegisterDone() ) ReportGDXError(*gdx);

   if( !gdx->gdxDataWriteStrStart( "Demand", "Demand data", 3, GMS_DT_PAR, 0 ) ) ReportGDXError(*gdx);

   switch( mode )
   {
      case ordered:
         for( auto &i: iarrayC )
         {
            strcpy( IndxC[0],
                    i );//this would be quicker, but "unfair" to compare it with the c++ approach: IndxCPtrs[0] = iarrayC[i];
            for( auto &j: jarrayC )
            {
               strcpy( IndxC[1], j );
               for( auto &k: karrayC )
               {
                  strcpy( IndxC[2], k );
                  if( !gdx->gdxDataWriteStr( (const char **) IndxCPtrs, Values ) ) ReportGDXError(*gdx);
               }
            }
         }
         break;
      case oneOOO:
         for( int i = 0; i < size; i++ )
         {
            strcpy( IndxC[0], iarrayC[i] );
            for( int j = 0; j < size; j++ )
            {
               strcpy( IndxC[1], jarrayC[j] );
               for( int k = 0; k < size; k++ )
               {
                  strcpy( IndxC[2], karrayC[k] );
                  if( i + j + k > 0 )
                     if( !gdx->gdxDataWriteStr( (const char **) IndxCPtrs, Values ) ) ReportGDXError(*gdx);
               }
            }
         }
         strcpy( IndxC[0], iarrayC[0] );
         strcpy( IndxC[1], jarrayC[0] );
         strcpy( IndxC[2], karrayC[0] );
         if( !gdx->gdxDataWriteStr( (const char **) IndxCPtrs, Values ) ) ReportGDXError(*gdx);
         break;
      case reversed:
         for( int i = size - 1; i > -1; i-- )
         {
            strcpy( IndxC[0], iarrayC[i] );
            for( int j = size - 1; j > -1; j-- )
            {
               strcpy( IndxC[1], jarrayC[j] );
               for( int k = size - 1; k > -1; k-- )
               {
                  strcpy( IndxC[2], karrayC[k] );
                  if( !gdx->gdxDataWriteStr( (const char **) IndxCPtrs, Values ) ) ReportGDXError(*gdx);
               }
            }
         }
         break;
   }

   PrintDiff( "Before Done (" + txt + "): ", clock() );

   if( !gdx->gdxDataWriteDone() ) ReportGDXError(*gdx);
   ErrNr = gdx->gdxClose();
   if( ErrNr ) ReportIOError( ErrNr, "gdxClose" );

   PrintDiff( "Running time (" + txt + "): ", clock() );
}

void worker::useRaw( const std::string &fileName, const std::string &txt )
{
   int ErrNr;

   tstart = clock();

   gdx->gdxOpenWrite( fileName.c_str(), "example", ErrNr );
   if( ErrNr ) ReportIOError( ErrNr, "gdxOpenWrite" );

   if( !gdx->gdxUELRegisterRawStart() ) ReportGDXError(*gdx);
   for( auto &i: iarrayC )
      if( !gdx->gdxUELRegisterRaw( i ) ) ReportGDXError(*gdx);
   for( auto &j: jarrayC )
      if( !gdx->gdxUELRegisterRaw( j ) ) ReportGDXError(*gdx);
   for( auto &k: karrayC )
      if( !gdx->gdxUELRegisterRaw( k ) ) ReportGDXError(*gdx);
   if( !gdx->gdxUELRegisterDone() ) ReportGDXError(*gdx);

   if( !gdx->gdxDataWriteRawStart( "Demand", "Demand data", 3, GMS_DT_PAR, 0 ) ) ReportGDXError(*gdx);

   for( int i = 0; i < size; i++ )
   {
      IndxI[0] = i + 1;
      for( int j = 0; j < size; j++ )
      {
         IndxI[1] = size + j + 1;
         for( int k = 0; k < size; k++ )
         {
            IndxI[2] = 2 * size + k + 1;
            if( !gdx->gdxDataWriteRaw( IndxI, Values ) ) ReportGDXError(*gdx);
         }
      }
   }

   PrintDiff( "Before Done (" + txt + "): ", clock() );

   if( !gdx->gdxDataWriteDone() ) ReportGDXError(*gdx);
   ErrNr = gdx->gdxClose();
   if( ErrNr ) ReportIOError( ErrNr, "gdxClose" );

   PrintDiff( "Running time (" + txt + "): ", clock() );
}

void worker::useMap( const std::string &fileName, const std::string &txt, writeMode mode )
{
   int ErrNr;

   tstart = clock();

   gdx->gdxOpenWrite( fileName.c_str(), "example", ErrNr );
   if( ErrNr ) ReportIOError( ErrNr, "gdxOpenWrite" );

   if( !gdx->gdxUELRegisterMapStart() ) ReportGDXError(*gdx);
   for( int i = 0; i < size; i++ )
      if( !gdx->gdxUELRegisterMap( i, iarrayC[i] ) ) ReportGDXError(*gdx);
   for( int j = 0; j < size; j++ )
      if( !gdx->gdxUELRegisterMap( size + j, jarrayC[j] ) ) ReportGDXError(*gdx);
   for( int k = 0; k < size; k++ )
      if( !gdx->gdxUELRegisterMap( 2 * size + k, karrayC[k] ) ) ReportGDXError(*gdx);
   if( !gdx->gdxUELRegisterDone() ) ReportGDXError(*gdx);

   if( !gdx->gdxDataWriteMapStart( "Demand", "Demand data", 3, GMS_DT_PAR, 0 ) ) ReportGDXError(*gdx);

   switch( mode )
   {
      case ordered:
         for( int i = 0; i < size; i++ )
         {
            IndxI[0] = i;
            for( int j = 0; j < size; j++ )
            {
               IndxI[1] = size + j;
               for( int k = 0; k < size; k++ )
               {
                  IndxI[2] = 2 * size + k;
                  if( !gdx->gdxDataWriteMap( IndxI, Values ) ) ReportGDXError(*gdx);
               }
            }
         }
         break;
      case oneOOO:
         for( int i = 0; i < size; i++ )
         {
            IndxI[0] = i;
            for( int j = 0; j < size; j++ )
            {
               IndxI[1] = size + j;
               for( int k = 0; k < size; k++ )
               {
                  IndxI[2] = 2 * size + k;
                  if( i + j + k > 0 )
                     if( !gdx->gdxDataWriteMap( IndxI, Values ) ) ReportGDXError(*gdx);
               }
            }
         }

         IndxI[0] = 0;
         IndxI[1] = 0;
         IndxI[2] = 0;
         if( !gdx->gdxDataWriteMap( IndxI, Values ) ) ReportGDXError(*gdx);
         break;
      case reversed:
         for( int i = size - 1; i > -1; i-- )
         {
            IndxI[0] = i;
            for( int j = size - 1; j > -1; j-- )
            {
               IndxI[1] = size + j;
               for( int k = size - 1; k > -1; k-- )
               {
                  IndxI[2] = 2 * size + k;
                  if( !gdx->gdxDataWriteMap( IndxI, Values ) ) ReportGDXError(*gdx);
               }
            }
         }
         break;
   }

   PrintDiff( "Before Done (" + txt + "): ", clock() );

   if( !gdx->gdxDataWriteDone() ) ReportGDXError(*gdx);
   ErrNr = gdx->gdxClose();
   if( ErrNr ) ReportIOError( ErrNr, "gdxClose" );

   PrintDiff( "Running time (" + txt + "): ", clock() );
}

void worker::useGmd( const std::string &fileName, const std::string &txt )
{
   void *symPtr, *symRecPtr;

   tstart = clock();

   if( !gmdAddSymbol( gmdHandle, "Demand", 3, GMS_DT_PAR, 0, "", &symPtr ) ) ReportGMDError();

   for( auto &i: iarrayC )
   {
      strcpy( IndxC[0], i );
      for( auto &j: jarrayC )
      {
         strcpy( IndxC[1], j );
         for( auto &k: karrayC )
         {
            strcpy( IndxC[2], k );
            if( !gmdAddRecord( gmdHandle, symPtr, (const char **) IndxCPtrs, &symRecPtr ) ) ReportGMDError();
            if( !gmdSetLevel( gmdHandle, symRecPtr, 11.0 ) ) ReportGMDError();
         }
      }
   }

   PrintDiff( "Before Done (" + txt + "): ", clock() );

   if( !gmdWriteGDX( gmdHandle, fileName.c_str(), true ) ) ReportGMDError();

   PrintDiff( "Running time (" + txt + "): ", clock() );
}

int main( int argc, char *argv[] )
{
   if( argc != 2 )
   {
      std::cout << "**** DataWrite: incorrect number of parameters" << std::endl;
      exit( 1 );
   }

   std::string Sysdir { argv[1] };
   std::cout << "DataWrite using GAMS system directory: " << Sysdir << std::endl;

   worker w;

   std::cout << "Start using C API (aka use *char all the time)" << std::endl;
   w.initC( Sysdir );

   w.useStr( "stringOrderedC.gdx", "strOrdered", ordered );
   w.useStr( "string1oooC.gdx", "str1ooo", oneOOO );
   w.useStr( "stringRevOrdC.gdx", "strRevOrd", reversed );

   w.useRaw( "raw.gdx", "raw" );

   w.useMap( "mapOrdered.gdx", "mapOrdered", ordered );
   w.useMap( "map1ooo.gdx", "map1ooo", oneOOO );
   w.useMap( "mapRevOrd.gdx", "mapRevOrd", reversed );

   w.useGmd( "gmd.gdx", "gmd" );

   std::cout << "All data written" << std::endl;
   return 0;
}
