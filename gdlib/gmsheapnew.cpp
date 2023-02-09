#include "gmsheapnew.h"

#include <utility>
#include <string>

#include "../rtl/p3utils.h"

using namespace std::literals::string_literals;

namespace gdlib::gmsheapnew {

    void* TBigBlockMgr::GetBigBlock() {
        void *res {FreeList.back()};
        if(res) {
            FreeList.pop_back();
            return res;
        }
        else {
            IncreaseMemorySize(BIGBLOCKSIZE);
            return std::malloc(BIGBLOCKSIZE);
        }
    }

    void TBigBlockMgr::ReleaseBigBlock(void* P) {
        FreeList.push_back(P);
    }

    void TBigBlockMgr::ReduceMemorySize(int64_t Delta) {
        OtherMemory -= Delta;
        TotalMemory -= (double)Delta;
        if(MemoryReportProc) MemoryReportProc(MemoryUsedMB());
    }

    void TBigBlockMgr::IncreaseMemorySize(int64_t Delta) {
        if(TotalMemory + (double)Delta > MemoryLimit)
            throw std::runtime_error("Requested memory exceeds assigned HeapLimit"s);
        OtherMemory += Delta;
        if(OtherMemory > HighMark) HighMark = OtherMemory;
        TotalMemory += (double)Delta;
        if(TotalMemory > TotalHighMark) TotalHighMark = TotalMemory;
        if(MemoryReportProc) MemoryReportProc(MemoryUsedMB());
    }

    TBigBlockMgr::TBigBlockMgr(std::string  Name) : spName{std::move(Name)} {
    }

    TBigBlockMgr::~TBigBlockMgr() {
        XClear();
    }

    // Brief:
    //   Return the amount of memory used by all active heaps. The amount returned is the total memory use
    //   divided by 1,000,000
    double TBigBlockMgr::MemoryUsedMB() const {
        int64_t rss{}, vss{};
        if(showOSMem == 1 && rtl::p3utils::p3GetMemoryInfo(rss, vss))
            return (double)rss / 1e6;
        else if(showOSMem == 2 && rtl::p3utils::p3GetMemoryInfo(rss, vss))
            return (double)vss / 1e6;
        else
            return TotalMemory/1e6;
    }

    // Brief:
    //   Returns the current memory limit specified by calling SetMemoryLimit
    // Returns:
    //   Returns the value specified when calling SetMemoryLimit divided by 1,000,000
    double TBigBlockMgr::MemoryLimitMB() const {
        return MemoryLimit / 1e6;
    }

    void TBigBlockMgr::XClear() {
        for(int N{}; N<FreeList.size(); N++)
            std::free(FreeList[N]);
        ReduceMemorySize(FreeList.size() * BIGBLOCKSIZE);
        FreeList.clear();
    }

    void TBigBlockMgr::GetBigStats(int64_t& sizeOtherMemory, int64_t& sizeHighMark, int64_t& cntFree)
    {
        sizeOtherMemory = OtherMemory;
        sizeHighMark = HighMark;
        cntFree = FreeList.size();
    }

    std::string TBigBlockMgr::GetName() const {
        return spName;
    }

    void TBigBlockMgr::SetOSMemory(int v) {
        showOSMem = v;
    }

    PLargeBlock THeapMgr::GetWorkBuffer()
    {
        void* p = BlockMgr.GetBigBlock();
        return new TLargeBlock{BIGBLOCKSIZE / HEAPGRANULARITY, p, p };
    }

    void THeapMgr::ReleaseWorkBuffer(PLargeBlock P)
    {
        BlockMgr.ReleaseBigBlock(P->InitialPtr);
        const auto pit = std::find(WrkBuffs.begin(), WrkBuffs.end(), P);
        if (pit != WrkBuffs.end()) WrkBuffs.erase(pit);
        std::free(P);
    }

    void THeapMgr::ReduceMemorySize(int64_t Delta)
    {
        BlockMgr.ReduceMemorySize(Delta);
        OtherMemory -= Delta;
    }

    void THeapMgr::IncreaseMemorySize(int64_t Delta)
    {
        BlockMgr.IncreaseMemorySize(Delta);
        OtherMemory += Delta;
        if (OtherMemory > HighMark) HighMark = OtherMemory;
    }

    void THeapMgr::prvClear()
    {
        while (!WrkBuffs.empty())
            ReleaseWorkBuffer(WrkBuffs.back());
        WrkBuffs.clear();
        WorkBuffer = nullptr;
        for (const auto act : Active)
            std::free(act);
        Active.clear();
        for (THeapSlotNr Slot{ 1 }; Slot <= LastSlot; ++Slot) {
            auto& obj = Slots[Slot];
            obj.GetCount = obj.FreeCount = obj.ListCount = 0;
            obj.FirstFree = nullptr;
        }
        ReduceMemorySize(OtherMemory);
        OtherGet = OtherFree = 0;
        OtherGet64 = OtherFree64 = 0;
        ReAllocCnt = ReAllocUsed = 0;
        ReAllocCnt64 = ReAllocUsed64 = 0;
    }

