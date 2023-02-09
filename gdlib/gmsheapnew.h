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
        int64_t GetCount, FreeCount, ListCount;
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
        int64_t OtherMemory{}, HighMark{};
        std::vector<void *> FreeList{};
        double MemoryLimit{1e200}, TotalMemory{}, TotalHighMark{};
        TMemoryReportProc MemoryReportProc{};
        int showOSMem{};

        void* GetBigBlock();
        void ReleaseBigBlock(void* P);
        void ReduceMemorySize(int64_t Delta);
        void IncreaseMemorySize(int64_t Delta);
    public:
        explicit TBigBlockMgr(std::string Name);
        ~TBigBlockMgr();
        double MemoryUsedMB();
        double MemoryLimitMB();
        void XClear();
        std::string GetName() const;
        void SetOSMemory(int v);
    };

    class THeapMgr {
        PLargeBlock WorkBuffer;
        TBigBlockMgr BlockMgr;
        std::array<TSlotRecord, LastSlot> Slots;
        int64_t HighMark, OtherMemory, OtherGet, OtherFree;
        int64_t OtherGet64, OtherFree64;
        int64_t ReAllocCnt, ReAllocUsed, ReAllocCnt64, ReAllocUsed64;
        gdlib::gmsobj::TXList<void *> WrkBuffs, Active;
        std::string spName;

        // ...
    };

};
