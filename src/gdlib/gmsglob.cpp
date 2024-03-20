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


#include "gmsglob.h"

#include "global/unit.h"

using namespace global::gmsspecs;

namespace gdlib::gmsglob
{
std::array<tvarreca, stypsemiint + 1> defrecvar;
std::array<tvarreca, ssyequb + 1> defrecequ;

void InitDefaultRecords()
{
   for( int v = tvarstyp::stypunknwn; v <= tvarstyp::stypsemiint; v++ )
   {
      for( int f = vallevel; f <= valupper; f++ )
         defrecvar[v][f] = 0.0;
      defrecvar[v][valscale] = 1.0;
   }

   defrecvar[stypbin][valupper] = 1.0;
   defrecvar[stypint][valupper] = valiup;
   defrecvar[styppos][valupper] = valpin;
   defrecvar[stypneg][vallower] = valmin;
   defrecvar[stypfre][vallower] = valmin;
   defrecvar[stypfre][valupper] = valpin;
   defrecvar[stypsos1][valupper] = valpin;
   defrecvar[stypsos2][valupper] = valpin;
   defrecvar[stypsemi][vallower] = 1.0;
   defrecvar[stypsemi][valupper] = valpin;
   defrecvar[stypsemiint][vallower] = 1.0;
   defrecvar[stypsemiint][valupper] = valiup;

   /*equations*/
   for( int e = ssyeque; e <= ssyequb; e++ )
   {
      for( int f = vallevel; f <= valupper; f++ )
         defrecequ[e][f] = 0.0;
      defrecequ[e][valscale] = 1.0;
   }

   defrecequ[ssyequg][valupper] = valpin;
   defrecequ[ssyequl][vallower] = valmin;
   defrecequ[ssyequn][vallower] = valmin;
   defrecequ[ssyequn][valupper] = valpin;
   defrecequ[ssyequc][valupper] = valpin;
}

static void initialization()
{
   for( int e = ssyeque; e <= ssyequb; e++ )
      equstypInfo[e - ssyeque] = e;
   InitDefaultRecords();
}

static void finalization()
{
}

UNIT_INIT_FINI();
}// namespace gdlib::gmsglob