    void* THeapMgr::prvGMSGetMem(uint16_t slot)
    {
        return nullptr;
    }

    void THeapMgr::prvGMSFreeMem(void* p, uint16_t slot)
    {
    }

    void* THeapMgr::prvXGetMem(int Size)
    {
        return nullptr;
    }

    void* THeapMgr::prvXGetMemNC(int Size)
    {
        return nullptr;
    }

    void* THeapMgr::prvXGetMem64(int64_t Size)
    {
        if(Size <= 0)
            return nullptr;
        else if(Size <= LastSlot * HEAPGRANULARITY)
            return prvGMSGetMem((uint16_t)((Size-1) / HEAPGRANULARITY + 1));
        else {
            OtherGet64++;
            IncreaseMemorySize(Size);
            Active.push_back(std::malloc(Size));
            return Active.back();
        }
    }

    void THeapMgr::prvXFreeMem(void* P, int Size)
    {
    }

    void THeapMgr::prvXFreeMem64(void* P, int64_t Size)
    {
    }

    void THeapMgr::prvGetSlotCnts(THeapSlotNr Slot, int64_t& cntGet, int64_t &cntFree, int64_t &cntAvail)
    {
    }

    THeapMgr::THeapMgr(const std::string& Name) : BlockMgr{"BBMgr_"s + Name}, spName{Name} {
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

    void* THeapMgr::GMSGetMem(uint16_t slot)
    {
#ifdef BYPASSHEAPMGR
        return std::malloc(Slot * HEAPGRANULARITY);
#else
        return prvGMSGetMem(slot);
#endif
    }

    void THeapMgr::GMSFreeMem(void* p, uint16_t slot)
    {
#ifdef BYPASSHEAPMGR
        std::free(p);
#else
        prvGMSFreeMem(p, slot);
#endif
    }

    void* THeapMgr::XGetMem(int Size)
    {
#ifdef BYPASSHEAPMGR
        return std::malloc(Size);
#else
        return prvXGetMem(Size);
#endif
    }

    void *THeapMgr::XGetMemNC(int Size)
    {
#ifdef BYPASSHEAPMGR
        return std::malloc(Size);
#else
        return prvXGetMemNC(Size);
#endif
    }

    void* THeapMgr::XAllocMem(int Size)
    {
#ifdef BYPASSHEAPMGR
        return std::malloc(Size);
#else
        void *res{prvXGetMem(Size)};
        if(res) std::memset(res, 0, Size);
        return res;
#endif
    }

    void* THeapMgr::XAllocMemNC(int Size)
    {
#ifdef BYPASSHEAPMGR
        return std::malloc(Size);
#else
        void *res{prvXGetMemNC(Size)};
        if(res) std::memset(res, 0, Size);
        return res;
#endif
    }

    void *THeapMgr::XGetMem64(int64_t Size)
    {
#ifdef BYPASSHEAPMGR
        return std::malloc(Size);
#else
        return prvXGetMem64(Size);
#endif
    }

    void* THeapMgr::XAllocMem64(int64_t Size)
    {
#ifdef BYPASSHEAPMGR
        return std::malloc(size);
#else
        void *res{ prvXGetMem64(Size) };
        if(res)
            std::memset(res, 0, Size);
        return res;
#endif
    }

    void THeapMgr::XFreeMem(void* P, int Size)
    {
#ifdef BYPASSHEAPMGR
        if(Size > 0) std::free(P);
#else
        prvXFreeMem(P, Size);
#endif
    }

    void THeapMgr::XFreeMemNC(void* P, int Size)
    {
#ifdef BYPASSHEAPMGR
        std::free(P);
#else
        OtherFree++;
        ReduceMemorySize(Size);
        std::free(P);
#endif
    }

    void THeapMgr::XFreeMemAndNil(void** P, int Size)
    {
#ifdef BYPASSHEAPMGR
        if(Size > 0) std::free(*P);
#else
        prvXFreeMem(*P, Size);
#endif
        *P = nullptr;
    }

    void THeapMgr::XFreeMem64(void* P, int64_t Size)
    {
#ifdef BYPASSHEAPMGR
        if(Size > 0) std::free(P);
#else
        prvXFreeMem64(P, Size);
#endif
    }

    void THeapMgr::XFreeMem64andNil(void** P, int64_t Size)
    {
#ifdef BYPASSHEAPMGR
        if(Size > 0) std::free(*P);
#else
        prvXFreeMem64(*P, Size);
#endif
        *P = nullptr;
    }

    void THeapMgr::XReAllocMem(void** P, int OldSize, int NewSize)
    {
        void *PNew{};
#ifdef BYPASSHEAPMGR
        if(NewSize <= 0) {
            if(OldSize > 0 && P && *P) std::free(*P);
            *P = nullptr;
        }
        else if(!P || !*P || OldSize <= 0) {
            P = std::malloc(NewSize);
        } else P = std::realloc(P, NewSize);
#else
        ReAllocCnt++;
        ReAllocUsed -= OldSize;
        ReAllocUsed += NewSize;
        if(NewSize <= 0) {
            if(OldSize > 0 && P) prvXFreeMem(P, OldSize);
            PNew = nullptr;
        } else if(!P || OldSize <= 0) {
            PNew = P;

            if(NewSize > OldSize) IncreaseMemorySize(NewSize-OldSize);
            else ReduceMemorySize(OldSize-NewSize);
        } else if(OldSize == NewSize) {

        } else if(OldSize > LastSlot * HEAPGRANULARITY && NewSize > LastSlot * HEAPGRANULARITY) {

        } else {

        }
        *P = PNew;
#endif
    }

    void THeapMgr::XReAllocMemNC(void** P, int OldSize, int NewSize)
    {
#ifdef BYPASSHEAPMGR
        if(NewSize <= 0) {
            if(OldSize > 0 && P && *P) std::free(*P);
            *P = nullptr;
        } else if(!P || OldSize <= 0)
            *P = std::malloc(NewSize);
        else
            *P = std::realloc(P, NewSize);
#else
        ReAllocCnt++;
        ReAllocUsed -= OldSize;
        ReAllocUsed += NewSize;
        *P = std::realloc(P, NewSize);
#endif
    }

    void THeapMgr::XReAllocMem64(void** P, int64_t OldSize, int64_t NewSize)
    {
        void *PNew{};
#ifdef BYPASSHEAPMGR
        if(NewSize <= 0) {
            if(OldSize > 0 && P) std::free(P);
            P = nullptr;
        }
        else if(!P || OldSize <= 0)
            PNew = std::malloc(NewSize);
        else {
            PNew = std::malloc(NewSize);
            std::memcpy(PNew, P, OldSize <= NewSize ? OldSize : NewSize);
            std::free(P);
        }
#else
        ReAllocCnt64++;
        ReAllocUsed64 -= OldSize;
        ReAllocUsed64 += NewSize;
        if(NewSize <= 0) {
            if(OldSize > 0 && P) prvXFreeMem64(P, OldSize);
            PNew = nullptr;
        }
        else if(!P || OldSize <= 0) {
            PNew = prvXGetMem64(NewSize);
        }
        else if(OldSize == NewSize) PNew = P;
        else if(OldSize > LastSlot * HEAPGRANULARITY && NewSize > LastSlot * HEAPGRANULARITY) {
            PNew = P;
            Active.erase(std::find(Active.begin(), Active.end(), P));
            PNew = std::realloc(PNew, NewSize);
            Active.push_back(PNew);
            if(NewSize > OldSize) IncreaseMemorySize(NewSize - OldSize);
            else ReduceMemorySize(OldSize - NewSize);
        }
        else {
            PNew = prvXGetMem64(NewSize);
            std::memcpy(PNew, P, OldSize <= NewSize ? OldSize : NewSize);
            prvXFreeMem64(P, OldSize);
        }
#endif
        *P = PNew;
    }

    void THeapMgr::GetSlotCnts(THeapSlotNr Slot, int64_t& cntGet, int64_t& cntFree, int64_t& cntAvail) {
        prvGetSlotCnts(Slot, cntGet, cntFree, cntAvail);
    }

    void THeapMgr::GetBlockStats(int64_t& cntWrkBuffs, int64_t& cntActive, int64_t& sizeOtherMemory, int64_t& sizeHighMark) {
        cntWrkBuffs = (int64_t)WrkBuffs.size();
        cntActive = (int64_t)Active.size();
        sizeOtherMemory = OtherMemory;
        sizeHighMark = HighMark;
    }

    void THeapMgr::GetOtherStats(bool do64, int64_t& cntGet, int64_t& cntFree, int64_t& cntReAlloc, int64_t& sizeRUsed) {
        if(do64) {
            cntGet = OtherGet64;
            cntFree = OtherFree64;
            cntReAlloc = ReAllocCnt64;
            sizeRUsed = ReAllocUsed64;
        } else {
            cntGet = OtherGet;
            cntFree = OtherFree;
            cntReAlloc = ReAllocCnt;
            sizeRUsed = ReAllocUsed;
        }
    }

    int64_t THeapMgr::GetFreeSlotSpace() {
        int64_t res{}, cntGet, cntFree, cntAvail;
        for(THeapSlotNr Slot{THeapSlotNr::getLowerBound()}; Slot<=THeapSlotNr::getUpperBound(); ++Slot) {
            prvGetSlotCnts(Slot, cntGet, cntFree, cntAvail);
            int64_t sizeFree {cntAvail * Slot * HEAPGRANULARITY };
            res += sizeFree;
        }
        return res;
    }

    bool THeapMgr::SetMemoryLimit(double limit) {
        BlockMgr.MemoryLimit = limit;
        return limit >= BlockMgr.TotalMemory;
    }

    void THeapMgr::SetMemoryReportProc(const TMemoryReportProc& F) {
        BlockMgr.MemoryReportProc = F;
    }

    std::string THeapMgr::GetName() const {
        return spName;
    }

    TBigBlockMgr &THeapMgr::GetBBMgr() {
        return BlockMgr;
    }

}