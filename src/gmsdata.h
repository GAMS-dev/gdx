/*
 * GAMS - General Algebraic Modeling System GDX API
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

#pragma once

#include "gmsobj.h"// for TXList
#include <array>   // for array
#include <cassert> // for assert
#include <cstdint> // for uint8_t, int64_t
#include <cstdlib> // for free, malloc, realloc, size_t
#include <cstring> // for memcpy, memset
#include <iterator>// for pair
#include <vector>  // for vector

namespace gdx::collections::gmsdata
{
template<typename T>
class TTblGamsData
{
   int FDim, FDataSize;
   std::vector<std::pair<int *, T *>> keyValues {};

public:
   TTblGamsData( int ADim, int ADataSize ) : FDim { ADim }, FDataSize { ADataSize } {}

   virtual ~TTblGamsData()
   {
      Clear();
   }

   void GetRecord( int N, int *Inx, T *Vals )
   {
      std::memcpy( Inx, keyValues[N].first, FDim * sizeof( int ) );
      std::memcpy( Vals, keyValues[N].second, FDataSize );
   }

   void GetKeys( int N, int *Inx )
   {
      std::memcpy( Inx, keyValues[N].first, FDim * sizeof( int ) );
   }

   void GetData( int N, T *Vals )
   {
      std::memcpy( Vals, keyValues[N].second, FDataSize );
   }
   double *GetDataPtr( int N )
   {
      return keyValues[N].second;
   }

   void AddRecord( const int *AElements, const T *AVals )
   {
      // NOTE: Maybe use batch allocator vs. single new allocs here for performance?
      auto vals { new T[(size_t) FDataSize / sizeof( T )] };
      std::memcpy( vals, AVals, FDataSize );
      auto keys { new int[FDim] };
      std::memcpy( keys, AElements, FDim * sizeof( int ) );
      keyValues.emplace_back( keys, vals );
   }

   void Clear()
   {
      for( auto [k, v]: keyValues )
      {
         delete[] k;
         delete[] v;
      }
      keyValues.clear();
   }

   [[nodiscard]] int size() const
   {
      return (int) keyValues.size();
   }

   [[nodiscard]] int GetCount() const
   {
      return size();
   }

   [[nodiscard]] bool empty() const
   {
      return keyValues.empty();
   }

   void Sort()
   {
      std::sort( keyValues.begin(), keyValues.end(), [this]( const auto &pair1, const auto &pair2 ) {
         for( int i = 0; i < FDim; i++ )
            if( pair1.first[i] >= pair2.first[i] ) return false;
         return true;
      } );
   }

   [[nodiscard]] int MemoryUsed() const
   {
      return static_cast<int>( keyValues.size() * ( FDim * sizeof( int ) + FDataSize * sizeof( T ) ) + keyValues.capacity() );
   }

   [[nodiscard]] int GetDimension() const
   {
      return FDim;
   }
};

}// namespace gdx::collections::gmsdata
