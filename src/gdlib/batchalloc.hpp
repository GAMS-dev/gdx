/*
* GAMS - General Algebraic Modeling System GDX API
 *
 * Copyright (c) 2017-2026 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2026 GAMS Development Corp. <support@gams.com>
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

// This big block allocator is not as good as gmsheapnew so please prefer that one!

#pragma once

#include <cstdint>
#include <cassert>

namespace gdlib::batchalloc
{

template<size_t batchSize, int byteAlign = 1>
class BatchAllocator
{
   struct DataBatch {
      DataBatch *next;
      uint8_t *ptr;
   
      explicit DataBatch( const size_t count ) : next {}, ptr { new uint8_t[count] }
      {
         assert(count % byteAlign == 0);
      }
   
      ~DataBatch()
      {
         delete[] ptr;
      }
   };

   DataBatch *head {}, *tail {};
   size_t offsetInTail {}, firstBatchSize {batchSize};
public:
   BatchAllocator() = default;

   ~BatchAllocator()
   {
      clear();
   }

   void clear()
   {
      if( !head ) return;
      DataBatch *next;
      for( const DataBatch *it = head; it; it = next )
      {
         next = it->next;
         delete it;
      }
      head = tail = nullptr;
   }

   // Use different buffer size for first batch
   // In case we know lots of data will be requested first
   void SetFirstBatchSize(size_t s) {
      if( const auto fbs = ( s + ( byteAlign - 1 ) ) & ~( byteAlign - 1 );
         fbs > batchSize && fbs < 0x10000000000)
         firstBatchSize = fbs;
   }

   uint8_t *GetBytes( size_t count )
   {
      static_assert(batchSize % byteAlign == 0);
      // add padding after block if its byte count doesn't align with 8 byte (64 bit)
      count = (count + ( byteAlign - 1 )) & ~( byteAlign - 1 );
      assert( count <= batchSize );
      if( !head )
      {
         head = tail = new DataBatch { firstBatchSize };
         offsetInTail = 0;
      }
      else if( ( head == tail ? firstBatchSize : batchSize ) - offsetInTail < count )
      {
         tail->next = new DataBatch { batchSize };
         tail = tail->next;
         offsetInTail = 0;
      }
      const auto res { tail->ptr + offsetInTail };
      offsetInTail += count;
      return res;
   }

   template<typename T>
   T *GetBytes()
   {
      return reinterpret_cast<T *>( GetBytes( sizeof( T ) ) );
   }
};

}// namespace gdlib::batchalloc
