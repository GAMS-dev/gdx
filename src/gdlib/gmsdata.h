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


#pragma once

#include <map>
#include <string>
#include <array>
#include <vector>

#include "gmsobj.h"

namespace gdlib::gmsdata
{
template<typename T>
class TTblGamsData final
{
   int FDim, FDataSize;
   std::vector<std::pair<int *, T *>> keyValues {};

public:
   TTblGamsData( const int ADim, const int ADataSize ) : FDim { ADim }, FDataSize { ADataSize } {}

   ~TTblGamsData()
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
      auto vals { new T[static_cast<size_t>( FDataSize ) / sizeof( T )] };
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
      return static_cast<int>( keyValues.size() );
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
            if( pair1.first[i] < pair2.first[i] )
               return true;
            else if( pair1.first[i] > pair2.first[i] )
               return false;
         return false;
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

constexpr int BufSize = 1024 * 16;

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

   explicit TGrowArrayFxd( const int ASize ) : FSize { ASize }, FStoreFact { BufSize / FSize }
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
            const size_t newByteCount { BaseAllocated * sizeof( uint8_t * ) };
            if( !PBase ) PBase = static_cast<TGADataBuffer **>( std::malloc( newByteCount ) );
            else
            {
               const auto PBaseRA = static_cast<TGADataBuffer **>( std::realloc( PBase, newByteCount ) );
               if( !PBaseRA && PBase )
                  std::free( PBase );
               PBase = PBaseRA;
            }
            assert( PBase );
         }
         PCurrentBuf = new TGADataBuffer;
         assert( BaseUsed >= 0 );
         if( BaseUsed >= 0 )
            PBase[BaseUsed] = PCurrentBuf;
         PCurrentBuf->BytesUsed = 0;
      }
      const auto res = reinterpret_cast<T *>( &PCurrentBuf->Buffer[PCurrentBuf->BytesUsed] );
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

   T *GetItemPtrIndex( const int N )
   {
      return reinterpret_cast<T *>( &PBase[N / FStoreFact]->Buffer[( N % FStoreFact ) * FSize] );
   }

   [[nodiscard]] T *GetItemPtrIndexConst( const int N ) const
   {
      return reinterpret_cast<T *>( &PBase[N / FStoreFact]->Buffer[( N % FStoreFact ) * FSize] );
   }

   void GetItem( int N, T **R )
   {
      const auto PB { GetItemPtrIndex( N ) };
      std::memcpy( R, PB, FSize );
   }

   [[nodiscard]] int64_t MemoryUsed() const
   {
      return !PCurrentBuf ? 0 : static_cast<int64_t>( BaseAllocated * sizeof( uint8_t * ) + BaseUsed * BufSize + PCurrentBuf->BytesUsed );
   }

   [[nodiscard]] int64_t GetCount() const
   {
      return FCount;
   }

   [[nodiscard]] int64_t size() const
   {
      return FCount;
   }

   T *operator[]( const int N ) const
   {
      return GetItemPtrIndexConst( N );
   }
};

class TXIntList final : public TGrowArrayFxd<int>
{
   [[nodiscard]] int &GetItems( int Index ) const
   {
      return *GetItemPtrIndexConst( Index );
   }

   void SetItems( int Index, int V )
   {
      while( Index >= FCount ) ReserveAndClear();
      *GetItemPtrIndex( Index ) = V;
   }

public:
   TXIntList() = default;
   ~TXIntList() override = default;

   int Add( const int Item )
   {
      const int res { static_cast<int>( FCount ) };
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

   int &operator[]( const int Index ) const
   {
      return GetItems( Index );
   }

   int &operator[]( const int Index )
   {
      while( Index >= FCount ) ReserveAndClear();
      return *GetItemPtrIndex( Index );
   }
};

}// namespace gdlib::gmsdata
