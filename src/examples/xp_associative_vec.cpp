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
  cl xp_associative_vec.cpp ../C/api/gdxcc.c -I../C/api
*/

#include <string>
#include <iostream>
#include <map>
#include <vector>

#include "gdx.hpp"

using namespace std::literals::string_literals;

std::map<std::vector<std::string>, double> loadXLevelMapFromGDX( const std::string &filename );

void processMap( std::map<std::vector<std::string>, double> &xlevel );

int main( int argc, char *argv[] )
{
   try
   {
      std::string filename { argc >= 2 ? std::string { argv[1] } : "trnsport.gdx"s };
      auto xlevel { loadXLevelMapFromGDX( filename ) };
      processMap( xlevel );
   }
   catch( std::runtime_error &e )
   {
      std::cout << e.what() << std::endl;
      return 1;
   }
   return 0;
}

std::map<std::vector<std::string>, double> loadXLevelMapFromGDX( const std::string &filename )
{
   std::string msg;
   gdx::TGXFileObj gdx { msg };
   if( !msg.empty() )
      throw std::runtime_error( "*** Could not load GDX library!" );

   int errNr {};
   int rc = gdx.gdxOpenRead( filename.c_str(), errNr );
   if( !rc || errNr != 0 )
   {
      std::array<char, GMS_SSSIZE> msgBuf {};
      gdx::TGXFileObj::gdxErrorStr( errNr, msgBuf.data() );
      throw std::runtime_error( "*** Error opening trnsport.gdx: "s + msgBuf.data() );
   }

   int varNr {};
   rc = gdx.gdxFindSymbol( "x", varNr );
   if( !rc )
      throw std::runtime_error( "*** Cannot find symbol 'x'"s );

   int dim {}, varType {};
   std::array<char, GMS_SSSIZE> varName {};
   rc = gdx.gdxSymbolInfo( varNr, varName.data(), dim, varType );
   if( !rc )
      throw std::runtime_error( "Unable to query symbol info for symbol 'x'"s );
   if( dim != 2 || GMS_DT_VAR != varType )
      throw std::runtime_error( "**** x is not a two dimensional variable.  dim:"s + std::to_string( dim ) + " type:"s + std::to_string( varType ) );

   int nrRecs {};
   rc = gdx.gdxDataReadStrStart( varNr, nrRecs );
   if( !rc )
      throw std::runtime_error( "*** Cannot read symbol 'x'"s );

   static gdxStrIndexPtrs_t idx;
   static gdxStrIndex_t idxXXX;
   GDXSTRINDEXPTRS_INIT( idxXXX, idx );

   std::map<std::vector<std::string>, double> xlevel;

   for( int i = 0; i < nrRecs; i++ )
   {
      std::vector<std::string> key( 2 );
      gdxValues_t values;
      int n {};
      gdx.gdxDataReadStr( idx, values, n );
      key[0] = idx[0];
      key[1] = idx[1];
      xlevel[key] = values[GMS_VAL_LEVEL];
   }

   gdx.gdxClose();

   return xlevel;
}

void processMap( std::map<std::vector<std::string>, double> &xlevel )
{
   std::cout << "-----\nrandom access of a key that does not exist:\n";
   std::vector key { "seattle"s, "something else"s };
   if( xlevel.find( key ) != xlevel.end() )
      std::cout << key[0] << "." << key[1] << " : " << xlevel[key] << std::endl;
   else
      std::cout << "key does not exist\n";

   std::cout << "-----\nrandom access of a key that does exist:" << std::endl;
   key[0] = "seattle";
   key[1] = "chicago";
   if( xlevel.find( key ) != xlevel.end() )
      std::cout << key[0] << "." << key[1] << " : " << xlevel[key] << std::endl;
   else
      std::cout << "key does not exist\n";

   std::cout << "-----\niterate through keys and perform random access to get the values:\n";
   for( const auto &[keys, v]: xlevel )
      std::cout << keys.front() << "." << keys[1] << " : " << v << std::endl;

   std::cout << "-----\nchange the value of a key and access it once again:\n";
   key[0] = "seattle";
   key[1] = "chicago";
   xlevel[key] = 111.1;
   std::cout << key[0] << "." << key[1] << " : " << xlevel[key] << std::endl;

   std::cout << "\nxp_associative_vec DONE" << std::endl;
}
