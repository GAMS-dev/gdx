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

#include "gmsstrm.hpp"
#include "utils.hpp"
#include "datastorage.hpp"
#include "gmsdata.hpp"

#include <vector>
#include <string>
#include <cassert>
#include <numeric>

namespace gdlib::strhash
{
template<typename T>
struct THashBucket {
   char *StrP {};
   THashBucket *NextBucket {};
   int StrNr {};
   T Obj {};
};

template<typename T>
using PHashBucket = THashBucket<T> *;

template<typename T>
class TXStrHashList
{
protected:
#ifdef TSH_BATCH_ALLOCS
   batchalloc::BatchAllocator<960> batchAllocator;
   batchalloc::BatchAllocator<1024> batchStrAllocator;
#endif
   std::vector<PHashBucket<T>> Buckets {};                    // sorted by order of insertion, no gaps
   std::unique_ptr<std::vector<PHashBucket<T>>> PHashTable {};// sorted by hash value, with gaps
   std::unique_ptr<std::vector<int>> SortMap {};
   int HashTableSize {}, ReHashCnt {}, FCount {};
   bool FSorted {};

   void ClearHashTable()
   {
      PHashTable = nullptr;
      HashTableSize = ReHashCnt = 0;
   }

   void HashTableReset( int ACnt )
   {
      constexpr int
              HashSize_1 = 997,
              HashSize_2 = 9973,
              HashSize_3 = 99991,
              HashSize_4 = 999979,
              HashSize_5 = 9999991,
              HashSize_6 = 99999989,
              Next_1 = 1500,
              Next_2 = 15000,
              Next_3 = 150000,
              Next_4 = 1500000,
              Next_5 = 15000000,
              Next_6 = std::numeric_limits<int>::max();
      if( ACnt >= Next_5 )
      {
         HashTableSize = HashSize_6;
         ReHashCnt = Next_6;
      }
      else if( ACnt >= Next_4 )
      {
         HashTableSize = HashSize_5;
         ReHashCnt = Next_5;
      }
      else if( ACnt >= Next_3 )
      {
         HashTableSize = HashSize_4;
         ReHashCnt = Next_4;
      }
      else if( ACnt >= Next_2 )
      {
         HashTableSize = HashSize_3;
         ReHashCnt = Next_3;
      }
      else if( ACnt >= Next_1 )
      {
         HashTableSize = HashSize_2;
         ReHashCnt = Next_2;
      }
      else
      {
         HashTableSize = HashSize_1;
         ReHashCnt = Next_1;
      }
      PHashTable = std::make_unique<std::vector<PHashBucket<T>>>( HashTableSize );
      // should be redundant due to std::vector being zero/value-initialized
      std::fill_n( PHashTable->begin(), HashTableSize, nullptr );
   }

   virtual int Hash( const char *s )
   {
      assert( HashTableSize > 0 );
      unsigned int res {};
      for( int i {}; s[i] != '\0'; i++ )
         res = 211 * res + utils::toupper( s[i] );
      return ( res & 0x7FFFFFFF ) % HashTableSize;
   }

   virtual bool EntryEqual( const char *ps1, const char *ps2 )
   {
      return utils::sameTextPChar( ps1, ps2 );
   }

   virtual int Compare( const char *ps1, const char *ps2 )
   {
      return utils::strCompare( ps1, ps2 );
   }

   void HashAll()
   {
      if( PHashTable ) PHashTable->clear();
      HashTableReset( FCount );
      for( int N {}; N < FCount; N++ )
      {
         auto &PBuck = Buckets[N];
         int HV = Hash( PBuck->StrP );
         PBuck->NextBucket = GetBucketByHash( HV );
         assert( PHashTable );
         ( *PHashTable )[HV] = PBuck;
      }
   }

   void SetObject( int N, T AObj )
   {
      Buckets[N - ( OneBased ? 1 : 0 )]->Obj = AObj;
   }

