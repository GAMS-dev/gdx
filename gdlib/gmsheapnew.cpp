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

    void TBigBlockMgr::ReduceMemorySize(uint64_t Delta) {
        OtherMemory -= Delta;
        TotalMemory -= (double)Delta;
        if(MemoryReportProc) MemoryReportProc(MemoryUsedMB());
    }

    void TBigBlockMgr::IncreaseMemorySize(uint64_t Delta) {
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
    double TBigBlockMgr::MemoryUsedMB() {
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
    double TBigBlockMgr::MemoryLimitMB() {
        return MemoryLimit / 1e6;
    }

    void TBigBlockMgr::XClear() {
        for(int N{}; N<FreeList.size(); N++)
            std::free(FreeList[N]);
        ReduceMemorySize(FreeList.size() * BIGBLOCKSIZE);
        FreeList.clear();
    }

    void TBigBlockMgr::GetBigStats(uint64_t& sizeOtherMemory, uint64_t& sizeHighMark, uint64_t& cntFree)
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

    void THeapMgr::ReduceMemorySize(uint64_t Delta)
    {
        BlockMgr.ReduceMemorySize(Delta);
        OtherMemory -= Delta;
    }

    void THeapMgr::IncreaseMemorySize(uint64_t Delta)
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
        for (THeapSlotNr Slot{ 1 }; Slot <= LastSlot; Slot++) {
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

    void* THeapMgr::prvXGetMem64(uint64_t Size)
    {
        return nullptr;
    }

    void THeapMgr::prvXFreeMem(void* P, int Size)
    {
    }

    void THeapMgr::prvXFreeMem64(void* P, int64_t Size)
    {
    }

    void THeapMgr::prvGetSlotCnts(THeapSlotNr Slot, uint64_t& cntGet, uint64_t cntFree, uint64_t cntAvail)
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
        return nullptr;
    }

    void THeapMgr::GMSFreeMem(void* p, uint16_t slot)
    {
    }

    void* THeapMgr::XGetMem(int Size)
    {
        return nullptr;
    }

    void THeapMgr::XGetMemNC(int Size)
    {
    }

    void* THeapMgr::XAllocMem(int Size)
    {
        return nullptr;
    }

    void* THeapMgr::XAllocMemNC(int Size)
    {
        return nullptr;
    }

    void THeapMgr::XGetMem64(uint64_t Size)
    {
    }

    void* THeapMgr::XAllocMem64(uint64_t Size)
    {
        return nullptr;
    }

    void THeapMgr::XFreeMem(void* P, int Size)
    {
    }

    void THeapMgr::XFreeMemNC(void* P, int Size)
    {
    }

    void THeapMgr::XFreeMemandNil(void** P, int Size)
    {
    }

    void THeapMgr::XFreeMem64(void* P, uint64_t Size)
    {
    }

    void THeapMgr::XFreeMem64andNil(void* P, uint64_t Size)
    {
    }

    void THeapMgr::XReAllocMem(void** P, int OldSize, int NewSize)
    {
    }

    void THeapMgr::XReAllocMemNC(void** P, int OldSize, int NewSize)
    {
    }

    void THeapMgr::XReAllocMem64(void** P, uint64_t OldSize, uint64_t NewSize)
    {
    }

    void THeapMgr::GetSlotCnts(THeapSlotNr Slot, uint64_t& cntGet, uint64_t& cntFree, uint64_t& cntAvail)
    {
    }

    void THeapMgr::GetBlockStats(uint64_t& cntWrkBuffs, uint64_t& cntActive, uint64_t& sizeOtherMemory, uint64_t& sizeHighMark)
    {
    }

    void THeapMgr::GetOtherStats(bool do64, uint64_t& cntGet, uint64_t& cntFree, uint64_t& cntReAlloc, uint64_t& sizeRUsed)
    {
    }

    uint64_t THeapMgr::GetFreeSlotSpace()
    {
        return 0;
    }

    bool THeapMgr::SetMemoryLimit(double limit)
    {
        return false;
    }

    void THeapMgr::SetMemoryReportProc(const TMemoryReportProc& F)
    {
    }

}