/*
* GAMS - General Algebraic Modeling System GDX API
 *
 * Copyright (c) 2017-2025 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2025 GAMS Development Corp. <support@gams.com>
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

struct DataBatch {
   DataBatch *next;
   uint8_t *ptr;
   explicit DataBatch( const size_t count ) : next {}, ptr { new uint8_t[count] } {}
   ~DataBatch()
   {
      delete[] ptr;
   }
};

template<int batchSize>
class BatchAllocator
{
   DataBatch *head {}, *tail {};
   size_t offsetInTail {};

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
      for( DataBatch *it = head; it; it = next )
      {
         next = it->next;
         delete it;
      }
      head = tail = nullptr;
   }

   uint8_t *GetBytes( size_t count )
   {
      static_assert(batchSize % 8 == 0);
      // add padding after block if its byte count doesn't align with 8 byte (64 bit)
      constexpr int byteAlign {8};
      if(count % byteAlign)
         count = ((int)(count / byteAlign) + 1) * byteAlign;
      assert( count <= batchSize );
      if( !head )
      {
         head = tail = new DataBatch { batchSize };
         offsetInTail = 0;
      }
      else if( batchSize - offsetInTail < count )
      {
         tail->next = new DataBatch { batchSize };
         tail = tail->next;
         offsetInTail = 0;
      }
      auto res { tail->ptr + offsetInTail };
      offsetInTail += count;
      return res;
   }
};

}// namespace gdlib::batchalloc
