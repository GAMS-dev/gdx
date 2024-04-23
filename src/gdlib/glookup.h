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

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include <cassert>
#include <limits>

#include "strutilx.h"
#include "utils.h"
#include "gmsonly.h"

#include "gmsheapnew.h"

namespace gdlib::glookup
{

inline int CalcNextHashSize( const int Cnt, int &Nxt )
{
   constexpr int
           HashSize_1 = 997, Next_1 = 1500,
           HashSize_2 = 9973, Next_2 = 15000,
           HashSize_3 = 99991, Next_3 = 150000,
           HashSize_4 = 999979, Next_4 = 1500000,
           HashSize_5 = 9999991, Next_5 = 15000000,
           HashSize_6 = 99999989, Next_6 = std::numeric_limits<int>::max();
   if( Cnt >= Next_4 )
   {
      Nxt = Next_5;
      return HashSize_5;
   }
   if( Cnt >= Next_3 )
   {
      Nxt = Next_4;
      return HashSize_4;
   }
   if( Cnt >= Next_2 )
   {
      Nxt = Next_3;
      return HashSize_3;
   }
   if( Cnt >= Next_1 )
   {
      Nxt = Next_2;
      return HashSize_2;
   }
   Nxt = Next_1;
   return HashSize_1;
}

template<typename T>
class TGAMSRecList
{
protected:
   uint32_t HashSize {};
   int ReHashCnt {}, FSrtCount {};
   // use bucket index (in rec list) instead of pointers since they may invalidate due to moving memory blocks
   // value=0 -> no bucket assigned
   // value >=1 -> bucket #(value-1)
   std::unique_ptr<std::vector<int>> HashTable {};
   std::vector<T *> RecList {};
   std::unique_ptr<std::vector<T *>> FSrtIndx {};

private:
   T *GetPtrForHash( const uint32_t hv )
   {
      const int n { ( *HashTable )[hv] };
      return !n ? nullptr : RecList[n - 1];
   }

   void SetPtrForHash( const uint32_t hv, T *nptr )
   {
      const int n { ( *HashTable )[hv] };
      if( n ) RecList[n - 1] = nptr;
   }

   T *GetPtrForStr( const std::string_view s )
   {
      return GetPtrForHash( Hash( s ) );
   }

   [[nodiscard]] uint32_t Hash( const std::string_view s ) const
   {
      uint32_t res { static_cast<uint32_t>( utils::toupper( static_cast<char>( s.length() ) ) ) };
      for( const char c: s )
         res = 23 * res + utils::toupper( c );
      return res % HashSize;
   }

   [[nodiscard]] uint32_t Hash( const gdlib::strutilx::DelphiStrRef &s ) const
   {
      uint32_t res { static_cast<uint32_t>( utils::toupper( static_cast<char>( s.length ) ) ) };
      for( int i {}; i < s.length; i++ )
         res = 23 * res + utils::toupper( s.chars[i] );
      return res % HashSize;
   }

   void HashTableReset( int ACnt )
   {
      HashSize = CalcNextHashSize( ACnt, ReHashCnt );
      if( !HashTable ) HashTable = std::make_unique<std::vector<int>>( HashSize );
      else if( HashTable->size() < HashSize )
         HashTable->resize( HashSize );
      std::fill_n( HashTable->begin(), HashSize, 0 );
   }

   void HashAll()
   {
      HashTableReset( static_cast<int>(RecList.size()) );
      for( int N {}; N < static_cast<int>( RecList.size() ); N++ )
      {
         T *PRec = RecList[N];
         auto [ps, nextBucketIx] = AccessRecord( PRec );
         auto HV = Hash( ps );
         *nextBucketIx = ( *HashTable )[HV];
         ( *HashTable )[HV] = N + 1;
      }
   }

protected:
   virtual std::pair<gdlib::strutilx::DelphiStrRef, int *> AccessRecord( T *prec ) = 0;

public:
   TGAMSRecList() = default;

   virtual ~TGAMSRecList()
   {
      for( auto elem: RecList )
         gmsonly::gheap.XFreeMemDestr( elem );
   }

   int AddItem( T *PRec )
   {
      if( !HashTable || static_cast<int>( RecList.size() ) > ReHashCnt )
         HashAll();
      RecList.push_back( PRec );
      const auto res { static_cast<int>( RecList.size() ) };
      auto [ps, nextBucketIndex] = AccessRecord( PRec );
      const uint32_t HV { Hash( ps ) };
      *nextBucketIndex = ( *HashTable )[HV];
      ( *HashTable )[HV] = res;
      return res;
   }

   void ClearHashTable()
   {
      HashTable = nullptr;
      HashSize = 0;
   }

