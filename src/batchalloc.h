#pragma once

#include <cstdint>
#include <cassert>

namespace gdx::batchalloc
{

struct DataBatch {
   DataBatch *next;
   uint8_t *ptr;
   explicit DataBatch( size_t count ) : next {}, ptr { new uint8_t[count] } {}
   ~DataBatch()
   {
      delete[] ptr;
   }
};

class BatchAllocator
{
   DataBatch *head {}, *tail {};
   size_t offsetInTail {}, batchSize { 1024 };

public:
   explicit BatchAllocator( size_t _batchSize ) : batchSize { _batchSize } {}
   BatchAllocator() = default;

   void setBatchSize(size_t _batchSize) {
      batchSize = _batchSize;
   }

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

}// namespace gdx::batchalloc
