#include "gmsdata.h"
#include <algorithm>
#include <cstring>
#include "../expertapi/gclgms.h"

namespace gdlib::gmsdata {

    void TTblGamsData::GetRecord(int N, int * Inx,  int InxCnt, double * Vals) {
        const auto InxArr = keyset[N];
        memcpy(Inx, InxArr.data(), InxCnt * sizeof(int));
        memcpy(Vals, mapping[InxArr].data(), GMS_VAL_MAX * sizeof(double));
    }

    ValueFields &TTblGamsData::operator[](const IndexKeys &Key) {
        if (std::find(keyset.begin(), keyset.end(), Key) == keyset.end())
            keyset.push_back(Key);
        return mapping[Key];
    }

    void TTblGamsData::clear() {
        keyset.clear();
        mapping.clear();
    }

    int TTblGamsData::size() const {
        return (int)keyset.size();
    }

    std::map<IndexKeys, ValueFields>::iterator TTblGamsData::begin() {
        return mapping.begin();
    }

    std::map<IndexKeys, ValueFields>::iterator TTblGamsData::end() {
        return mapping.end();
    }

    bool TTblGamsData::empty() const {
        return keyset.empty();
    }

    void TTblGamsData::sort()
    {
        std::sort(keyset.begin(), keyset.end());
    }

    TGrowArrayFxd::TGrowArrayFxd(int ASize) : FSize{ASize}, FStoreFact{BufSize/FSize} {}

    TGrowArrayFxd::~TGrowArrayFxd() { Clear(); }

    void TGrowArrayFxd::Clear() {
        while(BaseUsed >= 0) {
            std::free(&PBase[BaseUsed]);
            BaseUsed--;
        }
        std::free(PBase);
        BaseAllocated = 0;
        PCurrentBuf = nullptr;
        FCount = 0;
    }

    void *TGrowArrayFxd::ReserveAndClear() {
        void *res = ReserveMem();
        memset(res, 0, FSize);
        return res;
    }

    void *TGrowArrayFxd::AddItem(const void *R) {
        auto *res = ReserveMem();
        std::memcpy(res, R, FSize);
        return res;
    }

    uint8_t *TGrowArrayFxd::GetItemPtrIndex(int N) {
        return &PBase[N / FStoreFact].Buffer[(N % FStoreFact) * FSize];
    }

    void TGrowArrayFxd::GetItem(int N, void **R) {
        auto PB = GetItemPtrIndex(N);
        std::memcpy(R, PB, FSize);
    }

    int64_t TGrowArrayFxd::MemoryUsed() const {
        return !PCurrentBuf ? 0 : (int64_t)(BaseAllocated * sizeof(uint8_t *) + BaseUsed * BufSize + PCurrentBuf->BytesUsed);
    }

    int64_t TGrowArrayFxd::GetCount() const {
        return FCount;
    }

    void *TGrowArrayFxd::ReserveMem() {
        if(!PCurrentBuf || PCurrentBuf->BytesUsed + FSize > BufSize) {
            BaseUsed++;
            if(BaseUsed >= BaseAllocated) {
                if(!BaseAllocated) BaseAllocated = 32;
                else BaseAllocated  *= 2;
                PBase = (TGADataBuffer *)std::realloc(PBase, BaseAllocated * sizeof(uint8_t *));
            }
            PCurrentBuf = (TGADataBuffer *)std::malloc(sizeof(TGADataBuffer));
            assert(BaseUsed >= 0);
            if(BaseUsed >= 0)
                PBase[BaseUsed] = *PCurrentBuf;
            PCurrentBuf->BytesUsed = 0;
        }
        void *res = &PCurrentBuf->Buffer[PCurrentBuf->BytesUsed];
        PCurrentBuf->BytesUsed += FSize;
        FCount++;
        return res;
    }
}