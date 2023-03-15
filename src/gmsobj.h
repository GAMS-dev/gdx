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

   void Put( int Index, T *Item )
   {
      FreeItem( Index );
      FList[Index - ( OneBased ? 1 : 0 )] = Item;
   }

   void SetCount( int NewCount )
   {
      if( NewCount != FCount )
      {
         if( NewCount > FCapacity ) SetCapacity( NewCount );
         if( NewCount > FCount ) std::memset( &FList[FCount], 0, ( NewCount - FCount ) * sizeof( T * ) );
         else
            for( int i{ FCount - 1 }; i >= NewCount; i-- ) FreeItem( i );
         FCount = NewCount;
      }
   }

protected:
   int FCount;
   T **FList;

   virtual void Grow()
   {
      int delta{ FCapacity >= 1024 * 1024 ? FCapacity / 4 : ( !FCapacity ? 16 : 7 * FCapacity ) };
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
   TXList() : FCapacity{},
              FListMemory{},
              FCount{},
              FList{},
              OneBased{}
   {
   }

   virtual ~TXList()
   {
      Clear();
   }

   int Add( T *Item )
   {
      int res{ FCount };
      if( res == FCapacity ) Grow();
      FList[res] = Item;
      FCount++;
      if( OneBased ) res++;
      return res;
   }

   void Clear()
   {
      for( int N{ FCount - 1 + ( OneBased ? 1 : 0 ) }; N >= ( OneBased ? 1 : 0 ); N-- ) FreeItem( N );
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

   T *Extract( T *Item )
   {
      T *res{};
      int I{ IndexOf( Item ) };
      if( OneBased ) I--;
      if( I >= 0 )
      {
         res = Item;
         // Delete item, do not call FreeItem
         FCount--;
         if( I < FCount )// overlap so use memmove instead of memcpy
            std::memmove( &FList[I], &FList[I + 1], ( FCount - I ) * sizeof( T * ) );
      }
      return res;
   }

   int IndexOf( const T *Item ) const
   {
      for( int N{}; N < FCount; N++ )
         if( FList[N] == Item )
            return N + ( OneBased ? 1 : 0 );
      return -1;
   }

   void Insert( int Index, T *Item )
   {
      if( FCount == FCapacity ) Grow();
      if( OneBased ) Index--;
      if( Index < FCount )// overlap so use memmove instead of memcpy
         std::memmove( &FList[Index + 1], &FList[Index], ( FCount - Index ) * sizeof( T * ) );
      FList[Index] = Item;
      FCount++;
   }

   int Remove( const T *Item )
   {
      int res{ FCount - 1 };
      while( res >= 0 && FList[res] != Item ) res--;
      if( res >= ( OneBased ? 1 : 0 ) ) Delete( res );
      return res;
   }

   [[nodiscard]] int GetCapacity() const
   {
      return FCapacity;
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
      } else
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

   void Put( int Index, const char *Item, size_t itemLen )
   {
      FreeItem( Index );
      FList[Index - ( OneBased ? 1 : 0 )] = utils::NewString( Item, itemLen, FStrMemory );
   }

   char *Get( int Index )
   {
      return FList[Index - ( OneBased ? 1 : 0 )];
   }

protected:
   void FreeItem( int Index ) override
   {
      delete[] FList[Index];
   }

public:
   TXStrings() : FStrMemory{} {}

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
      for( int N{}; N < FCount; N++ )
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
   TBooleanBitArray() : PData{}, FAllocated{}, FHighIndex{ -1 }
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
         int NewMemSize{ ( V + 8 ) / 8 };
         if( NewMemSize > FAllocated )
         {
            int Delta{};
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

class TQuickSortClass
{
   void QuickSort( int L, int R );

public:
   virtual ~TQuickSortClass() = default;
   bool OneBased{};
   virtual void Exchange( int Index1, int Index2 ) = 0;
   virtual int Compare( int Index1, int Index2 ) = 0;
   void SortN( int n );
};

template<typename T>
struct TStringItem {
   char *FString;
   T *FObject;
};

template<typename T>
class TXCustomStringList : public TQuickSortClass
{
protected:
   int FCount{};
   TStringItem<T> *FList{};
   int FCapacity{};
   size_t FStrMemory{}, FListMemory{};

   void SetName( int Index, const char *v )
   {
      char **sref = FList[Index - ( OneBased ? 1 : 0 )]->FString;
      delete[] *sref;
      *sref = utils::NewString( v, std::strlen( v ), FStrMemory );
   }

public:
   char *GetName( int Index )
   {
      return FList[Index - ( OneBased ? 1 : 0 )].FString;
   }

   char *GetString( int Index )
   {
      return GetName( Index );
   }

   T *GetObject( int Index )
   {
      return FList[Index - ( OneBased ? 1 : 0 )].FObject;
   }

   void PutObject( int Index, T *AObject )
   {
      FList[Index - ( OneBased ? 1 : 0 )].FObject = AObject;
   }

   void SetCapacity( int NewCapacity )
   {
      if( NewCapacity == FCapacity ) return;
      if( NewCapacity < FCount ) NewCapacity = FCount;
      FListMemory = sizeof( TStringItem<T> ) * NewCapacity;
      if( !FList ) FList = (TStringItem<T> *) std::malloc( FListMemory );
      else if( !FListMemory )
      {
         std::free( FList );
         FList = nullptr;
      } else
         FList = (TStringItem<T> *) std::realloc( FList, FListMemory );
      FCapacity = NewCapacity;
   }

protected:
   virtual void Grow()
   {
      int delta{ FCapacity >= 1024 * 1024 ? FCapacity / 4 : ( !FCapacity ? 16 : 7 * FCapacity ) };
      int64_t i64{ FCapacity };
      i64 += delta;
      if( i64 <= std::numeric_limits<int>::max() )
         SetCapacity( (int) i64 );
      else
      {
         delta = std::numeric_limits<int>::max();
         if( FCapacity < delta ) SetCapacity( delta );
         else
            assert( i64 <= std::numeric_limits<int>::max() && "TXCustromStringList.grow(): max capacity reached" );
      }
   }

   void FreeObject( int Index )
   {
      // noop
   }

public:
   void InsertItem( int Index, const char *S, size_t slen, T *APointer )
   {
      if( FCount == FCapacity ) Grow();
      if( OneBased ) Index--;
      if( Index < FCount )// overlap so use memmove instead of memcpy
         std::memmove( &FList[Index + 1], &FList[Index], ( FCount - Index ) * sizeof( TStringItem<T> ) );
      FList[Index].FString = utils::NewString( S, slen, FStrMemory );
      FList[Index].FObject = APointer;
      FCount++;
   }

   ~TXCustomStringList() override
   {
      Clear();
   }

   void Delete( int Index )
   {
      FreeItem( Index );
      if( OneBased ) Index--;
      FCount--;
      if( Index < FCount )// overlap so use memmove instead of memcpy
         std::memmove( &FList[Index], &FList[Index + 1], ( FCount - Index ) * sizeof( TStringItem<T> ) );
   }

   void FreeItem( int Index )
   {
      delete[] FList[Index - ( OneBased ? 1 : 0 )].FString;
      FreeObject( Index );
   }

   virtual void Clear()
   {
      for( int N{ FCount - 1 + ( OneBased ? 1 : 0 ) }; N >= ( OneBased ? 1 : 0 ); N-- )
         FreeItem( N );
      FCount = 0;
      SetCapacity( 0 );
   }

   virtual int Add( const char *S, size_t slen )
   {
      return AddObject( S, slen, nullptr );
   }

   virtual int AddObject( const char *S, size_t slen, T *APointer )
   {
      int res{ FCount + ( OneBased ? 1 : 0 ) };
      InsertItem( res, S, slen, APointer );
      return res;
   }

   int IndexOf( const char *S )
   {
      for( int N{}; N < FCount; N++ )
         if( utils::sameTextPChar( FList[N].FString, S ) ) return N + ( OneBased ? 1 : 0 );
      return -1;
   }

   int IndexOfObject( const T &AObject )
   {
      for( int N{}; N < FCount; N++ )
         if( FList[N].FObject == AObject )
            return N + ( OneBased ? 1 : 0 );
      return -1;
   }

   [[nodiscard]] char *GetName( int Index ) const
   {
      return FList[Index].FString;
   }

   char *operator[]( int Index ) const
   {
      return GetName( Index );
   }

   void Exchange( int Index1, int Index2 ) override
   {
      if( OneBased )
      {
         Index1--;
         Index2--;
      }
      TStringItem<T> Item = FList[Index1];
      std::memcpy( &FList[Index1], &FList[Index2], sizeof( TStringItem<T> ) );
      std::memcpy( &FList[Index2], &Item, sizeof( TStringItem<T> ) );
   }

   int Compare( int Index1, int Index2 ) override
   {
      char *s1 = FList[Index1 - ( OneBased ? 1 : 0 )].FString;
      char *s2 = FList[Index2 - ( OneBased ? 1 : 0 )].FString;
      return utils::sameTextPChar( s1, s2 );
   }

   [[nodiscard]] int Count() const
   {
      return FCount;
   }

   [[nodiscard]] int size() const
   {
      return FCount;
   }

   [[nodiscard]] int GetCapacity() const
   {
      return FCapacity;
   }

   [[nodiscard]] size_t MemoryUsed() const
   {
      return FListMemory + FStrMemory;
   }
};

const char NON_EMPTY{ '=' };
const int HASHMULT = 31,
          HASHMULT_6 = 887503681,
          HASHMULT2 = 71,
          HASHMOD2 = 32;
const double HASH2_MAXFULLRATIO = 0.75,
             HASH2_NICEFULLRATIO = 0.55;
const int SCHASH_FACTOR_MAX = 13,
          SCHASH_FACTOR_MIN = 6;

const int SCHASHSIZE0 = 10007,
          SCHASHSIZE1 = 77317,
          SCHASHSIZE2 = 598363,
          SCHASHSIZE3 = 4631287,
          SCHASHSIZE4 = 35846143,
          SCHASHSIZE5 = 277449127,
          // SCHASHSIZE6 =2147483647;
        // we do not need it so big as maxint32: maxint32 / SCHASH_FACTOR_MIN is big enough
        SCHASHSIZE6 = 357913951;

const std::array<int, 7> schashSizes{
        SCHASHSIZE0, SCHASHSIZE1, SCHASHSIZE2, SCHASHSIZE3, SCHASHSIZE4, SCHASHSIZE5, SCHASHSIZE6 };

int getSCHashSize( int itemCount );

struct THashRecord {
   THashRecord *PNext;
   int RefNr;
};
using PHashRecord = THashRecord *;

template<typename T>
class TXHashedStringList : public TXCustomStringList<T>
{
protected:
   PHashRecord *pHashSC{};
   int hashCount{}, trigger{ -1 };
   size_t hashBytes{};

   virtual int compareEntry( const char *s, int EN )
   {
      auto p{ this->FList[EN].FString };
      return !p ? ( !( !s || s[0] == '\0' ) ? 1 : 0 ) : utils::sameTextPChar<true>( s, p );
   }

   void ClearHashList()
   {
      if( pHashSC )
      {
         for( int n{}; n < hashCount; n++ )
         {
            PHashRecord p1{ pHashSC[n] };
            pHashSC[n] = nullptr;
            while( p1 )
            {
               auto p2 = p1->PNext;
               std::free( p1 );
               p1 = p2;
            }
         }
         //int64_t nBytes = sizeof(PHashRecord) * hashCount;
         std::free( pHashSC );
         pHashSC = nullptr;
         hashCount = 0;
         trigger = -1;
         hashBytes = 0;
      }
   }

   void SetHashSize( int newCount )
   {
      int newSiz{ newCount >= trigger ? getSCHashSize( newCount ) : 0 };
      if( newSiz == hashCount ) return;// no bump made
      hashCount = newSiz;
      int64_t i64 = hashCount * SCHASH_FACTOR_MAX;
      i64 = std::min<int64_t>( std::numeric_limits<int>::max(), i64 );
      trigger = (int) i64;
      hashBytes = sizeof( PHashRecord ) * hashCount;
      pHashSC = static_cast<PHashRecord *>( std::malloc( hashBytes ) );
      std::memset( pHashSC, 0, hashBytes );
      for( int n{ this->OneBased ? 1 : 0 }; n <= this->FCount - 1 + ( this->OneBased ? 1 : 0 ); n++ )
      {
         auto name{ this->GetName( n ) };
         auto hv{ hashValue( name, std::strlen( name ) ) };
         auto PH = (PHashRecord) std::malloc( sizeof( THashRecord ) );
         PH->PNext = pHashSC[hv];
         PH->RefNr = n - ( this->OneBased ? 1 : 0 );
         pHashSC[hv] = PH;
      }
      hashBytes += sizeof( THashRecord ) * this->FCount;
   }

   virtual uint32_t hashValue( const char *s, size_t slen )
   {
      int64_t r{};
      int i{}, n{ (int) slen };
      while( i + 5 < n )
      {
         uint32_t t{ (uint32_t) utils::toupper( s[i++] ) };
         for( int j{}; j < 5; j++ )
            t = ( HASHMULT * t ) + (uint32_t) utils::toupper( s[i++] );
         r = ( HASHMULT_6 * r + t ) % hashCount;
      }
      while( i < n )
         r = ( HASHMULT * r + (uint32_t) utils::toupper( s[i++] ) ) % hashCount;
      return (uint32_t) r;
   }

public:
   virtual ~TXHashedStringList()
   {
      Clear();
   }

   void Clear() override
   {
      ClearHashList();
      TXCustomStringList<T>::Clear();
   }

   int AddObject( const char *s, size_t slen, T *APointer ) override
   {
      if( !pHashSC || this->FCount > trigger ) SetHashSize( this->FCount );
      auto hv{ hashValue( s, slen ) };
      PHashRecord PH;
      for( PH = pHashSC[hv]; PH && !compareEntry( s, PH->RefNr ); PH = PH->PNext )
         ;
      if( PH ) return PH->RefNr + ( this->OneBased ? 1 : 0 );
      else
      {
         int res{ this->FCount + ( this->OneBased ? 1 : 0 ) };
         TXCustomStringList<T>::InsertItem( res, s, slen, APointer );
         PH = (PHashRecord) std::malloc( sizeof( THashRecord ) );
         hashBytes += sizeof( THashRecord );
         PH->PNext = pHashSC[hv];
         PH->RefNr = res - ( this->OneBased ? 1 : 0 );
         pHashSC[hv] = PH;
         return res;
      }
   }

   int Add( const char *S, size_t slen ) override
   {
      return AddObject( S, slen, nullptr );
   }
};

template<typename T>
class TXStrPool : public TXHashedStringList<T>
{
   int compareEntry( const char *s, int EN ) override
   {
      auto p{ this->FList[EN].FString };
      return !p ? ( !( !s || s[0] == '\0' ) ? 1 : 0 ) : utils::sameTextPChar<false>( s, p );
   }

   uint32_t hashValue( const char *s, size_t slen ) override
   {
      int64_t r{};
      int i{}, n{ (int) slen };
      while( i + 5 < n )
      {
         uint32_t t{ (uint32_t) s[i++] };
         for( int j{}; j < 5; j++ )
            t = ( HASHMULT * t ) + (uint32_t) s[i++];
         r = ( HASHMULT_6 * r + t ) % this->hashCount;
      }
      while( i < n )
         r = ( HASHMULT * r + (uint32_t) s[i++] ) % this->hashCount;
      return (uint32_t) r;
   }

public:
   virtual ~TXStrPool() = default;

   int Compare( int Index1, int Index2 ) override
   {
      char *s1{ this->FList[Index1 - ( this->OneBased ? 1 : 0 )].FString },
              *s2{ this->FList[Index2 - ( this->OneBased ? 1 : 0 )].FString };
      return utils::sameTextPChar<false>( s1, s2 );
   }
};

}// namespace gdx::collections::gmsobj