   void SetSortedObject( int N, T &AObj )
   {
      if( !FSorted ) Sort();
      Buckets[( *SortMap )[N - ( OneBased ? 1 : 0 )]]->Obj = &AObj;
   }

   char *GetSortedString( int N )
   {
      if( FSorted ) Sort();
      return Buckets[( *SortMap )[N - ( OneBased ? 1 : 0 )]]->StrP;
   }

   void QuickSort( int L, int R )
   {
      int i { L };
      while( i < R )
      {
         int j { R }, p { ( L + R ) >> 1 };
         char *pPivotStr = Buckets[( *SortMap )[p]]->StrP;
         do {
            while( Compare( Buckets[( *SortMap )[i]]->StrP, pPivotStr ) < 0 ) i++;
            while( Compare( Buckets[( *SortMap )[j]]->StrP, pPivotStr ) > 0 ) j--;
            if( i < j )
            {
               std::swap( ( *SortMap )[i], ( *SortMap )[j] );
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
         if( j - L > R - i )
         {                                // left part is bigger, look right first
            if( i < R ) QuickSort( i, R );// if necessary, sort the right part
            i = L;                        // and move to the left part
            R = j;
         }
         else
         {                                // right part is bigger, look left first
            if( L < j ) QuickSort( L, j );// if necessary, sort the right part
            L = i;
         }
      }
   }

   void Sort()
   {
      if( !SortMap )
      {
         SortMap = std::make_unique<std::vector<int>>( FCount );
         std::iota( SortMap->begin(), SortMap->end(), 0 );
         FSorted = false;
      }
      if( !FSorted )
      {
         if( FCount >= 2 )
         {
            const char *PSN = Buckets[0]->StrP;
            for( int N {}; N < FCount - 1; N++ )
            {
               const char *PSN1 = Buckets[N + 1]->StrP;
               if( Compare( PSN, PSN1 ) > 0 )
               {
                  QuickSort( 0, FCount - 1 );
                  break;
               }
               PSN = PSN1;
            }
         }
         FSorted = true;
      }
   }

public:
   bool OneBased {};// When false (default) indices are in the range 0..Count-1
   // when true, indices are in the range 1..Count

   TXStrHashList()
   {
      ClearHashTable();
   }

   virtual ~TXStrHashList()
   {
      Clear();
   }

   void Clear()
   {
#ifndef TSH_BATCH_ALLOCS
      for( auto bucket: Buckets )
      {
         delete[] bucket->StrP;
         delete bucket;
      }
#else
      batchAllocator.clear();
      batchStrAllocator.clear();
#endif
      Buckets.clear();
      FCount = 0;
      ClearHashTable();
      SortMap = nullptr;
      FSorted = false;
   }

   [[nodiscard]] T *GetObject( int N ) const
   {
      return &Buckets[N - ( OneBased ? 1 : 0 )]->Obj;
   }

   inline PHashBucket<T> GetBucketByHash( int hash )
   {
      return ( *PHashTable )[hash];
   }

   int StoreObject( const char *s, size_t slen, T AObj )
   {
      if( PHashTable ) ClearHashTable();
#ifdef TSH_BATCH_ALLOCS
      auto PBuck = reinterpret_cast<PHashBucket<T>>( batchAllocator.GetBytes( sizeof( THashBucket<T> ) ) );
#else
      PHashBucket<T> PBuck = new THashBucket<T> {};
#endif
      Buckets.push_back( PBuck );
      PBuck->NextBucket = nullptr;
      PBuck->StrNr = FCount;// before it was added!
      int res { FCount + ( OneBased ? 1 : 0 ) };
      if( SortMap )
      {
         ( *SortMap )[FCount] = FCount;
         FSorted = false;
      }
      FCount++;// ugly
#ifdef TSH_BATCH_ALLOCS
      PBuck->StrP = reinterpret_cast<char *>( batchStrAllocator.GetBytes( slen + 1 ) );
#else
      PBuck->StrP = new char[slen + 1];
#endif
      utils::assignPCharToBuf( s, (int) slen, PBuck->StrP, (int) slen + 1 );
      PBuck->Obj = std::move( AObj );
      return res;
   }

   int AddObject( const char *s, size_t slen, T AObj )
   {
      assert( FCount < std::numeric_limits<int>::max() );
      if( FCount >= ReHashCnt ) HashAll();
      int HV { Hash( s ) };
      PHashBucket<T> PBuck = GetBucketByHash( HV );
      while( PBuck )
      {
         if( !EntryEqual( PBuck->StrP, s ) ) PBuck = PBuck->NextBucket;
         else
            return PBuck->StrNr + ( OneBased ? 1 : 0 );
      }
#ifdef TSH_BATCH_ALLOCS
      PBuck = reinterpret_cast<PHashBucket<T>>( batchAllocator.GetBytes( sizeof( THashBucket<T> ) ) );
#else
      PBuck = new THashBucket<T> {};
#endif
      Buckets.push_back( PBuck );
      PBuck->NextBucket = GetBucketByHash( HV );
      ( *PHashTable )[HV] = PBuck;
      PBuck->StrNr = FCount;// before it was added! zero based
      int res { FCount + ( OneBased ? 1 : 0 ) };
      if( SortMap )
      {
         ( *SortMap )[FCount] = FCount;
         FSorted = false;
      }
      FCount++;// ugly
#ifdef TSH_BATCH_ALLOCS
      PBuck->StrP = reinterpret_cast<char *>( batchStrAllocator.GetBytes( slen + 1 ) );
#else
      PBuck->StrP = new char[slen + 1];
#endif
      utils::assignPCharToBuf( s, slen, PBuck->StrP, slen + 1 );
      PBuck->Obj = std::move( AObj );
      return res;
   }

   virtual void FreeItem( int N )
   {
      // noop by default
   }

   int Add( const char *s, size_t slen )
   {
      return AddObject( s, slen, nullptr );
   }

   int IndexOf( const char *s )
   {
      if( !PHashTable ) HashAll();
      int HV { Hash( s ) };
      PHashBucket<T> PBuck = GetBucketByHash( HV );
      while( PBuck )
      {
         if( !EntryEqual( PBuck->StrP, s ) ) PBuck = PBuck->NextBucket;
         else
            return PBuck->StrNr + ( OneBased ? 1 : 0 );
      }
      return -1;
   }

   template<typename T2>
   void LoadFromStream( T2 &s )
   {
      Clear();
      int Cnt { s.ReadInteger() };
      for( int N {}; N < Cnt; N++ )
         StoreObject( s.ReadString(), nullptr );
   }

   template<typename T2>
   void SaveToStream( T2 &s )
   {
      s.WriteInteger( FCount );
      for( int N { OneBased ? 1 : 0 }; N < FCount + ( OneBased ? 1 : 0 ); N++ )
         s.WriteString( GetString( N ) );
   }

   [[nodiscard]] int GetStringLength( int N ) const
   {
      return std::strlen( GetString( N ) );
   }

   [[nodiscard]] int64_t MemoryUsed() const
   {
      int64_t res {};
      for( int N {}; N < Count(); N++ )
         res += std::strlen( Buckets[N]->StrP ) + 1;
      res += static_cast<int>( Buckets.size() * sizeof( THashBucket<T> ) );
      if( PHashTable ) res += static_cast<int>( PHashTable->size() * sizeof( THashBucket<T> ) );
      if( SortMap ) res += static_cast<int>( SortMap->size() * sizeof( int ) );
      return res;
   }

   void RenameEntry( int N, const char *s )
   {
      N -= OneBased ? 1 : 0;
      if( FSorted )
      {
         SortMap = nullptr;
         FSorted = false;
      }
      if( PHashTable )
      {
         int HV0 { Hash( GetString( N + 1 ) ) }, HV1 { Hash( s ) };
         if( HV0 != HV1 )
         {
            PHashBucket<T> PrevBuck {}, PBuck;
            for( PBuck = GetBucketByHash( HV0 );
                 PBuck->StrNr != N; PBuck = PBuck->NextBucket )
               PrevBuck = PBuck;
            if( !PrevBuck ) ( *PHashTable )[HV0] = PBuck->NextBucket;
            else
               PrevBuck->NextBucket = PBuck->NextBucket;
            PBuck->NextBucket = GetBucketByHash( HV1 );
            ( *PHashTable )[HV1] = PBuck;
         }
      }
      SetString( N + 1, s, std::strlen( s ) );
   }

   T *operator[]( int N )
   {
      return GetObject( N );
   }

   T &operator[]( const char *key )
   {
      return *GetObject( IndexOf( key ) );
   }

   T *GetSortedObject( int N )
   {
      if( !FSorted ) Sort();
      return Buckets[( *SortMap )[N - ( OneBased ? 1 : 0 )]].Obj;
   }

   [[nodiscard]] char *GetString( int N ) const
   {
      return Buckets[N - ( OneBased ? 1 : 0 )]->StrP;
   }

   [[nodiscard]] char *GetName( int N ) const
   {
      return GetString( N );
   }

   void SetString( int N, const char *s, size_t slen )
   {
      auto bucket = Buckets[N - ( OneBased ? 1 : 0 )];
#ifdef TSH_BATCH_ALLOCS
      // Storage for old string will leak temporarily but will be collected by batchAllocator.clear call
      bucket->StrP = reinterpret_cast<char *>( batchStrAllocator.GetBytes( slen + 1 ) );
#else
      delete[] bucket->StrP;
      bucket->StrP = new char[slen + 1];
#endif
      utils::assignPCharToBuf( s, slen, bucket->StrP );
   }

   [[nodiscard]] char *GetSortedString( int N ) const
   {
      if( !FSorted ) Sort();
      return Buckets[( *SortMap )[N - ( OneBased ? 1 : 0 )]]->StrP;
   }

   [[nodiscard]] int Count() const
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

   void SetCapacity( int n )
   {
      Buckets.reserve( n );
   }

   int GetCapacity()
   {
      return (int) Buckets.capacity();
   }
};

// Specialization when it is not a pointer type
template<>
inline int TXStrHashList<uint8_t>::Add( const char *s, size_t slen )
{
   return AddObject( s, slen, 0 );
}

template<>
inline int TXStrHashList<int>::Add( const char *s, size_t slen )
{
   return AddObject( s, slen, 0 );
}

template<>
template<typename T2>
inline void TXStrHashList<uint8_t>::LoadFromStream( T2 &s )
{
   Clear();
   int Cnt { s.ReadInteger() };
   for( int N {}; N < Cnt; N++ )
   {
      auto str { s.ReadString() };
      StoreObject( str.c_str(), str.length(), 0 );
   }
}

template<>
template<typename T2>
inline void TXStrHashList<int>::LoadFromStream( T2 &s )
{
   Clear();
   int Cnt { s.ReadInteger() };
   for( int N {}; N < Cnt; N++ )
   {
      auto str { s.ReadString() };
      StoreObject( str.c_str(), str.length(), 0 );
   }
}

template<typename T>
class TXCSStrHashList : public TXStrHashList<T>
{
protected:
   int Hash( const char *s ) override
   {
      assert( this->HashTableSize > 0 );
      unsigned int res {};
      for( int i {}; s[i] != '\0'; i++ )
         res = 211 * res + s[i];
      return ( res & 0x7FFFFFFF ) % this->HashTableSize;
   }

   bool EntryEqual( const char *ps1, const char *ps2 ) override
   {
      return utils::sameTextPChar<false>( ps1, ps2 );
   }

   int Compare( const char *ps1, const char *ps2 ) override
   {
      return utils::strCompare( ps1, ps2, false );
   }
};

}// namespace gdlib::strhash
