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
#include <map>

#include "gdx2veda.h"
#include "../library/common.h"
#include "../../gdlib/strutilx.h"

namespace gdx2veda
{

void ShortHelp()
{
   std::cout << '\n'
             //  << gdlGetAuditLine() << '\n'
             //  << '\n'
             //  << '\n'
             << ">gdx2veda gdx vdd [run]" << '\n'
             << '\n'
             << "   gdx  GAMS GDX file" << '\n'
             << "   vdd  VEDA Data Definition file" << '\n'
             << "   run  VEDA Run identifier (optional)" << '\n'
             << '\n'
             << "The VEDA data file name and run identifier are either taken from the gdx file name" << '\n'
             << "or specified with the run name. Use \"token with blanks\" if needed." << '\n'
             << '\n'
             << ">gdx2veda mygdx    //  will dump the gdx symbols" << '\n'
             << ">gdx2veda          //  prints this message" << '\n'
             << ">gdx2veda --help   //  prints more detailed help message" << '\n'
             << '\n'
             << "Add .csv to the run name to write in csv format" << '\n'
             << std::endl;
}

gdxSVals_t SpecialValues {};
void GetSpecialValues( const gdxHandle_t &PGX )
{
   gdxResetSpecialValues( PGX );
   gdxGetSpecialValues( PGX, SpecialValues );
}

int main( const int argc, const char *argv[] )
{
   int ParamCount { argc - 1 };
   const char **ParamStr { argv };

   // TODO: Remove? Update to GDX2VEDA?
   // gdlSetSystemName( 'GMS2VEDA' );
   // if( gdlib::strutilx::StrUEqual( ParamStr[1], "AUDIT" ) )
   // {
   //    std::cout << gdlGetAuditLine() << std::endl;
   //    return 0;
   // }

   // unsigned int NumErr {};

   std::map<gdxSyType, std::string> DataText {
           { dt_set, "Set" },
           { dt_par, "Par" },
           { dt_var, "Var" },
           { dt_equ, "Equ" } };

   std::string help;
   if( ParamCount == 0 )
      help = "-H";
   else
      help = gdlib::strutilx::UpperCase( ParamStr[1] );

   if( help == "--HELP" )
   {
      ShortHelp();
      // VddHelp();
      return 0;
   }

   if( help == "-H" || help == "-HELP" || help == "/?" || help == "?" )
   {
      ShortHelp();
      return 0;
   }

   library::short_string msg;
   if( !gdxGetReady( msg.data(), msg.length() ) )
   {
      library::printErrorMessage( "*** Could not load GDX library" );
      library::printErrorMessage( "*** Msg: " + msg.string() );
      return 1;
   }

   gdxHandle_t PGX;
   std::string fngdx = gdlib::strutilx::CompleteFileExtEx( ParamStr[1], ".gdx" );
   gdxCreate( &PGX, msg.data(), msg.length() );
   int rc;
   gdxOpenRead( PGX, fngdx.data(), &rc );
   if( rc != 0 )
   {
      gdxErrorStr( nullptr, rc, msg.data() );
      library::printErrorMessage( "*** Could not open GDX: " + fngdx );
      library::printErrorMessage( "*** Msg: " + msg.string() );
      // UnloadGdxLibrary();
      return 1;
   }

   GetSpecialValues( PGX );
   int NrSy, NrUel;
   gdxSystemInfo( PGX, &NrSy, &NrUel );
   std::cout << "--- GDX File : Symbols=" << NrSy << " UELs=" << NrUel << std::endl;

   return 0;
}

}// namespace gdx2veda

int main( const int argc, const char *argv[] )
{
   return gdx2veda::main( argc, argv );
}
