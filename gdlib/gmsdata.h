#pragma once

#include "../expertapi/gclgms.h"

#include <map>
#include <string>
#include <array>
#include <vector>

#include "gmsobj.h"

namespace gdlib::gmsdata {
    using IndexKeys = std::array<int, GLOBAL_MAX_INDEX_DIM>;
    using ValueFields = std::array<double, GMS_VAL_MAX>;

    // TODO: The port of this class uses C++ standard library collections instead of Paul's custom GAMS colections
    // evalute performance impact of this choice!
    class TTblGamsData {
        std::map<IndexKeys, ValueFields> mapping;
        std::vector<IndexKeys> keyset;
    public:
        void GetRecord(int N, int* Inx, int InxCnt, double* Vals);
        ValueFields& operator[](const IndexKeys& Key);
        void clear();
        int size() const;
        std::map<IndexKeys, ValueFields>::iterator begin();
        std::map<IndexKeys, ValueFields>::iterator end();
        bool empty() const;
        void sort();
        int MemoryUsed() const {
            // FIXME: Return actual value!
            return 0;
        }
    };

    const int BufSize = 1024 * 16;

    struct TGADataBuffer {
        // filler is needed, so a buffer can start on 8 byte boundary
        int BytesUsed{}, filler{};
        std::array<uint8_t, BufSize> Buffer{};
    };

    template<typename T>
    class TGrowArrayFxd {
        TGADataBuffer** PBase{}; // dynamic heap array of pointers
        TGADataBuffer*  PCurrentBuf{};
        int BaseAllocated{}, BaseUsed{ -1 }, FSize, FStoreFact;
    protected:
        int64_t FCount{};
    public:
        explicit TGrowArrayFxd() :
            FSize{ sizeof(T) },
            FStoreFact{ BufSize / FSize }
        {}

        virtual ~TGrowArrayFxd() {
            Clear();
        }

        void Clear() {
            while (BaseUsed >= 0) {
                delete PBase[BaseUsed];
                BaseUsed--;
            }
            std::free(PBase);
            PBase = nullptr;
            BaseAllocated = 0;
            PCurrentBuf = nullptr;
            FCount = 0;
        }

        T* ReserveMem() {
            if (!PCurrentBuf || PCurrentBuf->BytesUsed + FSize > BufSize) {
                BaseUsed++;
                if (BaseUsed >= BaseAllocated) {
                    if (!BaseAllocated) BaseAllocated = 32;
                    else BaseAllocated *= 2;
                    size_t newByteCount{ BaseAllocated * sizeof(uint8_t*) };
                    if (!PBase) PBase = (TGADataBuffer**)std::malloc(newByteCount);
                    else PBase = (TGADataBuffer**)std::realloc(PBase, newByteCount);
                }
                PCurrentBuf = new TGADataBuffer;
                assert(BaseUsed >= 0);
                if (BaseUsed >= 0)
                    PBase[BaseUsed] = PCurrentBuf;
                PCurrentBuf->BytesUsed = 0;
            }
            auto res = (T*)&PCurrentBuf->Buffer[PCurrentBuf->BytesUsed];
            PCurrentBuf->BytesUsed += FSize;
            FCount++;
            return res;
        }

        T* ReserveAndClear() {
            T* res = ReserveMem();
            memset(res, 0, FSize);
            return res;
        }

        T* AddItem(const T* R) {
            auto* res = ReserveMem();
            std::memcpy(res, R, FSize);
            return res;
        }

        T* GetItemPtrIndex(int N) {
            return (T*)&PBase[N / FStoreFact]->Buffer[(N % FStoreFact) * FSize];
        }

        T* GetItemPtrIndexConst(int N) const {
            return (T*)&PBase[N / FStoreFact]->Buffer[(N % FStoreFact) * FSize];
        }

        void GetItem(int N, T** R) {
            T* PB = GetItemPtrIndex(N);
            std::memcpy(R, PB, FSize);
        }

        int64_t MemoryUsed() const {
            return !PCurrentBuf ? 0 : (int64_t)(BaseAllocated * sizeof(uint8_t*) + BaseUsed * BufSize + PCurrentBuf->BytesUsed);
        }

        int64_t GetCount() const {
            return FCount;
        }

        int64_t size() const {
            return FCount;
        }

        T *operator[](int N) const {
            return GetItemPtrIndexConst(N);
        }
    };

    class TXIntList : public TGrowArrayFxd<int> {
        int &GetItems(int Index) const {
            return *GetItemPtrIndexConst(Index);
        }

        void SetItems(int Index, int V) {
            while(Index >= FCount) ReserveAndClear();
            *GetItemPtrIndex(Index) = V;
        }

    public:
        TXIntList() = default;
        ~TXIntList() override = default;

        int Add(int Item) {
            int res{(int)FCount};
            AddItem(&Item);
            return res;
        }

        void Exchange(int Index1, int Index2) {
            int *p1 {GetItemPtrIndex(Index1)}, *p2 {GetItemPtrIndex(Index2)};
            int t{*p1};
            *p1 = *p2;
            *p2 = t;
        }

        int &operator[](int Index) const {
            return GetItems(Index);
        }

        int &operator[](int Index) {
            while(Index >= FCount) ReserveAndClear();
            return *GetItemPtrIndex(Index);
        }

    };

    // FIXME: Work in progress!
    /*class TTblGamsDataLegacy {
        TGrowArrayFxd<void*> DS;
        gdlib::gmsobj::TXList<uint8_t> FList;
        int FDim, FIndexSize, FDataSize;
        bool FIsSorted;
        int FLastIndex;

    public:
    };*/
}
