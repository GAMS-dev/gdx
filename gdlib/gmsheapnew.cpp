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

    std::string TBigBlockMgr::GetName() const {
        return spName;
    }

    void TBigBlockMgr::SetOSMemory(int v) {
        showOSMem = v;
    }

}