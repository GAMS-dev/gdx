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

#include "gmsheapnew.h"
#include <cassert> // for assert
#include <cstdint> // for uint8_t
#include <cstring> // for memcpy, memset, size_t
#include <limits>  // for numeric_limits
#include <optional>// for nullopt, optional
namespace gdx::collections::datastorage
{
template<typename KeyType, typename ValueType>
struct TLinkedDataRec;
}

// Batch allocations
// When TLD_DYN_ARRAYS is active: No single item #TotalSize-bytes new allocations
// but instead allocate big blocks (potentially wasting a couple bytes if items don't fit tightly)
// also has space overhead for list of blocks to free it later
#define TLD_BATCH_ALLOCS

#define TLD_TEMPLATE_HEADER template<typename KeyType, typename ValueType>
#define TLD_REC_TYPE TLinkedDataRec<KeyType, ValueType>

#if defined( P3_COLLECTIONS )
// Instead of using builtin C++ heap functionality new/delete, use custom GAMS big block heap from gmsheapnew
#define USE_GMSHEAP
#endif

namespace gdx::collections::datastorage
{

#ifdef TLD_BATCH_ALLOCS
struct DataBatch {
   DataBatch *next;
   uint8_t *ptr;
   explicit DataBatch( size_t count ) : next{}, ptr{ new uint8_t[count] } {}
   ~DataBatch()
   {
      delete[] ptr;
   }
};

template<int batchSize>
class BatchAllocator
{
   DataBatch *head, *tail;
   size_t offsetInTail;

public:
   BatchAllocator() : head{}, tail{}, offsetInTail{} {}

   ~BatchAllocator()
   {
      clear();
   }

   void clear()
   {
      if( !head ) return;
      DataBatch *next;
      for( DataBatch *it = head; it; it = next )
      {
         next = it->next;
         delete it;
      }
      head = tail = nullptr;
   }

   uint8_t *GetBytes( size_t count )
   {
      assert( count <= batchSize );
      if( !head )
      {
         head = tail = new DataBatch{ batchSize };
         offsetInTail = 0;
      }
      else if( batchSize - offsetInTail < count )
      {
         tail->next = new DataBatch{ batchSize };
         tail = tail->next;
         offsetInTail = 0;
      }
      auto res{ tail->ptr + offsetInTail };
      offsetInTail += count;
      return res;
   }
};
#endif

TLD_TEMPLATE_HEADER
struct TLinkedDataRec {
   TLinkedDataRec *RecNext{};
   // when RecData is used, first dim * sizeof(int) bytes are keys and then datasize * sizeof(double) bytes for values
   // hence data bytes start at offset FKeySize
   // when RecKeys is used corresponds directly to key entries (as integers)
   union
   {
      uint8_t RecData[20 * 4];
      int RecKeys[20];
   };
};

TLD_TEMPLATE_HEADER
class TLinkedData
{
   int FMinKey,
           FMaxKey,
           FDimension,// number of keys / symbol dimension
           FKeySize,  // byte count for key storage
           FDataSize, // byte count for value storage
           FTotalSize,// byte count for entry
           FCount;
   using RecType = TLD_REC_TYPE;
   RecType *FHead, *FTail;

#if defined( USE_GMSHEAP )
   gmsheapnew::THeapMgr MyHeap{ "TLinkedData" };
#elif defined( TLD_BATCH_ALLOCS )
   BatchAllocator<960> batchAllocator;
#endif

   bool IsSorted()
   {
      RecType *R{ FHead };
      auto *PrevKey{ R->RecKeys };
      R = R->RecNext;
      int KD{};
      while( R )
      {
         for( int D{}; D < FDimension; D++ )
         {
            KD = R->RecKeys[D] - PrevKey[D];
            if( KD ) break;
         }
         if( KD < 0 ) return false;
         PrevKey = R->RecKeys;
         R = R->RecNext;
      }
      return true;
   }

public:
   TLinkedData( int ADimension, int ADataSize ) : FMinKey{ std::numeric_limits<int>::max() },
                                                  FMaxKey{},
                                                  FDimension{ ADimension },
                                                  FKeySize{ ADimension * (int) sizeof( KeyType ) },
                                                  FDataSize{ ADataSize },
                                                  FTotalSize{ 1 * (int) sizeof( void * ) + FKeySize + FDataSize },
                                                  FCount{},
                                                  FHead{},
                                                  FTail{} {};

