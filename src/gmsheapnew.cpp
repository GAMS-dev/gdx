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

#include "gmsheapnew.h"
#include <algorithm>// for find, max
#include <cstdint>  // for int64_t, uint16_t, uint8_t
#include <cstdio>   // for fclose, fopen, fscanf, FILE
#include <cstdlib>  // for free, malloc, realloc
#include <cstring>  // for memset, memcpy
#include <stdexcept>// for runtime_error
#include <string>   // for string, operator""s, operator+, string_literals
#include <utility>  // for move

//#define BYPASSHEAPMGR

#ifdef _WIN32
#pragma comment( lib, "iphlpapi.lib" )
#define _WINSOCKAPI_ /* Prevent inclusion of winsock.h in windows.h */
#include <Windows.h>
#include <WinSock2.h>
#include <io.h>
#include <Psapi.h> /* enough if we run on Windows 7 or later */
#include <ShlObj.h>
#include <iphlpapi.h>
#include <IPTypes.h>
#else
#if( defined( __linux__ ) || defined( __APPLE__ ) ) /* at least, maybe for others too */
#if defined( __linux__ )
#elif defined( __APPLE__ )
#include <libproc.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <sys/ioctl.h>
#include <sys/proc_info.h>
#include <sys/sysctl.h>
#endif
#endif
#include <unistd.h>
#endif

using namespace std::literals::string_literals;

