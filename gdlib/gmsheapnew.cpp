#include "gmsheapnew.h"

#include <utility>

namespace gdlib::gmsheapnew {

    void* TBigBlockMgr::GetBigBlock() { return nullptr; }

    void TBigBlockMgr::ReleaseBigBlock(void* P) {}

    void TBigBlockMgr::ReduceMemorySize(int64_t Delta) {
        OtherMemory -= Delta;
        TotalMemory -= (double)Delta;
        if(MemoryReportProc) MemoryReportProc(MemoryUsedMB());
    }

    void TBigBlockMgr::IncreaseMemorySize(int64_t Delta) {}

    TBigBlockMgr::TBigBlockMgr(std::string  Name) : spName{std::move(Name)} {
    }

    TBigBlockMgr::~TBigBlockMgr() {
        XClear();
    }

    double TBigBlockMgr::MemoryUsedMB() {
        return 0.0;
    }

    double TBigBlockMgr::MemoryLimitMB() {
        return 0.0;
    }

    void TBigBlockMgr::XClear() {
        for(int N{}; N<FreeList.size(); N++)
            std::free(*FreeList[N]);
        ReduceMemorySize(FreeList.size() * BIGBLOCKSIZE);
        FreeList.Clear();
    }

    std::string TBigBlockMgr::GetName() const { return spName; }

    void TBigBlockMgr::SetOSMemory(int v) {
        showOSMem = v;
    }

}