#pragma once

#include <cstdint>
#include "datastorage.h"

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace gdlib::gmsobj {

    template<typename T>
    class TXList {
        bool OneBased;
        int FCapacity;
        size_t FListMemory;

        T *Get(int Index) {
            return FList[Index - (OneBased ? 1 : 0)];
        }

        void Put(int Index, T *Item) {
            FreeItem(Index);
            FList[Index-(OneBased?1:0)] = Item;
        }

        void SetCapacity(int NewCapacity) {
            if(NewCapacity == FCapacity) return;
            else if(NewCapacity < FCount) NewCapacity = FCount;
            FListMemory = sizeof(T *) * NewCapacity;
            if(!FList) FList = (T **)std::malloc(FListMemory);
            else if(!NewCapacity) std::free(FList);
            else FList = (T **)std::realloc(FList, FListMemory);
            FCapacity = NewCapacity;
        }

        void SetCount(int NewCount) {
            if(NewCount != FCount) {
                if(NewCount > FCapacity) SetCapacity(NewCount);
                if(NewCount > FCount) std::memset(&FList[FCount], 0, (NewCount-FCount)*sizeof(T*));
                else for(int i{FCount-1}; i>=NewCount; i--) FreeItem(i);
                FCount = NewCount;
            }
        }

    protected:
        int FCount;
        T **FList;

        virtual void Grow() {
            int delta { FCapacity >= 1024 * 1024 ? FCapacity / 4 : (!FCapacity ? 16 : 7 * FCapacity) };
            int64_t i64 = FCapacity;
            i64 += delta;
            if(i64 <= std::numeric_limits<int>::max()) SetCapacity((int)i64);
            else {
                delta = std::numeric_limits<int>::max();
                if(FCapacity < delta) SetCapacity(delta);
                else assert(i64 <= std::numeric_limits<int>::max() && "TXList.grow(): max capacity reached");
            }
        }

        virtual void FreeItem(int Index) {
            // No-op
        }

    public:
        TXList() :
            OneBased{},
            FCapacity{},
            FListMemory{},
            FCount{},
            FList{}
        {
        }

        ~TXList() {
            Clear();
        }

        int Add(T *Item) {
            int res{FCount};
            if(res == FCapacity) Grow();
            FList[res] = Item;
            FCount++;
            if(OneBased) res++;
            return res;
        }

        void Clear() {
            for(int N{FCount-1+(OneBased?1:0)}; N>=(OneBased?1:0); N--) FreeItem(N);
            FCount = 0;
            SetCapacity(0);
        }

        void Delete(int Index) {
            FreeItem(Index);
            FCount--;
            if(Index<FCount) {
                if(OneBased) Index--;
                std::memcpy(FList[Index], FList[Index+1], (FCount-Index)*sizeof(T*));
            }
        }

        int GetCapacity() const {
            return FCapacity;
        }

        int GetCount() const {
            return FCount;
        }

        T *operator[](int Index) {
            return Get(Index);
        }

        T *GetLast() {
            return FCount <= 0 ? nullptr : FList[FCount-1];
        }

        size_t GetMemoryUsed() {
            return FListMemory;
        }
    };

    class TBooleanBitArray {
        uint8_t *PData;
        int FAllocated, FHighIndex;

        static void GetBitMask(int V, int &N, uint8_t &M) {
            N = V >> 3;
            M = 1 << (V & 0x7);
        }

    public:
        TBooleanBitArray() : PData{}, FAllocated{}, FHighIndex{-1} {
        }

        ~TBooleanBitArray() {
            if(FAllocated > 0)
                delete [] PData;
        }

        bool GetBit(int N) const {
            if(N < 0 || N > FHighIndex) return false;
            int P;
            uint8_t M;
            GetBitMask(N, P, M);
            return PData[P] & M;
        }

        void SetHighIndex(int V) {
            if(V > FHighIndex) {
                int NewMemSize {(V + 8) / 8};
                if(NewMemSize > FAllocated) {
                    int Delta{};
                    do {
                        if(!FAllocated) Delta += 256;
                        else if(FAllocated < 32 * 256) Delta += FAllocated;
                        else Delta += FAllocated / 4;
                    } while(NewMemSize >= FAllocated + Delta);
                    NewMemSize = FAllocated + Delta;
                    auto NewMem = new uint8_t[NewMemSize];
                    memset(NewMem, 0, NewMemSize);
                    if(FAllocated) {
                        memcpy(NewMem, PData, FAllocated);
                        delete [] PData;
                    }
                    PData = NewMem;
                    FAllocated = NewMemSize;
                }
                FHighIndex = V;
            }
        }

        int GetHighIndex() const {
            return FHighIndex;
        }

        void SetBit(int N, bool V) {
            if(N >= 0) {
                if(N > FHighIndex) {
                    if(!V) return;
                    SetHighIndex(N);
                }
                int P;
                uint8_t M;
                GetBitMask(N, P, M);
                if(V) PData[P] |= M;
                else PData[P] &= !M;
            }
        }

        int GetMemoryUsed() const {
            return FAllocated;
        }
    };

}