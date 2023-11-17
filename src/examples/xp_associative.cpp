/**
 *
 * GAMS - General Algebraic Modeling System C++ API
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

/*
  Use this command to compile the example:
  cl xp_associative.cpp ../C/api/gdxcc.c -I../C/api
*/

#include <string>
#include <iostream>
#include <map>
#include <array>

#include "gdx.h"

using namespace std::literals::string_literals;

std::map<std::string, double> fillXLevelMapFromGDX(const std::string &filename);
void processXLevelMap(std::map<std::string, double> &xlevel);

int main( int argc, const char **argv )
{
   try
   {
      const auto filename {argc >= 2 ? std::string{argv[1]} : "trnsport.gdx"s};
      auto xlevel {fillXLevelMapFromGDX( filename )};
      processXLevelMap( xlevel );
   }
   catch(std::runtime_error &e)
   {
      std::cout << e.what() << std::endl;
      return 1;
   }
   return 0;
}

std::map<std::string, double> fillXLevelMapFromGDX(const std::string &filename)
{
   std::string msg;
   gdx::TGXFileObj gdx { msg };
   if(!msg.empty())
      throw std::runtime_error("Unable to instantiate GDX object: "s + msg);

   int errNr {};
   int rc {gdx.gdxOpenRead( filename.c_str(), errNr )};
   if( !rc || errNr != 0 )
   {
      std::array<char, GMS_SSSIZE> msgBuf{};
      gdx::TGXFileObj::gdxErrorStr( errNr, msgBuf.data() );
      throw std::runtime_error("*** Error opening trnsport.gdx: "s + msgBuf.data());
   }

   int varNr{};
   rc = gdx.gdxFindSymbol( "x", varNr );
   if( !rc )
      throw std::runtime_error("*** Cannot find symbol 'x'");

   std::array<char, GMS_SSSIZE> varName {};
   int dim {}, varType {};
   rc = gdx.gdxSymbolInfo( varNr, varName.data(), dim, varType );
   if(!rc)
      throw std::runtime_error("Getting symbol info for variable (x) #"s + std::to_string(varNr) + " failed!"s);
   if( dim != 2 || GMS_DT_VAR != varType )
      throw std::runtime_error("*** x is not a two dimensional variable.  dim:"s + std::to_string(dim) + "  type:"s + std::to_string(varType));

   int nrRecs{};
   if( !gdx.gdxDataReadStrStart( varNr, nrRecs ) )
      throw std::runtime_error("*** Cannot read symbol 'x'"s);

   static gdxStrIndexPtrs_t idx;
   static gdxStrIndex_t idxXXX;
   GDXSTRINDEXPTRS_INIT( idxXXX, idx );

   std::map<std::string, double> xlevel;

   for( int i = 0; i < nrRecs; i++ )
   {
      gdxValues_t values;
      int n{};
      gdx.gdxDataReadStr( idx, values, n );
      std::string key = ""s + idx[0] + "."s + idx[1];
      xlevel[key] = values[GMS_VAL_LEVEL];
   }

   rc = gdx.gdxDataReadDone();
   if( !rc )
      throw std::runtime_error("*** Unable to finish reading records of symbol 'x'"s);

   gdx.gdxClose();

   return xlevel;
}

void processXLevelMap(std::map<std::string, double> &xlevel)
{
   std::cout << "-----\nrandom access of a key that does not exist:" << std::endl;
   auto key {"seattle.something else"s};
   if( xlevel.find( key ) != xlevel.end() )
      std::cout << key << " : " << xlevel[key] << std::endl;
   else
      std::cout << "key does not exist" << std::endl;

   std::cout << "-----\nrandom access of a key that does exist:" << std::endl;
   key = "seattle.chicago"s;
   if( xlevel.find( key ) != xlevel.end() )
      std::cout << key << " : " << xlevel[key] << std::endl;
   else
      std::cout << "key does not exist" << std::endl;

   std::cout << "-----\niterate through keys and perform random access to get the values:" << std::endl;

   for(const auto &[k,v] : xlevel)
      std::cout << k << " : " << v << std::endl;

   std::cout << "-----\nchange the value of a key and access it once again:" << std::endl;
   key = "seattle.chicago";
   xlevel[key] = 111.1;
   std::cout << key << " : " << xlevel[key] << std::endl;

   std::cout << "\nxp_associative DONE" << std::endl;
}