   T *Find( const std::string_view s )
   {
      if( !HashTable ) HashAll();
      T *res;
      for( res = GetPtrForStr( s ); res; )
      {
         auto [ps, nextIx] = AccessRecord( res );
         if( gdlib::strutilx::StrUEqual( ps, s ) ) return res;
         res = !*nextIx ? nullptr : RecList[*nextIx - 1];
      }
      return res;
   }

   int StoreEntry( T *PRec )
   {
      assert( !HashTable && "StoreEntry" );
      RecList.push_back( PRec );
      return RecList.size();
   }

   int MemoryUsed() { return RecList.size() * sizeof( T ); }

   void ReNameEntry( T *PRec, const std::string_view OldName )
   {
      const uint32_t HV { Hash( OldName ) };
      T *PrP = GetPtrForHash( HV );
      while( PrP && PrP != PRec )
      {
         auto [ps, nextIx] = AccessRecord( PrP );
         PrP = !nextIx ? nullptr : RecList[*nextIx - 1];
      }
      if( PrP )
      {
         auto [newName, nextIx] = AccessRecord( PRec );
         const uint32_t HVNew = Hash( newName );
         const int oldIx { ( *HashTable )[HV] };
         *nextIx = ( *HashTable )[HVNew];
         ( *HashTable )[HVNew] = oldIx;
      }
   }

   void SortEntries()
   {
      T **PA;// local copy for speed

      // TODO: Use std::sort instead!
      std::function<void( int, int )> QuickSort = [&]( int L, int R ) {
         strutilx::DelphiStrRef pPivotString {};

         auto compare2Pivot = [&]( int i ) {
            auto [PStri, PDum] = AccessRecord( PA[i] );
            return StrUCmp( PStri, pPivotString );
         };

         /*auto debugSequence = [&](const std::string &pivotStr) {
            std::cout << "Sequence: ";
            for(int i{}; i<FSrtIndx->size(); i++) {
               auto s{AccessRecord((*FSrtIndx)[i]).first.str()};
               std::cout << (s == pivotStr ? "***" : "") << s << " ";
            }
            std::cout << std::endl;
         };*/

         int i { L };
         while( i < R )
         {
            int j { R };
            int p { ( L + R ) >> 1 };
            auto pair = AccessRecord( PA[p] );
            pPivotString = pair.first;
            do {
               //debugSequence(pPivotString.str());
               while( compare2Pivot( i ) < 0 ) i++;
               while( compare2Pivot( j ) > 0 ) j--;
               if( i < j )
               {
                  //std::cout << "Swapping " << AccessRecord(PA[i]).first.str() << " and " << AccessRecord(PA[j]).first.str() << std::endl;
                  T *Tptr = PA[i];
                  PA[i] = PA[j];
                  PA[j] = Tptr;
                  i++;
                  j--;
               }
               else if( i == j )
               {
                  i++;
                  j--;
               }
            } while( i <= j );
            // partition finished, now sort left and right, starting with the smallest
            if( ( j - L ) > ( R - i ) )
            {
               // left part is bigger, look right first
               if( i < R ) QuickSort( i, R );// if necessary, sort the right part
               i = L;                        // and move to the left part
               R = j;
            }
            else
            {                                // right part is bigger, look left first
               if( L < j ) QuickSort( L, j );// if necessary, sort the left part
               L = i;                        // and move to the right part
            }
         }
      };

      if( FSrtCount != static_cast<int>( RecList.size() ) )
      {
         if( FSrtCount > 0 ) FSrtIndx = nullptr;
         FSrtCount = RecList.size();
         FSrtIndx = std::make_unique<std::vector<T *>>( FSrtCount );
      }
      PA = FSrtIndx->data();// local copy for speed
      for( int N {}; N < static_cast<int>( RecList.size() ); N++ )
         ( *FSrtIndx )[N] = RecList[N];
      QuickSort( 0, static_cast<int>( RecList.size() ) - 1 );
   }

   void SortFinished()
   {
      if( FSrtCount )
      {
         FSrtIndx = nullptr;
         FSrtCount = 0;
      }
   }

   void DeleteLasts( int n )
   {
      for( int i {}; i < n; i++ )
      {
         gmsonly::gheap.XFreeMemDestr( RecList.back() );
         RecList.pop_back();
      }
      HashAll();
   }

   T *operator[]( const int n )
   {
      return GetEntryPtr( n );
   }

   T *GetEntryPtr( const int N )
   {
      return N >= 1 && N <= static_cast<int>( RecList.size() ) ? RecList[N - 1] : nullptr;
   }

   T *GetSortedEntryPtr( const int N )
   {
      if( FSrtCount != static_cast<int>( RecList.size() ) ) SortEntries();
      return N >= 1 && N <= FSrtCount ? ( *FSrtIndx )[N - 1] : nullptr;
   }