   ~TLinkedData()
   {
      Clear();
   }

   [[nodiscard]] int Count() const
   {
      return FCount;
   }

   void Clear()
   {
#if defined( TLD_BATCH_ALLOCS ) && !defined( USE_GMSHEAP )
      batchAllocator.clear();
#else
      RecType *P{ FHead };
      while( P )
      {
         auto Pn = P->RecNext;
#ifdef USE_GMSHEAP
         MyHeap.XFreeMem( P, FTotalSize );
#else
         delete[] P;
#endif
         P = Pn;
      }
#endif
      FCount = FMaxKey = 0;
      FHead = FTail = nullptr;
      FMinKey = std::numeric_limits<int>::max();
   }

   int MemoryUsed()
   {
      return FCount * FTotalSize;
   }

   RecType *AddItem( const KeyType *AKey, const ValueType *AData )
   {
#if defined( USE_GMSHEAP )
      auto *node = reinterpret_cast<RecType *>( MyHeap.XGetMem( FTotalSize ) );
#elif defined( TLD_BATCH_ALLOCS )
      auto *node = reinterpret_cast<RecType *>( batchAllocator.GetBytes( FTotalSize ) );
#else
      auto *node = reinterpret_cast<RecType *>( new uint8_t[FTotalSize] );
#endif
      if( !FHead ) FHead = node;
      else
         FTail->RecNext = node;
      FTail = node;
      node->RecNext = nullptr;
      std::memcpy( node->RecData, AKey, FKeySize );             // first FKeySize bytes for keys (integers)
      std::memcpy( &node->RecData[FKeySize], AData, FDataSize );// rest for actual data (doubles)
      FCount++;
      for( int D{}; D < FDimension; D++ )
      {
         int Key{ AKey[D] };
         if( Key > FMaxKey ) FMaxKey = Key;
         if( Key < FMinKey ) FMinKey = Key;
      }
      return node;
   }

   void Sort( const int *AMap = nullptr )
   {
      if( !FHead || IsSorted() ) return;
      const int AllocCount = FMaxKey - FMinKey + 1;
      const int KeyBase{ FMinKey };
#if !defined( USE_GMSHEAP )
      auto Head{ new RecType *[AllocCount] }, Tail{ new RecType *[AllocCount] };
#else
      const int64_t AllocSize{ static_cast<int64_t>( AllocCount * sizeof( RecType * ) ) };
      auto Head{ reinterpret_cast<RecType **>( MyHeap.XGetMem64( AllocSize ) ) };
      auto Tail{ reinterpret_cast<RecType **>( MyHeap.XGetMem64( AllocSize ) ) };
#endif
      std::memset( Head, 0, sizeof( RecType * ) * AllocCount );
      // Perform radix sort
      for( int D{ FDimension - 1 }; D >= 0; D-- )
      {
         RecType *R = FHead;
         while( R )
         {
            int Key{ R->RecKeys[AMap ? AMap[D] : D] - KeyBase };
            if( !Head[Key] ) Head[Key] = R;
            else
               Tail[Key]->RecNext = R;
            Tail[Key] = R;
            R = R->RecNext;
         }
         R = nullptr;
         for( int Key{ FMaxKey - KeyBase }; Key >= 0; Key-- )
         {
            if( Head[Key] )
            {
               Tail[Key]->RecNext = R;
               R = Head[Key];
               Head[Key] = nullptr;// so we keep it all nullptr
            }
         }
         FHead = R;
      }
      FTail = nullptr;// what is the tail???
#if !defined( USE_GMSHEAP )
      delete[] Head;
      delete[] Tail;
#else
      MyHeap.XFreeMem64( Head, AllocSize );
      MyHeap.XFreeMem64( Tail, AllocSize );
#endif
   }

   std::optional<RecType *> StartRead( const int *AMap = nullptr )
   {
      if( FCount <= 0 ) return std::nullopt;
      Sort( AMap );
      return { FHead };
   }

   bool GetNextRecord( RecType **P, KeyType *AKey, ValueType *AData )
   {
      if( P && *P )
      {
         const RecType &it = **P;
         std::memcpy( AKey, it.RecData, FKeySize );             // first FKeySize bytes for keys (integers)
         std::memcpy( AData, &it.RecData[FKeySize], FDataSize );// rest actual data bytes (doubles)
         *P = it.RecNext;
         return true;
      }
      return false;
   }
};
}// namespace gdx::collections::datastorage
