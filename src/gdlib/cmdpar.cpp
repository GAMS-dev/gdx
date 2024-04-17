/*
* GAMS - General Algebraic Modeling System GDX API
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


#include "cmdpar.h"
#include "utils.h"

#include <cassert>
#include <optional>

using namespace std::literals::string_literals;

namespace gdlib::cmdpar
{

// Brief:
//   Create instance of command line object
TCmdParams::TCmdParams() = default;

// Brief:
//  Release allocated memory and destroy the object
TCmdParams::~TCmdParams() = default;

void TCmdParams::ClearParams()
{
   FKeyList.clear();
   FParList.clear();
}

void TCmdParams::AddVS( int v, const std::string &s )
{
   assert( !utils::in( s, FKeyList ) && "Duplicate keyword!" );
   FKeyList[s] = v;
}

//search from the end so the LAST value will be used
int TCmdParams::FindKeyV( int V )
{
   for( int N { GetParamCount() - 1 }; N >= 0; N-- )
      if( GetParams( N ).key == V )
         return N;
   return -1;
}

// Brief:
//   Crack the command line
// Returns:
//   True if there were no synyax errors; false otherwise
// Description:
//   After specifying the strings and integer values for
//   keywords and parameters (see AddKeyW and AddParam)
//   this function will build the list of keywords and parameters.
//   A string that starts with '@' is interpreted as the name of
//   a file with additional parameters.
bool TCmdParams::CrackCommandLine()
{
   return AddParameters( 0, ""s );
}

// Brief:
//   Insert additional parameters
// Parameters:
//   AInsp: Insertion point for additional parameters
//   CmdLine: Additional parameters
// Description:
//   This function is used to add more parameters to the parameter list
//   that could not be processed while reading the command line for the
//   first time.
bool TCmdParams::AddParameters( int AInsP, const std::string &CmdLine )
{
   std::string fparams;
   int xr, maxr;

   auto SkipBl = [&]() {
      for( ; xr <= maxr && fparams[xr] <= ' '; xr++ )
         ;
   };

   auto NextToken = [&]() -> std::string {
      std::string res;
      SkipBl();
      if( xr <= maxr )
      {
         utils::charset StopSet;
         if( fparams[xr] != '"' ) StopSet = { ' ', '\t' };
         else
         {
            StopSet = { '"' };
            xr++;
         }
         StopSet.insert( '=' );
         int xk {};
         for( int k { 1 }; k <= maxr - xr + 1; k++ )
         {
            if( utils::in( fparams[xr + k - 1], StopSet ) )
            {
               xk = k;
               break;
            }
            if( !xk )
            {
               res += fparams.substr( xr, fparams.length() - xr + 1 );
               xr = maxr + 1;
            }
            else
            {
               res += fparams.substr( xr, xk - 1 );
               xr += xk;
            }
         }
      }
      return res;
   };

   auto NextKey = [&]( std::string &s ) -> int {
      s.clear();
      auto T {NextToken()};
      if(T.empty())
         return ke_empty;
      // ...
      return 0;
   };

   auto ExpandCommandLine = []() -> std::string {
      return ""s;
   };

   auto ExpandFiles = []( const std::string &Src, const std::string &Dest ) -> bool {
      return false;
   };

   /* AddParameters */

   TParamRec P;
   std::string ks;
   int Insp;
   bool res {};

   if( !CmdLine.empty() )
   {
      Insp = AInsP;
      res = ExpandFiles( CmdLine, fparams );
   }
   else
   {
      Insp = 0;
      const std::string wrk = ExpandCommandLine();
      res = ExpandFiles( wrk, fparams );
   }

   xr = 1;
   maxr = static_cast<int>(fparams.length());
   do {
      const int kw = NextKey( ks );
      if( kw == ke_empty )
         break;
      P.key = kw;
      P.keys = ks;
      FParList[Insp] = P;
      Insp++;
   } while( true );
   if( !HasKey( kp_input ) && GetParams( 0 ).key == ke_unknown )
      FParList[0].key = kp_input;
   return res;
}

void TCmdParams::AddKeyWord( int v, const std::string &s )
{
}

void TCmdParams::AddParam( int v, const std::string &s )
{
}

bool TCmdParams::HasParam( int v, std::string &s )
{
   return false;
}

// Brief:
//   Check if a keyword was specified
// Parameters:
//   V: Key number
// Returns:
//   True if keyword was specified; false otherwise
bool TCmdParams::HasKey( int v )
{
   return FindKeyV( v ) >= 0;
}

TParamRec TCmdParams::GetParams( int n )
{
   return n >= 0 && n < static_cast<int>( FParList.size() ) ? FParList[n] : TParamRec { ke_empty, "" };
}

int TCmdParams::GetParamCount() const
{
   return static_cast<int>(FParList.size());
}

std::string TCmdParams::GetParamText( int key ) const
{
   std::optional<std::string> name = utils::keyForValue( FKeyList, key );
   return name ? *name : "?"s + std::to_string( key ) + "?"s;
}
}// namespace gdlib::cmdpar