#pragma once

#include "../global/delphitypes.h"
#include "../gdlib/gmsobj.h"

namespace gdlib::gmsheapnew {

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

    struct TLargeBlock {
        int FreeSlots;
        void *InitialPtr, *CurrPtr;
    };
    using PLargeBlock = TLargeBlock;

    class TBigBlockMgr {
        int64_t OtherMemory, HighMark;
        gdlib::gmsobj::TXList<void *> FreeList;
        double MemoryLimit, TotalMemory, TotalHighMark;

        int showOSMem;

        // ...

    public:
        TBigBlockMgr(const std::string &Name) {}
        ~TBigBlockMgr() = default;

        double MemoryUsedMB() {
            return 0.0;
        }

        // ...
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
