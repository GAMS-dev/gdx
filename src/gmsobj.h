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

#include "utils.h"  // for NewString, sameTextPChar, toupper
#include <algorithm>// for min
#include <array>    // for array
#include <cassert>  // for assert
#include <cstdint>  // for uint8_t, uint32_t, int64_t
#include <cstdlib>  // for malloc, free, size_t, realloc
#include <cstring>  // for memmove, memcpy, memset, strlen
#include <limits>   // for numeric_limits

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace gdx::collections::gmsobj
{

template<typename T>
class TXList
{
   int FCapacity;
   size_t FListMemory;

   T *Get( int Index )
   {
      return FList[Index - ( OneBased ? 1 : 0 )];
   }

protected:
   int FCount;
   T **FList;

   virtual void Grow()
   {
      int delta { FCapacity >= 1024 * 1024 ? FCapacity / 4 : ( !FCapacity ? 16 : 7 * FCapacity ) };
      int64_t i64 = FCapacity;
      i64 += delta;
      if( i64 <= std::numeric_limits<int>::max() ) SetCapacity( (int) i64 );
      else
      {
         delta = std::numeric_limits<int>::max();
         if( FCapacity < delta ) SetCapacity( delta );
         else
            assert( i64 <= std::numeric_limits<int>::max() && "TXList.grow(): max capacity reached" );
      }
   }

   virtual void FreeItem( int Index )
   {
      // No-op
   }

   bool OneBased;

public:
   TXList() : FCapacity {},
              FListMemory {},
              FCount {},
              FList {},
              OneBased {}
   {
   }

   virtual ~TXList()
   {
      Clear();
   }

   int Add( T *Item )
   {
      int res { FCount };
      if( res == FCapacity ) Grow();
      assert(FList);
      FList[res] = Item;
      FCount++;
      if( OneBased ) res++;
      return res;
   }

   void Clear()
   {
      for( int N { FCount - 1 + ( OneBased ? 1 : 0 ) }; N >= ( OneBased ? 1 : 0 ); N-- ) FreeItem( N );
      FCount = 0;
      SetCapacity( 0 );
   }

   void Delete( int Index )
   {
      FreeItem( Index );
      FCount--;
      if( Index < FCount )
      {
         if( OneBased ) Index--;
         // overlap so use memmove instead of memcpy
         std::memmove( &FList[Index], &FList[Index + 1], ( FCount - Index ) * sizeof( T * ) );
      }
   }

   void Insert( int Index, T *Item )
   {
      if( FCount == FCapacity ) Grow();
      if( OneBased ) Index--;
      assert(FList);
      // overlap so use memmove instead of memcpy
      if( Index < FCount )
         std::memmove( &FList[Index + 1], &FList[Index], ( FCount - Index ) * sizeof( T * ) );
      FList[Index] = Item;
      FCount++;
   }

   int Remove( const T *Item )
   {
      int res { FCount - 1 };
      while( res >= 0 && FList[res] != Item ) res--;
      if( res >= ( OneBased ? 1 : 0 ) ) Delete( res );
      return res;
   }

   void SetCapacity( int NewCapacity )
   {
      if( NewCapacity == FCapacity ) return;
      else if( NewCapacity < FCount )
         NewCapacity = FCount;
      FListMemory = sizeof( T * ) * NewCapacity;
      if( !FList ) FList = static_cast<T **>( std::malloc( FListMemory ) );
      else if( !NewCapacity )
      {
         std::free( FList );
         FList = nullptr;
      }
      else
         FList = static_cast<T **>( std::realloc( FList, FListMemory ) );
      FCapacity = NewCapacity;
   }

   [[nodiscard]] int GetCount() const
   {
      return FCount;
   }

   [[nodiscard]] int size() const
   {
      return FCount;
   }

   [[nodiscard]] bool empty() const
   {
      return !FCount;
   }

   virtual T *operator[]( int Index )
   {
      return Get( Index );
   }

   [[nodiscard]] const T *GetConst( int Index ) const
   {
      return FList[Index - ( OneBased ? 1 : 0 )];
   }

   T *GetLast()
   {
      return FCount <= 0 ? nullptr : FList[FCount - 1];
   }

   [[nodiscard]] size_t MemoryUsed() const
   {
      return FListMemory;
   }
};

class TXStrings : public TXList<char>
{
private:
   size_t FStrMemory;

protected:
   void FreeItem( int Index ) override
   {
      delete[] FList[Index];
   }

public:
   TXStrings() : FStrMemory {} {}

   ~TXStrings() override
   {
      Clear();
   }

   int Add( const char *Item, size_t ItemLen )
   {
      return TXList<char>::Add( utils::NewString( Item, ItemLen, FStrMemory ) );
   }

   int IndexOf( const char *Item )
   {
      for( int N {}; N < FCount; N++ )
         if( utils::sameTextPChar( FList[N], Item ) )
            return N + ( OneBased ? 1 : 0 );
      return -1;
   }
};

class TBooleanBitArray
{
   uint8_t *PData;
   int FAllocated, FHighIndex;

   static void GetBitMask( int V, int &N, uint8_t &M )
   {
      N = V >> 3;
      M = 1 << ( V & 0x7 );
   }

public:
   TBooleanBitArray() : PData {}, FAllocated {}, FHighIndex { -1 }
   {
   }

   ~TBooleanBitArray()
   {
      if( FAllocated > 0 )
         delete[] PData;
   }

   [[nodiscard]] bool GetBit( int N ) const
   {
      if( N < 0 || N > FHighIndex ) return false;
      int P;
      uint8_t M;
      GetBitMask( N, P, M );
      return PData[P] & M;
   }

   void SetHighIndex( int V )
   {
      if( V > FHighIndex )
      {
         int NewMemSize { ( V + 8 ) / 8 };
         if( NewMemSize > FAllocated )
         {
            int Delta {};
            do {
               if( !FAllocated ) Delta += 256;
               else if( FAllocated < 32 * 256 )
                  Delta += FAllocated;
               else
                  Delta += FAllocated / 4;
            } while( NewMemSize >= FAllocated + Delta );
            NewMemSize = FAllocated + Delta;
            auto NewMem = new uint8_t[NewMemSize];
            memset( NewMem, 0, NewMemSize );
            if( FAllocated )
            {
               std::memcpy( NewMem, PData, FAllocated );
               delete[] PData;
            }
            PData = NewMem;
            FAllocated = NewMemSize;
         }
         FHighIndex = V;
      }
   }

   [[nodiscard]] int GetHighIndex() const
   {
      return FHighIndex;
   }

   void SetBit( int N, bool V )
   {
      if( N >= 0 )
      {
         if( N > FHighIndex )
         {
            if( !V ) return;
            SetHighIndex( N );
         }
         int P;
         uint8_t M;
         GetBitMask( N, P, M );
         if( V ) PData[P] |= M;
         else
            PData[P] &= !M;
      }
   }

   [[nodiscard]] int MemoryUsed() const
   {
      return FAllocated;
   }
};

}// namespace gdx::collections::gmsobj
