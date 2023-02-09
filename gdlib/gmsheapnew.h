#pragma once

#include "../global/delphitypes.h"
#include "../gdlib/gmsobj.h"

namespace gdlib::gmsheapnew {

    using TMemoryReportProc = std::function<void(double)>;

    const int   BIGBLOCKSIZE = 0x80000, // 0.5 Mb note same value in glookup
                HEAPGRANULARITY = 8,
                LastSlot = 256 / HEAPGRANULARITY;

    using THeapSlotNr = global::delphitypes::Bounded<int, 1, LastSlot>;

    struct TSmallBlock {
        TSmallBlock *NextSmallBlock;
    };
    using PSmallBlock = TSmallBlock*;

    struct TSlotRecord {
        PSmallBlock FirstFree;
        uint64_t GetCount, FreeCount, ListCount;
    };

    // I do not like the small block to point to the big block
    // Could put this inside the block, but may get into trouble with
    // the string heap which is written to the workfile
    struct TLargeBlock {
        int FreeSlots;
        void *InitialPtr, *CurrPtr;
    };
    using PLargeBlock = TLargeBlock;

    class TBigBlockMgr {
        std::string spName;
        uint64_t OtherMemory{}, HighMark{};
        std::vector<void *> FreeList{};
        double MemoryLimit{1e200}, TotalMemory{}, TotalHighMark{};
        TMemoryReportProc MemoryReportProc{};
        int showOSMem{};

        void* GetBigBlock();
        void ReleaseBigBlock(void* P);
        void ReduceMemorySize(uint64_t Delta);
        void IncreaseMemorySize(uint64_t Delta);
    public:
        explicit TBigBlockMgr(std::string Name);
        virtual ~TBigBlockMgr();
        double MemoryUsedMB();
        double MemoryLimitMB();
        void XClear();
        std::string GetName() const;
        void SetOSMemory(int v);
    };

    //Note: In this class we need to maintain a clear separation of public and
    //      private classes. This means that a public class should not call
    //      another public class. This was done to allow for an easier/more
    //      maintainable way to mutex/protect critical sections.
    class THeapMgr {
        PLargeBlock WorkBuffer;
        TBigBlockMgr BlockMgr;
        std::array<TSlotRecord, LastSlot> Slots;
        uint64_t HighMark, OtherMemory, OtherGet, OtherFree;
        uint64_t OtherGet64, OtherFree64;
        uint64_t ReAllocCnt, ReAllocUsed, ReAllocCnt64, ReAllocUsed64;
        std::vector<void *> WrkBuffs, Active;
        std::string spName;

        PLargeBlock GetWorkBuffer();
        void ReleaseWorkBuffer(PLargeBlock P);
        void ReduceMemorySize(uint64_t Delta);
        void IncreaseMemorySize(uint64_t Delta);

        void prvClear();
        void *prvGMSGetMem(uint16_t slot);
        void prvGMSFreeMem(void *p, uint16_t slot);
        void *prvXGetMem(int Size);
        void *prvXGetMemNC(int Size);
        void *prvXGetMem64(uint64_t Size);
        void prvXFreeMem(void *P, int Size);
        void prvXFreeMem64(void *P, int64_t Size);
        void prvGetSlotCnts(THeapSlotNr Slot, uint64_t &cntGet, uint64_t cntFree, uint64_t cntAvail);

    public:
        explicit THeapMgr(const std::string &Name);
        virtual ~THeapMgr();
        void Clear();
        void *GMSGetMem(uint16_t slot);
        void GMSFreeMem(void *p, uint16_t slot);
        void *XGetMem(int Size);
        void XGetMemNC(int Size);
        void *XAllocMem(int Size);
        void *XAllocMemNC(int Size);
        void XGetMem64(uint64_t Size);
        void *XAllocMem64(uint64_t Size);
        void XFreeMem(void *P, int Size);
        void XFreeMemNC(void *P, int Size);
        void XFreeMemandNil(void **P, int Size);
        void XFreeMem64(void *P, uint64_t Size);
        void XFreeMem64andNil(void *P, uint64_t Size);
        void XReAllocMem(void **P, int OldSize, int NewSize);
        void XReAllocMemNC(void **P, int OldSize, int NewSize);
        void XReAllocMem64(void **P, uint64_t OldSize, uint64_t NewSize);
        void GetSlotCnts(THeapSlotNr Slot, uint64_t &cntGet, uint64_t &cntFree, uint64_t &cntAvail);
        void GetBlockStats(uint64_t &cntWrkBuffs, uint64_t &cntActive, uint64_t &sizeOtherMemory, uint64_t &sizeHighMark);
        void GetOtherStats(bool do64, uint64_t &cntGet, uint64_t &cntFree, uint64_t &cntReAlloc, uint64_t &sizeRUsed);
        uint64_t GetFreeSlotSpace();
        bool SetMemoryLimit(double limit);
        void SetMemoryReportProc(const TMemoryReportProc &F);

        std::string GetName() const { return spName; }

        TBigBlockMgr &GetBBMgr() {
            return BlockMgr;
        }
    };

};