   [[nodiscard]] int GetCount() const
   {
      return RecList.size();
   }

   [[nodiscard]] int Count() const
   {
      return RecList.size();
   }
};

using TBaseArray = uint8_t *;
using PBaseArray = TBaseArray *;

template<typename t>
class TBucketPtrArray;

template<typename T>
class TBucketArray
{
   PBaseArray FBaseArray {};
   int FBucketSize { 0x80000 }, FCount {}, FCapacity {}, FBaseCnt {}, FBaseCap {}, FRecSize {}, FDivisor {};
   gdlib::gmsheapnew::THeapMgr &Fmyheap;

public:
   friend class TBucketPtrArray<T>;

   TBucketArray( gmsheapnew::THeapMgr &Amyheap, int ARecSize ) : FRecSize { ARecSize },
                                                                 FDivisor { FBucketSize / ARecSize },
                                                                 Fmyheap { Amyheap }
   {}

   TBucketArray( gmsheapnew::THeapMgr &Amyheap, int ARecSize, int ARecMult ) : TBucketArray( Amyheap, ARecSize )
   {
      if( ARecMult )
      {
         FBucketSize = ARecSize * ARecMult;
         FDivisor = ARecMult;
      }
   }

   virtual ~TBucketArray()
   {
      Clear();
      if( FBaseCap > 0 )
         Fmyheap.XReAllocMem( reinterpret_cast<void **>( &FBaseArray ), (int) ( FBaseCap * sizeof( T * ) ), 0 );
   }

   void Clear()
   {
      while( FBaseCnt > 0 )
      {
         FBaseCnt--;
         Fmyheap.XFreeMem( FBaseArray[FBaseCnt], FBucketSize );
      }
      FCount = FCapacity = 0;
   }

   T *AddItem()
   {
      if( FCount == FCapacity )
      {
         if( FCapacity > std::numeric_limits<int>::max() - FDivisor )
            throw std::runtime_error( "TBucketArray.AddItem: Capacity = " + std::to_string( FCapacity ) + ", Addition = " + std::to_string( FDivisor ) + ", Maximum = " + std::to_string( std::numeric_limits<int>::max() ) );

         FBaseCnt++;
         if( FBaseCnt >= FBaseCap )
         {
            const int OldCap { FBaseCap };
            FBaseCap = !FBaseCap ? 16 : 2 * FBaseCap;
            Fmyheap.XReAllocMem( reinterpret_cast<void **>( &FBaseArray ), OldCap * sizeof( T * ), FBaseCap * sizeof( T * ) );
         }
         FBaseArray[FBaseCnt - 1] = static_cast<uint8_t *>( Fmyheap.XGetMem( FBucketSize ) );
         FCapacity += FDivisor;
      }
      if( !FBaseArray || !FDivisor ) return nullptr;
      auto res { reinterpret_cast<T *>( &FBaseArray[FCount / FDivisor][( FCount % FDivisor ) * FRecSize] ) };
      FCount++;
      return res;
   }

   T *ItemPtr( int N )
   {
      return reinterpret_cast<T *>( &FBaseArray[N / FDivisor][N % FDivisor * FRecSize] );
   }

   void DeleteLast()
   {
      if( !FCount ) throw std::runtime_error( "TBucketArray: DeleteLast Count = 0" );
      FCount--;
   }

   [[nodiscard]] int MemoryUsed() const
   {
      return FBaseCap * sizeof( T * ) + FBaseCnt * FBucketSize;
   }

   void DeleteAtEnd( int Cnt )
   {
      if( Cnt > FCount )
      {
         std::string msg { "TBucketArray: DeleteAtEnd, Cnt = " };
         msg.append( std::to_string( Cnt ) );
         msg.append( ", Count = " );
         msg.append( std::to_string( FCount ) );
         throw std::runtime_error( msg );
      }
      FCount -= Cnt;
   }

   void ReduceSize( int NewCount )
   {
      if( NewCount < 0 || NewCount > FCount )
         throw std::runtime_error( "TBucketArray: ReduceSize NewCount = " + std::to_string( NewCount ) );
      FCount = NewCount;
   }

   [[nodiscard]] int GetCount() const { return FCount; }
   [[nodiscard]] int size() const { return FCount; }

   T *operator[]( int N ) { return ItemPtr( N ); }
};

template<typename T>
class TBucketPtrArray final : public TBucketArray<T>
{
public:
   explicit TBucketPtrArray( gmsheapnew::THeapMgr &Amyheap ) : TBucketArray<T>( Amyheap, sizeof( T * ) ) {}

   int AddItem( T *p )
   {
      *reinterpret_cast<T **>( TBucketArray<T>::AddItem() ) = p;
      return this->FCount - 1;
   }

