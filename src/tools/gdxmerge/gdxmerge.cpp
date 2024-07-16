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

#include <sstream>
#include <iomanip>
#include <string>
#include <vector>

#include "gdxmerge.h"

// GDX library interface
#include "../../../generated/gdxcc.h"

namespace gdxmerge
{

static gdxHandle_t PGXMerge { nullptr };
static bool DoBigSymbols, StrictMode;
static int64_t SizeCutOff;
static std::string OutFile;
static std::vector<std::string> FilePatterns;

std::string FormatDateTime( const std::tm &dt )
{
   auto int2 = []( const int n ) -> std::string {
      std::ostringstream oss;
      oss << std::setw( 2 ) << std::setfill( '0' ) << n;
      return oss.str();
   };

   const int year { dt.tm_year + 1900 },
           month { dt.tm_mon + 1 },
           day { dt.tm_mday },
           hour { dt.tm_hour },
           min { dt.tm_min },
           sec { dt.tm_sec };

   return int2( year ) + '/' + int2( month ) + '/' + int2( day ) + ' ' +
          int2( hour ) + ':' + int2( min ) + ':' + int2( sec );
}

int main( const int argc, const char *argv[] )
{
   return 0;
}

}// namespace gdxmerge

int main( const int argc, const char *argv[] )
{
   return gdxmerge::main( argc, argv );
}
