#pragma once

#include <cstdint>
#include <string>
#include "datastorage.h"
#include "../utils.h"

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace gdlib::gmsobj {

    template<typename T>
    class TXList {
        int FCapacity;
        size_t FListMemory;

        T *Get(int Index) {
            return FList[Index - (OneBased ? 1 : 0)];
        }

        void Put(int Index, T *Item) {
            FreeItem(Index);
            FList[Index-(OneBased?1:0)] = Item;
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

        bool OneBased;
    public:
        TXList() :
            FCapacity{},
            FListMemory{},
            FCount{},
            FList{},
            OneBased{}
        {
        }

        virtual ~TXList() {
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
                std::memcpy(&FList[Index], &FList[Index+1], (FCount-Index)*sizeof(T*));
            }
        }

        T *Extract(T *Item) {
            T *res{};
            int I{IndexOf(Item)};
            if(OneBased) I--;
            if(I >= 0) {
                res = Item;
                // Delete item, do not call FreeItem
                FCount--;
                if(I < FCount)
                    std::memcpy(&FList[I], &FList[I+1], (FCount-I)*sizeof(T *));
            }
            return res;
        }

        int IndexOf(const T *Item) const {
            for(int N{}; N<FCount; N++)
                if(FList[N] == Item)
                    return N+(OneBased ? 1 : 0);
            return -1;
        }

        void Insert(int Index, T *Item) {
            if(FCount == FCapacity) Grow();
            if(OneBased) Index--;
            if(Index<FCount)
                std::memcpy(&FList[Index+1], &FList[Index], (FCount-Index)*sizeof(T*));
            FList[Index] = Item;
            FCount++;
        }

        int Remove(const T *Item) {
            int res{FCount-1};
            while(res >= 0 && FList[res] != Item) res--;
            if(res >= (OneBased ? 1 : 0)) Delete(res);
            return res;
        }

        int GetCapacity() const {
            return FCapacity;
        }

        void SetCapacity(int NewCapacity) {
            if (NewCapacity == FCapacity) return;
            else if (NewCapacity < FCount) NewCapacity = FCount;
            FListMemory = sizeof(T*) * NewCapacity;
            if (!FList) FList = (T**)std::malloc(FListMemory);
            else if (!NewCapacity) std::free(FList);
            else FList = (T**)std::realloc(FList, FListMemory);
            FCapacity = NewCapacity;
        }

        int GetCount() const {
            return FCount;
        }

        int size() const {
            return FCount;
        }

        bool empty() const {
            return !FCount;
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

    inline char *NewString(const std::string &s, size_t &memSize) {
        memSize = s.length()+1;
        char *buf {new char[memSize]};
        memcpy(buf, s.c_str(), memSize);
        return buf;
    }

    class TXStrings : public TXList<char> {
    private:
        size_t FStrMemory;

        void Put(int Index, const std::string &Item) {
            FreeItem(Index);
            FList[Index-(OneBased ? 1 : 0)] = NewString(Item, FStrMemory);
        }

        std::string Get(int Index) {
            return FList[Index-(OneBased ? 1 : 0)];
        }

    protected:
        void FreeItem(int Index) override {
            delete [] FList[Index];
        }

    public:
        TXStrings() : FStrMemory{} {}
        virtual ~TXStrings() {
            Clear();
        }

        int Add(const std::string &Item) {
            return TXList<char>::Add(NewString(Item, FStrMemory));
        }

        int IndexOf(const std::string &Item) {
            for(int N{}; N<FCount; N++)
                if(utils::sameText(FList[N], Item))
                    return N + (OneBased ? 1 : 0);
            return -1;
        }

        std::string operator[](int Index) {
            return Get(Index);
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