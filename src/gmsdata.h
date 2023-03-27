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
const int BufSize = 1024 * 16;

struct TGADataBuffer {
   // filler is needed, so a buffer can start on 8 byte boundary
   int BytesUsed {}, filler {};
   std::array<uint8_t, BufSize> Buffer {};
};

template<typename T>
class TGrowArrayFxd
{
   TGADataBuffer **PBase {};// dynamic heap array of pointers
   TGADataBuffer *PCurrentBuf {};
   int BaseAllocated {}, BaseUsed { -1 }, FSize, FStoreFact;

protected:
   int64_t FCount {};

public:
   explicit TGrowArrayFxd() : FSize { sizeof( T ) },
                              FStoreFact { BufSize / FSize }
   {}

   explicit TGrowArrayFxd( int ASize ) : FSize { ASize }, FStoreFact { BufSize / FSize }
   {}

   virtual ~TGrowArrayFxd()
   {
      Clear();
   }

   void Clear()
   {
      while( BaseUsed >= 0 )
      {
         delete PBase[BaseUsed];
         BaseUsed--;
      }
      std::free( PBase );
      PBase = nullptr;
      BaseAllocated = 0;
      PCurrentBuf = nullptr;
      FCount = 0;
   }

   T *ReserveMem()
   {
      if( !PCurrentBuf || PCurrentBuf->BytesUsed + FSize > BufSize )
      {
         BaseUsed++;
         if( BaseUsed >= BaseAllocated )
         {
            if( !BaseAllocated ) BaseAllocated = 32;
            else
               BaseAllocated *= 2;
            size_t newByteCount { BaseAllocated * sizeof( uint8_t * ) };
            if( !PBase ) PBase = static_cast<TGADataBuffer **>( std::malloc( newByteCount ) );
            else
               PBase = static_cast<TGADataBuffer **>( std::realloc( PBase, newByteCount ) );
         }
         PCurrentBuf = new TGADataBuffer;
         assert( BaseUsed >= 0 );
         if( BaseUsed >= 0 )
            PBase[BaseUsed] = PCurrentBuf;
         PCurrentBuf->BytesUsed = 0;
      }
      auto res = (T *) &PCurrentBuf->Buffer[PCurrentBuf->BytesUsed];
      PCurrentBuf->BytesUsed += FSize;
      FCount++;
      return res;
   }

   T *ReserveAndClear()
   {
      T *res = ReserveMem();
      memset( res, 0, FSize );
      return res;
   }

   T *AddItem( const T *R )
   {
      auto *res = ReserveMem();
      std::memcpy( res, R, FSize );
      return res;
   }

   T *GetItemPtrIndex( int N )
   {
      return (T *) &PBase[N / FStoreFact]->Buffer[( N % FStoreFact ) * FSize];
   }

   [[nodiscard]] T *GetItemPtrIndexConst( int N ) const
   {
      return (T *) &PBase[N / FStoreFact]->Buffer[( N % FStoreFact ) * FSize];
   }

   void GetItem( int N, T **R )
   {
      T *PB = GetItemPtrIndex( N );
      std::memcpy( R, PB, FSize );
   }

   [[nodiscard]] int64_t MemoryUsed() const
   {
      return !PCurrentBuf ? 0 : (int64_t) ( BaseAllocated * sizeof( uint8_t * ) + BaseUsed * BufSize + PCurrentBuf->BytesUsed );
   }

   [[nodiscard]] int64_t GetCount() const
   {
      return FCount;
   }

   [[nodiscard]] int64_t size() const
   {
      return FCount;
   }

   T *operator[]( int N ) const
   {
      return GetItemPtrIndexConst( N );
   }
};

class TXIntList : public TGrowArrayFxd<int>
{
   [[nodiscard]] int &GetItems( int Index ) const
   {
      return *GetItemPtrIndexConst( Index );
   }

public:
   TXIntList() = default;
   ~TXIntList() override = default;

   int Add( int Item )
   {
      int res { (int) FCount };
      AddItem( &Item );
      return res;
   }

   void Exchange( int Index1, int Index2 )
   {
      int *p1 { GetItemPtrIndex( Index1 ) }, *p2 { GetItemPtrIndex( Index2 ) };
      int t { *p1 };
      *p1 = *p2;
      *p2 = t;
   }

   [[nodiscard]] int &operator[]( int Index ) const
   {
      return GetItems( Index );
   }

   int &operator[]( int Index )
   {
      while( Index >= FCount ) ReserveAndClear();
      return *GetItemPtrIndex( Index );
   }
};

