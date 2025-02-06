/*
* GAMS - General Algebraic Modeling System GDX API
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


#pragma once

#include <vector>
#include <string>
#include <map>

// Description:
//    Object that handles command line parameters

namespace gdlib::cmdpar
{

// Parameter record
struct TParamRec {
   int key {};      // key number
   std::string keys;// key value
};
using PParamRec = TParamRec *;

class TCmdParams
{// Class to handle command line parameters
   std::map<std::string, int> FKeyList;
   std::map<int, TParamRec> FParList;

   void ClearParams();
   void AddVS( int v, const std::string &s );
   int FindKeyV( int V );

public:
   TCmdParams();
   ~TCmdParams();

   bool CrackCommandLine();
   bool AddParameters( int AInsP, const std::string &CmdLine );
   void AddKeyWord( int v, const std::string &s );
   void AddParam( int v, const std::string &s );
   bool HasParam( int v, std::string &s );
   bool HasKey( int v );

   TParamRec GetParams( int n );
   [[nodiscard]] int GetParamCount() const;
   [[nodiscard]] std::string GetParamText( int key ) const;
};

enum CndParamStatus
{
   kp_input,
   ke_empty = -1,
   ke_unknown = -2,
   ke_noparam = -3,
   ke_badfile = -4,
   kk_big = 10000
};
}// namespace gdlib::cmdpar