namespace gdx::gmsheapnew
{

bool getMemoryInfo( int64_t &rss, int64_t &vss );

bool getMemoryInfo( int64_t &rss, int64_t &vss )
{
#if defined( _WIN32 )
   PROCESS_MEMORY_COUNTERS info;
   int ok = GetProcessMemoryInfo( GetCurrentProcess(), &info, sizeof( info ) );
   if( !ok )
      return false; /* failure */
   rss = (int64_t) info.WorkingSetSize;
   vss = (int64_t) info.PagefileUsage;
   return true; /* success */
#elif defined( __linux )
   size_t sz;
   FILE *fp = fopen( "/proc/self/statm", "r" );
   if( !fp )
      return false; /* failure */
   /* first two are VmSize, VmRSS */
   unsigned long urss, uvss;
   int n = fscanf( fp, "%lu %lu", &uvss, &urss );
   fclose( fp );
   if( 2 != n )
      return false; /* failure */
   sz = sysconf( _SC_PAGESIZE );
   rss = sz * urss;
   vss = sz * uvss;
   return true; /* success */
#elif defined( __APPLE__ )
   int ret;
   struct proc_taskinfo procTaskInfo;
   ret = proc_pidinfo( (int) getpid(), PROC_PIDTASKINFO, 0,
                       (void *) &procTaskInfo, sizeof( procTaskInfo ) );
   if( ret < (int) sizeof( procTaskInfo ) )
      return false; /* failure */
   rss = (int64_t) procTaskInfo.pti_resident_size;
   vss = (int64_t) procTaskInfo.pti_virtual_size;
   return true; /* success */
#else
   throw std::runtime_error( "Unknown platform for getMemoryInfo!" );
   return false; /* fail */
#endif
}

void *TBigBlockMgr::GetBigBlock()
{
   void *res{ FreeList.empty() ? nullptr : FreeList.back() };
   if( res )
   {
      FreeList.pop_back();
      return res;
   }
   else
   {
      IncreaseMemorySize( BIGBLOCKSIZE );
      return std::malloc( BIGBLOCKSIZE );
   }
}

void TBigBlockMgr::ReleaseBigBlock( void *P )
{
   FreeList.push_back( P );
}

void TBigBlockMgr::ReduceMemorySize( int64_t Delta )
{
   OtherMemory -= Delta;
   TotalMemory -= (double) Delta;
   if( MemoryReportProc ) MemoryReportProc( MemoryUsedMB() );
}

void TBigBlockMgr::IncreaseMemorySize( int64_t Delta )
{
   if( TotalMemory + (double) Delta > MemoryLimit )
      throw std::runtime_error( "Requested memory exceeds assigned HeapLimit"s );
   OtherMemory += Delta;
   if( OtherMemory > HighMark ) HighMark = OtherMemory;
   TotalMemory += (double) Delta;
   if( TotalMemory > TotalHighMark ) TotalHighMark = TotalMemory;
   if( MemoryReportProc ) MemoryReportProc( MemoryUsedMB() );
}

TBigBlockMgr::TBigBlockMgr( std::string Name ) : spName{ std::move( Name ) }
{
}

TBigBlockMgr::~TBigBlockMgr()
{
   XClear();
}

// Brief:
//   Return the amount of memory used by all active heaps. The amount returned is the total memory use
//   divided by 1,000,000
double TBigBlockMgr::MemoryUsedMB() const
{
   {
      int64_t rss{}, vss{};
      if( showOSMem == 1 && getMemoryInfo( rss, vss ) )
         return (double) rss / 1e6;
      if( showOSMem == 2 && getMemoryInfo( rss, vss ) )
         return (double) vss / 1e6;
   }
   return TotalMemory / 1e6;
}

// Brief:
//   Returns the current memory limit specified by calling SetMemoryLimit
// Returns:
//   Returns the value specified when calling SetMemoryLimit divided by 1,000,000
double TBigBlockMgr::MemoryLimitMB() const
{
   return MemoryLimit / 1e6;
}

void TBigBlockMgr::XClear()
{
   for( size_t N{}; N < FreeList.size(); N++ )
      std::free( FreeList[N] );
   ReduceMemorySize( static_cast<int64_t>( FreeList.size() * BIGBLOCKSIZE ) );
   FreeList.clear();
}

void TBigBlockMgr::GetBigStats( int64_t &sizeOtherMemory, int64_t &sizeHighMark, int64_t &cntFree ) const
{
   sizeOtherMemory = OtherMemory;
   sizeHighMark = HighMark;
   cntFree = (int64_t) FreeList.size();
}

std::string TBigBlockMgr::GetName() const
{
   return spName;
}

void TBigBlockMgr::SetOSMemory( int v )
{
   showOSMem = v;
}

PLargeBlock THeapMgr::GetWorkBuffer()
{
   void *p = BlockMgr.GetBigBlock();
   auto res = new TLargeBlock{ BIGBLOCKSIZE / HEAPGRANULARITY, p, p };
   WrkBuffs.push_back( res );
   return res;
}

void THeapMgr::ReleaseWorkBuffer( PLargeBlock P )
{
   BlockMgr.ReleaseBigBlock( P->InitialPtr );
   if( const auto pit = std::find( WrkBuffs.begin(), WrkBuffs.end(), P ); pit != WrkBuffs.end() )
      WrkBuffs.erase( pit );
   delete P;
}

void THeapMgr::ReduceMemorySize( int64_t Delta )
{
   BlockMgr.ReduceMemorySize( Delta );
   OtherMemory -= Delta;
}

void THeapMgr::IncreaseMemorySize( int64_t Delta )
{
   BlockMgr.IncreaseMemorySize( Delta );
   OtherMemory += Delta;
   if( OtherMemory > HighMark ) HighMark = OtherMemory;
}

void THeapMgr::prvClear()
{
   while( !WrkBuffs.empty() )
      ReleaseWorkBuffer( WrkBuffs.back() );
   WrkBuffs.clear();
   WorkBuffer = nullptr;
   for( const auto act: Active )
      std::free( act );
   Active.clear();
   for( auto &slot: Slots )
   {
      slot.GetCount = slot.FreeCount = slot.ListCount = 0;
      slot.FirstFree = nullptr;
   }
   ReduceMemorySize( OtherMemory );
   OtherGet = OtherFree = OtherGet64 = OtherFree64 = ReAllocCnt = ReAllocUsed = ReAllocCnt64 = ReAllocUsed64 = 0;
}

void *THeapMgr::prvGMSGetMem( uint16_t slot )
{
   {
      auto &obj = Slots[slot - 1];
      obj.GetCount++;
      void *res{ obj.FirstFree };
      if( res )
      {
         obj.FirstFree = static_cast<PSmallBlock>( res )->NextSmallBlock;
         obj.ListCount--;
         return res;
      }
   }

   if( !WorkBuffer ) WorkBuffer = GetWorkBuffer();

   {
      auto wb = WorkBuffer;
      if( wb->FreeSlots >= slot )
      {
         void *res{ wb->CurrPtr };
         wb->CurrPtr = &( (std::uint8_t *) wb->CurrPtr )[slot * HEAPGRANULARITY];
         wb->FreeSlots -= slot;
         return res;
      }
      if( wb->FreeSlots > 0 )
      {
         auto &fs = Slots[wb->FreeSlots - 1];
         fs.ListCount++;
         static_cast<PSmallBlock>( wb->CurrPtr )->NextSmallBlock = fs.FirstFree;
         fs.FirstFree = static_cast<PSmallBlock>( wb->CurrPtr );
      }
   }

   WorkBuffer = GetWorkBuffer();

   {
      auto wb = WorkBuffer;
      void *res{ wb->CurrPtr };
      wb->CurrPtr = &( (std::uint8_t *) wb->CurrPtr )[slot * HEAPGRANULARITY];
      wb->FreeSlots -= slot;
      return res;
   }
}

void THeapMgr::prvGMSFreeMem( void *p, uint16_t slot )
{
   auto &obj = Slots[slot - 1];
   obj.FreeCount++;
   obj.ListCount++;
   static_cast<PSmallBlock>( p )->NextSmallBlock = obj.FirstFree;
   obj.FirstFree = static_cast<PSmallBlock>( p );
}

void *THeapMgr::prvXGetMem( int Size )
{
   if( Size <= 0 )
      return nullptr;

   if( Size <= LastSlot * HEAPGRANULARITY )
      return prvGMSGetMem( (uint16_t) ( ( Size - 1 ) / HEAPGRANULARITY ) + 1 );

   OtherGet++;
   IncreaseMemorySize( Size );
   void *res{ std::malloc( Size ) };
   Active.push_back( res );
   return res;
}

void *THeapMgr::prvXGetMemNC( int Size )
{
   if( Size <= 0 ) return nullptr;
   OtherGet++;
   IncreaseMemorySize( Size );
   return std::malloc( Size );
}

void *THeapMgr::prvXGetMem64( int64_t Size )
{
   if( Size <= 0 )
      return nullptr;

   if( Size <= LastSlot * HEAPGRANULARITY )
      return prvGMSGetMem( (uint16_t) ( ( Size - 1 ) / HEAPGRANULARITY ) + 1 );

   OtherGet64++;
   IncreaseMemorySize( Size );
   Active.push_back( std::malloc( Size ) );
   return Active.back();
}

void THeapMgr::prvXFreeMem( void *P, int Size )
{
   if( Size > 0 )
   {
      if( Size <= LastSlot * HEAPGRANULARITY ) prvGMSFreeMem( P, static_cast<uint16_t>( ( Size - 1 ) / HEAPGRANULARITY + 1 ) );
      else
      {
         OtherFree++;
         Active.erase( std::find( Active.begin(), Active.end(), P ) );
         ReduceMemorySize( Size );
         std::free( P );
      }
   }
}

void THeapMgr::prvXFreeMem64( void *P, int64_t Size )
{
   if( Size > 0 )
   {
      if( Size <= LastSlot * HEAPGRANULARITY )
         prvGMSFreeMem( P, static_cast<uint16_t>( ( Size - 1 ) / HEAPGRANULARITY + 1 ) );
      else
      {
         OtherFree64++;
         Active.erase( std::find( Active.begin(), Active.end(), P ) );
         ReduceMemorySize( Size );
         std::free( P );
      }
   }
}

void THeapMgr::prvGetSlotCnts( THeapSlotNr Slot, int64_t &cntGet, int64_t &cntFree, int64_t &cntAvail )
{
   const auto &obj = Slots[Slot - 1];
   cntGet = obj.GetCount;
   cntFree = obj.FreeCount;
   cntAvail = obj.ListCount;
}

THeapMgr::THeapMgr( const std::string &Name ) : BlockMgr{ "BBMgr_"s + Name }, spName{ Name }
{
   prvClear();
}

THeapMgr::~THeapMgr()
{
   prvClear();
}

void THeapMgr::Clear()
{
   prvClear();
}

void *THeapMgr::GMSGetMem( uint16_t slot )
{
#ifdef BYPASSHEAPMGR
   return std::malloc( slot * HEAPGRANULARITY );
#else
   return prvGMSGetMem( slot );
#endif
}

void THeapMgr::GMSFreeMem( void *p, uint16_t slot )
{
#ifdef BYPASSHEAPMGR
   std::free( p );
#else
   prvGMSFreeMem( p, slot );
#endif
}

void *THeapMgr::XGetMem( int Size )
{
#ifdef BYPASSHEAPMGR
   return std::malloc( Size );
#else
   return prvXGetMem( Size );
#endif
}

void *THeapMgr::XGetMemNC( int Size )
{
#ifdef BYPASSHEAPMGR
   return std::malloc( Size );
#else
   return prvXGetMemNC( Size );
#endif
}

void *THeapMgr::XAllocMem( int Size )
{
#ifdef BYPASSHEAPMGR
   return std::malloc( Size );
#else
   void *res{ prvXGetMem( Size ) };
   if( res ) std::memset( res, 0, Size );
   return res;
#endif
}

void *THeapMgr::XAllocMemNC( int Size )
{
#ifdef BYPASSHEAPMGR
   return std::malloc( Size );
#else
   void *res{ prvXGetMemNC( Size ) };
   if( res ) std::memset( res, 0, Size );
   return res;
#endif
}

void *THeapMgr::XGetMem64( int64_t Size )
{
#ifdef BYPASSHEAPMGR
   return std::malloc( Size );
#else
   return prvXGetMem64( Size );
#endif
}

void *THeapMgr::XAllocMem64( int64_t Size )
{
#ifdef BYPASSHEAPMGR
   return std::malloc( Size );
#else
   void *res{ prvXGetMem64( Size ) };
   if( res )
      std::memset( res, 0, Size );
   return res;
#endif
}

void THeapMgr::XFreeMem( void *P, int Size )
{
#ifdef BYPASSHEAPMGR
   if( Size > 0 ) std::free( P );
#else
   prvXFreeMem( P, Size );
#endif
}

void THeapMgr::XFreeMemNC( void *P, int Size )
{
#ifdef BYPASSHEAPMGR
   std::free( P );
#else
   OtherFree++;
   ReduceMemorySize( Size );
   std::free( P );
#endif
}

void THeapMgr::XFreeMemAndNil( void **P, int Size )
{
#ifdef BYPASSHEAPMGR
   if( Size > 0 ) std::free( *P );
#else
   prvXFreeMem( *P, Size );
#endif
   *P = nullptr;
}

void THeapMgr::XFreeMem64( void *P, int64_t Size )
{
#ifdef BYPASSHEAPMGR
   if( Size > 0 ) std::free( P );
#else
   prvXFreeMem64( P, Size );
#endif
}

void THeapMgr::XFreeMem64andNil( void **P, int64_t Size )
{
#ifdef BYPASSHEAPMGR
   if( Size > 0 ) std::free( *P );
#else
   prvXFreeMem64( *P, Size );
#endif
   *P = nullptr;
}

void THeapMgr::XReAllocMem( void **P, int OldSize, int NewSize )
{
#ifdef BYPASSHEAPMGR
   if( NewSize <= 0 )
   {
      if( OldSize > 0 && P && *P ) std::free( *P );
      *P = nullptr;
   }
   else if( !P || !*P || OldSize <= 0 )
   {
      *P = std::malloc( NewSize );
   }
   else
      *P = std::realloc( *P, NewSize );
#else
   void *PNew;
   ReAllocCnt++;
   ReAllocUsed -= OldSize;
   ReAllocUsed += NewSize;
   if( NewSize <= 0 )
   {
      if( OldSize > 0 && P ) prvXFreeMem( P, OldSize );
      PNew = nullptr;
   }
   else if( !P || OldSize <= 0 )
   {
      PNew = prvXGetMem( NewSize );
   }
   else if( OldSize == NewSize )
   {
      PNew = P;
   }
   else if( OldSize > LastSlot * HEAPGRANULARITY && NewSize > LastSlot * HEAPGRANULARITY )
   {
      PNew = P;
      Active.erase( std::find( Active.begin(), Active.end(), P ) );
      PNew = std::realloc( PNew, NewSize );
      Active.push_back( PNew );
      if( NewSize > OldSize ) IncreaseMemorySize( NewSize - OldSize );
      else
         ReduceMemorySize( OldSize - NewSize );
   }
   else
   {
      PNew = prvXGetMem( NewSize );
      std::memcpy( PNew, P, OldSize <= NewSize ? OldSize : NewSize );
      prvXFreeMem( P, OldSize );
   }
   if(P)
      *P = PNew;
#endif
}

void THeapMgr::XReAllocMemNC( void **P, int OldSize, int NewSize )
{
#ifdef BYPASSHEAPMGR
   if( NewSize <= 0 )
   {
      if( OldSize > 0 && P && *P ) std::free( *P );
      *P = nullptr;
   }
   else if( !P || OldSize <= 0 )
      *P = std::malloc( NewSize );
   else
      *P = std::realloc( *P, NewSize );
#else
   ReAllocCnt++;
   ReAllocUsed -= OldSize;
   ReAllocUsed += NewSize;
   *P = std::realloc( *P, NewSize );
#endif
}

void THeapMgr::XReAllocMem64( void **P, int64_t OldSize, int64_t NewSize )
{
   void *PNew;
#ifdef BYPASSHEAPMGR
   if( NewSize <= 0 )
   {
      if( OldSize > 0 && P ) std::free( P );
      P = nullptr;
   }
   else if( !P || OldSize <= 0 )
      PNew = std::malloc( NewSize );
   else
   {
      PNew = std::malloc( NewSize );
      std::memcpy( PNew, P, OldSize <= NewSize ? OldSize : NewSize );
      std::free( P );
   }
#else
   ReAllocCnt64++;
   ReAllocUsed64 -= OldSize;
   ReAllocUsed64 += NewSize;
   if( NewSize <= 0 )
   {
      if( OldSize > 0 && P ) {
         prvXFreeMem64( P, OldSize );
         P = nullptr;
      }
      PNew = nullptr;
   }
   else if( !P || OldSize <= 0 )
   {
      PNew = prvXGetMem64( NewSize );
   }
   else if( OldSize == NewSize )
      PNew = P;
   else if( OldSize > LastSlot * HEAPGRANULARITY && NewSize > LastSlot * HEAPGRANULARITY )
   {
      PNew = P;
      Active.erase( std::find( Active.begin(), Active.end(), P ) );
      PNew = std::realloc( PNew, NewSize );
      Active.push_back( PNew );
      if( NewSize > OldSize ) IncreaseMemorySize( NewSize - OldSize );
      else ReduceMemorySize( OldSize - NewSize );
   }
   else
   {
      PNew = prvXGetMem64( NewSize );
      std::memcpy( PNew, P, OldSize <= NewSize ? OldSize : NewSize );
      prvXFreeMem64( P, OldSize );
   }
#endif
   if(P)
      *P = PNew;
}

void THeapMgr::GetSlotCnts( THeapSlotNr Slot, int64_t &cntGet, int64_t &cntFree, int64_t &cntAvail )
{
   prvGetSlotCnts( Slot, cntGet, cntFree, cntAvail );
}

void THeapMgr::GetBlockStats( int64_t &cntWrkBuffs, int64_t &cntActive, int64_t &sizeOtherMemory, int64_t &sizeHighMark ) const
{
   cntWrkBuffs = (int64_t) WrkBuffs.size();
   cntActive = (int64_t) Active.size();
   sizeOtherMemory = OtherMemory;
   sizeHighMark = HighMark;
}

void THeapMgr::GetOtherStats( bool do64, int64_t &cntGet, int64_t &cntFree, int64_t &cntReAlloc, int64_t &sizeRUsed ) const
{
   if( do64 )
   {
      cntGet = OtherGet64;
      cntFree = OtherFree64;
      cntReAlloc = ReAllocCnt64;
      sizeRUsed = ReAllocUsed64;
   }
   else
   {
      cntGet = OtherGet;
      cntFree = OtherFree;
      cntReAlloc = ReAllocCnt;
      sizeRUsed = ReAllocUsed;
   }
}

int64_t THeapMgr::GetFreeSlotSpace()
{
   int64_t res{}, cntGet, cntFree, cntAvail;
   for( THeapSlotNr Slot{ 1 }; Slot <= LastSlot; ++Slot )
   {
      prvGetSlotCnts( Slot, cntGet, cntFree, cntAvail );
      int64_t sizeFree{ cntAvail * Slot * HEAPGRANULARITY };
      res += sizeFree;
   }
   return res;
}

bool THeapMgr::SetMemoryLimit( double limit )
{
   BlockMgr.MemoryLimit = limit;
   return limit >= BlockMgr.TotalMemory;
}

void THeapMgr::SetMemoryReportProc( const TMemoryReportProc &F )
{
   BlockMgr.MemoryReportProc = F;
}

std::string THeapMgr::GetName() const
{
   return spName;
}

TBigBlockMgr &THeapMgr::GetBBMgr()
{
   return BlockMgr;
}

}// namespace gdx::gmsheapnew
