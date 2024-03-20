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

#include "../global/delphitypes.h"
#include "../gdlib/gmsobj.h"

namespace gdlib::gmsheapnew
{

using TMemoryReportProc = std::function<void( double )>;

constexpr int  BIGBLOCKSIZE = 0x80000,// 0.5 Mb note same value in glookup
               HEAPGRANULARITY = 8,
               LastSlot = 256 / HEAPGRANULARITY;

using THeapSlotNr = global::delphitypes::Bounded<int, 1, LastSlot>;

struct TSmallBlock {
   TSmallBlock *NextSmallBlock;
};
using PSmallBlock = TSmallBlock *;

struct TSlotRecord {
   PSmallBlock FirstFree;
   int64_t GetCount, FreeCount, ListCount;
};

// I do not like the small block to point to the big block
// Could put this inside the block, but may get into trouble with
// the string heap which is written to the workfile
struct TLargeBlock {
   int FreeSlots;
   void *InitialPtr, *CurrPtr;

   TLargeBlock( const int freeSlots, void *initialPtr, void *currPtr ) : FreeSlots { freeSlots }, InitialPtr { initialPtr }, CurrPtr { currPtr } {}
};
using PLargeBlock = TLargeBlock *;

class TBigBlockMgr final
{
   std::string spName;
   int64_t OtherMemory {}, HighMark {};
   std::vector<void *> FreeList {};
   double MemoryLimit { 1e200 }, TotalMemory {}, TotalHighMark {};
   TMemoryReportProc MemoryReportProc {};
   int showOSMem {};

   void ReleaseBigBlock( void *P );
   void ReduceMemorySize( int64_t Delta );
   void IncreaseMemorySize( int64_t Delta );
   void *GetBigBlock();

public:
   friend class THeapMgr;

   explicit TBigBlockMgr( std::string Name );
   ~TBigBlockMgr();

   [[nodiscard]] double MemoryUsedMB() const;
   [[nodiscard]] double MemoryLimitMB() const;
   void XClear();
   void GetBigStats( int64_t &sizeOtherMemory, int64_t &sizeHighMark, int64_t &cntFree ) const;
   [[nodiscard]] std::string GetName() const;
   void SetOSMemory( int v );
};

//Note: In this class we need to maintain a clear separation of public and
//      private classes. This means that a public class should not call
//      another public class. This was done to allow for an easier/more
//      maintainable way to mutex/protect critical sections.
class THeapMgr final
{
   PLargeBlock WorkBuffer {};
   TBigBlockMgr BlockMgr;
   std::array<TSlotRecord, LastSlot> Slots;
   int64_t HighMark {}, OtherMemory {}, OtherGet {}, OtherFree {};
   int64_t OtherGet64 {}, OtherFree64 {};
   int64_t ReAllocCnt {}, ReAllocUsed {}, ReAllocCnt64 {}, ReAllocUsed64 {};
   std::vector<PLargeBlock> WrkBuffs;
   std::vector<void *> Active;
   std::string spName;

   PLargeBlock GetWorkBuffer();
   void ReleaseWorkBuffer( PLargeBlock P );
   void ReduceMemorySize( int64_t Delta );
   void IncreaseMemorySize( int64_t Delta );

   void prvClear();
   void *prvGMSGetMem( uint16_t slot );
   void prvGMSFreeMem( void *p, uint16_t slot );
   void *prvXGetMem( int Size );
   void *prvXGetMemNC( int Size );
   void *prvXGetMem64( int64_t Size );
   void prvXFreeMem( void *P, int Size );
   void prvXFreeMem64( void *P, int64_t Size );
   void prvGetSlotCnts( THeapSlotNr Slot, int64_t &cntGet, int64_t &cntFree, int64_t &cntAvail ) const;

public:
   explicit THeapMgr( const std::string &Name );
   ~THeapMgr();
   void Clear();
   void *GMSGetMem( uint16_t slot );
   void GMSFreeMem( void *p, uint16_t slot );
   void *XGetMem( int Size );

   template<typename T>
   inline T *XGetMem() {
      return static_cast<T *>( XGetMem( sizeof( T ) ) );
   }

   // Use placement-new to init obj via constructor for manually allocated storage
   template<typename T>
   inline T* XGetMemCreat() {
      return new(XGetMem<T>()) T {};
   }

   void *XGetMemNC( int Size );
   void *XAllocMem( int Size );

   template<typename T>
   inline T *XAllocMem() {
      return static_cast<T *>( XAllocMem( sizeof( T ) ) );
   }

   // Use XAllocMemZero when type T has no constructor defined
   template<typename T>
   inline T *XAllocMemCreat()
   {
      return new(XAllocMem<T>()) T {};
   }

   template<typename T>
   inline T* XAllocMemZero()
   {
      auto ptr { XAllocMem<T>() };
      std::memset( ptr, 0, sizeof( T ) );
      return ptr;
   }

   void *XAllocMemNC( int Size );
   void *XGetMem64( int64_t Size );

   template<typename T>
   T *XGetMem64() {
      return (T *) XGetMem64(sizeof(T));
   }

   void *XAllocMem64( int64_t Size );
   void XFreeMem( void *P, int Size );

   template<typename T>
   void XFreeMem(T *p) {
      XFreeMem(p, sizeof(T));
   }

   // With explicit destructor call for objs allocated via XGetMemCreat (placement-new)
   template<typename T>
   void XFreeMemDestr(T *p) {
      if(p)
      {
         p->~T();
         XFreeMem(p, sizeof(T));
      }
   }

   void XFreeMemNC( void *P, int Size );
   void XFreeMemAndNil( void **P, int Size );
   void XFreeMem64( void *P, int64_t Size );

   template<typename T>
   void XFreeMem64( T *P ) {
      XFreeMem64(P, sizeof(T));
   }

   template<typename T>
   void XFreeMem64Destr( T *P ) {
      P->~T();
      XFreeMem64<T>(P);
   }

   void XFreeMem64andNil( void **P, int64_t Size );
   void XReAllocMem( void **P, int OldSize, int NewSize );
   void XReAllocMemNC( void **P, int OldSize, int NewSize );
   void XReAllocMem64( void **P, int64_t OldSize, int64_t NewSize );
   void GetSlotCnts( THeapSlotNr Slot, int64_t &cntGet, int64_t &cntFree, int64_t &cntAvail ) const;
   void GetBlockStats( int64_t &cntWrkBuffs, int64_t &cntActive, int64_t &sizeOtherMemory, int64_t &sizeHighMark ) const;
   void GetOtherStats( bool do64, int64_t &cntGet, int64_t &cntFree, int64_t &cntReAlloc, int64_t &sizeRUsed ) const;
   int64_t GetFreeSlotSpace() const;
   bool SetMemoryLimit( double limit );
   void SetMemoryReportProc( const TMemoryReportProc &F );

   [[nodiscard]] std::string GetName() const;
   TBigBlockMgr &GetBBMgr();
};

};// namespace gdlib::gmsheapnew