   T *GetItem( int N )
   {
      return *reinterpret_cast<T **>( TBucketArray<T>::ItemPtr( N ) );
   }

   void SetItem( int N, T *p )
   {
      if( N >= this->FCount ) AddItem( p );
      else
         *reinterpret_cast<T **>( TBucketArray<T>::ItemPtr( N ) ) = p;
   }
};

template<typename T>
class TGAMSRecListLegacy
{
   std::unique_ptr<TBucketPtrArray<T>> HashTable {};
   uint32_t HashSize {};
   int ReHashCnt {};
   TBucketPtrArray<T> RecList;
   T **FSrtIndx {};
   int FSrtCount {};
   gmsheapnew::THeapMgr &Fmyheap;

   uint32_t Hash( const char *s )
   {
      uint32_t res {};
      for( int k {}; s[k] != '\0'; k++ )
         res = 23 * res + utils::toupper( s[k] );
      return res % HashSize;
   }

   void HashTableReset( int ACnt )
   {
      HashSize = CalcNextHashSize( ACnt, ReHashCnt );
      if( !HashTable ) HashTable = std::make_unique<TBucketPtrArray<T>>( Fmyheap );
      for( uint32_t N {}; N < HashSize; N++ )
         HashTable->SetItem( N, nullptr );
   }

   void HashAll()
   {
      HashTableReset( RecList.GetCount() );
      for( int N {}; N < RecList.GetCount(); N++ )
      {
         auto PRec { RecList.GetItem( N ) };
         auto [Ps, PBuck] = AccessRecord( PRec );
         auto HV { Hash( Ps ) };
         *PBuck = ( *HashTable )[HV];
         HashTable->SetItem( HV, PRec );
      }
   }

protected:
   virtual std::pair<char *, T **> AccessRecord( T *prec ) = 0;

public:
   explicit TGAMSRecListLegacy( gmsheapnew::THeapMgr &Amyheap ) : RecList { Amyheap }, Fmyheap { Amyheap } {}

   virtual ~TGAMSRecListLegacy()
   {
      SortFinished();
   }

   void ClearHashTable()
   {
      HashTable = nullptr;
      HashSize = 0;
   }

   T *Find( const char *s )
   {
      if( !HashTable ) HashAll();
      assert( HashTable );
      if( !HashTable ) return nullptr;
      for( T *res { ( *HashTable )[Hash( s )] }; res; )
      {
         auto [ps, pbuck] = AccessRecord( res );
         if( utils::sameTextPChar( s, ps ) ) return res;
         res = *pbuck;
      }
      return nullptr;
   }

   int StoreEntry( T *PRec )
   {
      assert( !HashTable && "Store Entry" );
      return RecList.AddItem( PRec ) + 1;
   }

   int AddItem( T *PRec )
   {
      if( !HashTable || RecList.GetCount() > ReHashCnt ) HashAll();
      auto res { RecList.AddItem( PRec ) + 1 };
      auto [ps, pbuck] = AccessRecord( PRec );
      auto HV { Hash( ps ) };
      assert( HashTable );
      if( HashTable )
      {
         *pbuck = ( *HashTable )[HV];
         HashTable->SetItem( HV, PRec );
      }
      return res;
   }

   [[nodiscard]] int MemoryUsed() const
   {
      // FIXME: Implement me!
      return 0;
   }

   void ReNameEntry( T *PRec, const char *OldName )
   {
      // FIXME: Impelment me!
   }

   void SortEntries()
   {
      // FIXME: Implement me!
   }

   void DeleteLasts( int n )
   {
      for( int i { 1 }; i <= n; i++ )
         RecList.DeleteLast();
   }

   void SortFinished()
   {
      if( FSrtCount )
      {
         Fmyheap.XFreeMem( FSrtIndx, FSrtCount * sizeof( T * ) );
         FSrtIndx = nullptr;
         FSrtCount = 0;
      }
   }

   [[nodiscard]] int GetCount() const { return RecList.GetCount(); }

   T *GetEntryPtr( int N )
   {
      if( N >= 1 && N <= RecList.GetCount() ) return RecList.GetItem( N - 1 );
      throw std::runtime_error( "TGAMSRecList: Index = " + std::to_string( N ) + ", Max = " + std::to_string( RecList.FCount ) );
   }

   T *GetSortedEntryPtr( int N )
   {
      if( FSrtCount != RecList.GetCount() ) SortEntries();
      if( N >= 1 && N <= FSrtCount ) return FSrtIndx[N - 1];
      throw std::runtime_error( "TGAMSRecList: Sorted index = " + std::to_string( N ) + ", Max = " + std::to_string( RecList.FCount ) );
   }
};

}// namespace gdlib::glookup
