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


#include "gmsobj.h"

namespace gdlib::gmsobj
{

void TQuickSortClass::SortN( int n )
{
   if( n > 1 )
      QuickSort( ( OneBased ? 1 : 0 ), n - 1 + ( OneBased ? 1 : 0 ) );
}

void TQuickSortClass::QuickSort( int L, int R )
{
   int i { L };
   while( i < R )
   {
      int j { R }, p { ( L + R ) >> 1 };
      do {
         while( Compare( i, p ) < 0 ) i++;
         while( Compare( j, p ) > 0 ) j--;
         if( i < j )
         {
            Exchange( i, j );
            if( p == i ) p = j;
            else if( p == j )
               p = i;
            i++;
            j--;
         }
         else if( i == j )
         {
            i++;
            j--;
         }
      } while( i <= j );
      // partition finished, now sort left and right
      // starting with the smaller piece to keep recursion in check
      if( ( j - L ) > ( R - i ) )
      {                                // left part is bigger, look right first
         if( i < R ) QuickSort( i, R );// sort the right part if necessary
         i = L;                        // move to the left part
         R = j;
      }
      else
      {                                // right part is bigger, look left first
         if( L < j ) QuickSort( L, j );// sort the right part if necessary
         L = i;
      }
   }
}

int getSCHashSize( int itemCount )
{
   int k { itemCount / SCHASH_FACTOR_MIN };
   int res { SCHASHSIZE0 };
   for( int i { 5 }; i >= 0; i-- )
      if( k >= schashSizes[i] )
         res = schashSizes[i + 1];
   return res;
}
}// namespace gdlib::gmsobj