template<typename T>
class TTblGamsData
{
   TGrowArrayFxd<uint8_t> DS;
   collections::gmsobj::TXList<uint8_t> FList {};
   int FDim, FIndexSize, FDataSize;
   bool FIsSorted { true };

   void QuickSort( int L, int R )
   {
      int i { L };
      while( i < R )
      {
         int j { R };
         int p { ( L + R ) >> 1 };
         auto pivot { reinterpret_cast<int *>( FList[p] ) };
         do {
            while( CompareWithRecPtr( i, pivot ) < 0 ) i++;
            while( CompareWithRecPtr( j, pivot ) > 0 ) j--;
            if( i < j )
               Exchange( i++, j-- );
            else if( i == j )
            {
               i++;
               j--;
            }
         } while( i <= j );
         if( ( j - L ) > ( R - i ) )
         {
            if( i < R ) QuickSort( i, R );
            i = L;
            R = j;
         }
         else
         {
            if( L < j ) QuickSort( L, j );
            L = i;
         }
      }
   }

   int Compare( int Index1, int Index2 )
   {
      const int *P1 { reinterpret_cast<const int *>( FList[Index1] ) },
              *P2 { reinterpret_cast<const int *>( FList[Index2] ) };
      for( int D {}; D < FDim; D++ )
      {
         int diff { P1[D] - P2[D] };
         if( diff ) return diff;
      }
      return 0;
   }

   int CompareWithRecPtr( int i1, const int *p2 )
   {
      auto P1 { reinterpret_cast<const int *>( FList[i1] ) };
      for( int k {}; k < FDim; k++ )
      {
         int diff { P1[k] - p2[k] };
         if( diff ) return diff;
      }
      return 0;
   }

   void Exchange( int Index1, int Index2 )
   {
      auto P { *FList[Index1] };
      *FList[Index1] = *FList[Index2];
      *FList[Index2] = P;
   }

   void InsertRecord( int N, const int *Inx, const T *Buffer )
   {
      auto P { DS.ReserveMem() };
      std::memcpy( P, Inx, FIndexSize );
      std::memcpy( &P[FIndexSize], Buffer, FDataSize );
      FList.Insert( N, P );
      FIsSorted = false;
   }

public:
   TTblGamsData( int ADim, int ADataSize ) : DS { static_cast<int>( ADim * sizeof( int ) + ADataSize ) },
                                             FDim { ADim },
                                             FIndexSize { static_cast<int>( FDim * sizeof( int ) ) },
                                             FDataSize { ADataSize }
   {
   }

   virtual ~TTblGamsData() = default;

   inline void AddRecord( const int *Inx, const T *Buffer )
   {
      InsertRecord( FList.size(), Inx, Buffer );
   }

   void GetRecord( int N, int *Inx, T *Buffer )
   {
      auto P { FList[N] };
      std::memcpy( Inx, P, FIndexSize );
      std::memcpy( Buffer, &P[FIndexSize], FDataSize );
   }

   void Sort()
   {
      if( !FIsSorted )
      {
         bool SortNeeded {};
         for( int N {}; N < FList.size() - 1; N++ )
         {
            if( Compare( N, N + 1 ) > 0 )
            {
               SortNeeded = true;
               break;
            }
         }
         if( SortNeeded ) QuickSort( 0, FList.size() - 1 );
         FIsSorted = true;
      }
   }

   void GetKeys( int N, int *Inx )
   {
      std::memcpy( Inx, FList[N], FIndexSize );
   }

   void GetData( int N, T *Buffer )
   {
      std::memcpy( Buffer, &FList[N][FIndexSize], FDataSize );
   }

   T *GetDataPtr( int N )
   {
      return reinterpret_cast<T *>( &FList[N][FIndexSize] );
   }

   void Clear()
   {
      DS.Clear();
      FList.Clear();
   }

   [[nodiscard]] int64_t MemoryUsed() const
   {
      return DS.MemoryUsed() + FList.MemoryUsed();
   }

   [[nodiscard]] int GetCount() const
   {
      return FList.size();
   }

   [[nodiscard]] int GetDimension() const
   {
      return FDim;
   }
};

template<typename T>
class TTblGamsDataCxx
{
   int FDim, FDataSize;
   std::vector<std::pair<int *, T *>> keyValues {};

public:
   TTblGamsDataCxx( int ADim, int ADataSize ) : FDim { ADim }, FDataSize { ADataSize } {}

   virtual ~TTblGamsDataCxx()
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
