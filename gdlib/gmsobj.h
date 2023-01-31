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
                    std::memcpy(&FList[I], &FList[I+1], (FCount-I)*sizeof(T*));
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

        const T *GetConst(int Index) const {
            return FList[Index - (OneBased ? 1 : 0)];
        }

        T *GetLast() {
            return FCount <= 0 ? nullptr : FList[FCount-1];
        }

        size_t MemoryUsed() const {
            return FListMemory;
        }
    };

    inline char *NewString(const std::string &s, size_t &memSize) {
        auto l{s.length()+1};
        char *buf {new char[l]};
        std::memcpy(buf, s.c_str(), l);
        memSize += l;
        return buf;
    }

    inline char *NewString(const std::string &s) {
        size_t memSize;
        return NewString(s, memSize);
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
                if(utils::sameTextPChar(FList[N], Item.c_str()))
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
                        std::memcpy(NewMem, PData, FAllocated);
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

        int MemoryUsed() const {
            return FAllocated;
        }
    };

    class TQuickSortClass {
        void QuickSort(int L, int R);
    public:
        bool OneBased {};
        virtual void Exchange(int Index1, int Index2) = 0;
        virtual int Compare(int Index1, int Index2) = 0;
        void SortN(int n);
    };

    template<typename T>
    struct TStringItem {
        char *FString;
        T *FObject;
    };

    // This seems very much redundant to TXStrings
    // TODO: Should be refactored. Work out the differences to TXStrings and try merging code
    template<typename T>
    class TXCustomStringList : public TQuickSortClass {
        int FCount{};
        TStringItem<T> *FList{};
        int FCapacity{};
        size_t FStrMemory{}, FListMemory{};

        void SetName(int Index, const std::string &v) {
            char **sref = FList[Index-(OneBased?1:0)]->FString;
            delete [] *sref;
            *sref = NewString(v, FStrMemory);
        }

        void SetCapacity(int NewCapacity) {
            if(NewCapacity == FCapacity) return;
            if(NewCapacity < FCount) NewCapacity = FCount;
            FListMemory = sizeof(TStringItem<T>) * NewCapacity;
            if(!FList) FList = (TStringItem<T> *)std::malloc(FListMemory);
            else if(!FListMemory) { // TODO: Isn't std::realloc(FList, 0) doing the same?
                std::free(FList);
                FList = nullptr;
            }
            else FList = (TStringItem<T> *)std::realloc(FList, FListMemory);
            FCapacity = NewCapacity;
        }

    protected:
        char *GetName(int Index) {
            return FList[Index-(OneBased?1:0)];
        }

        T *GetObject(int Index) {
            return FList[Index-(OneBased?1:0)].FObject;
        }

        void PutObject(int Index, T *AObject) {
            FList[Index-(OneBased ? 1 : 0)].FObject = AObject;
        }

        virtual void Grow() {
            int delta{FCapacity >= 1024*1024 ? FCapacity / 4 : (!FCapacity ? 16 : 7 * FCapacity)};
            int64_t i64{FCapacity};
            i64 += delta;
            if(i64 <= std::numeric_limits<int>::max())
                SetCapacity((int)i64);
            else {
                delta = std::numeric_limits<int>::max();
                if(FCapacity < delta) SetCapacity(delta);
                else assert(i64 <= std::numeric_limits<int>::max() && "TXCustromStringList.grow(): max capacity reached");
            }
        }

        void FreeObject(int Index) {
            // noop
        }

        void InsertItem(int Index, const std::string &S, T *APointer) {
            if(FCount == FCapacity) Grow();
            if(OneBased) Index--;
            if(Index < FCount)
                std::memcpy(&FList[Index+1], &FList[Index], (FCount-Index)*sizeof(TStringItem<T>));
            FList[Index].FString = NewString(S, FStrMemory);
            FList[Index].FObject = APointer;
            FCount++;
        }

    public:
        ~TXCustomStringList() {
            Clear();
        }

        void Delete(int Index) {
            FreeItem(Index);
            if(OneBased) Index--;
            FCount--;
            if(Index < FCount)
                std::memcpy(&FList[Index], &FList[Index+1], (FCount-Index) * sizeof(TStringItem<T>));
        }

        void FreeItem(int Index) {
            delete [] FList[Index-(OneBased?1:0)].FString;
            FreeObject(Index);
        }

        void Clear() {
            for(int N{FCount-1+(OneBased?1:0)}; N >=(OneBased ? 1 : 0); N--)
                FreeItem(N);
            FCount = 0;
            SetCapacity(0);
        }

        int Add(const std::string &S) {
            return AddObject(S, nullptr);
        }

        int AddObject(const std::string &S, T *APointer) {
            int res{FCount+(OneBased?1:0)};
            InsertItem(res, S, APointer);
            return res;
        }

        int IndexOf(const std::string &S) {
            for(int N{}; N<FCount; N++)
                if(utils::sameTextPChar(FList[N].FString, S.c_str())) return N + (OneBased ? 1 : 0);
            return -1;
        }

        int IndexOfObject(const T &AObject) {
            for(int N{}; N<FCount; N++)
                if(FList[N].FObject == AObject)
                    return N+(OneBased?1:0);
            return -1;
        }

        char *GetName(int Index) const {
            return FList[Index].FString;
        }

        char *operator[](int Index) const  {
            return GetName(Index);
        }

        void Exchange(int Index1, int Index2) override {
            if(OneBased) {
                Index1--;
                Index2--;
            }
            TStringItem<T> Item = FList[Index1];
            std::memcpy(&FList[Index1], &FList[Index2], sizeof(TStringItem<T>));
            std::memcpy(&FList[Index2], &Item, sizeof(TStringItem<T>));
        }

        int Compare(int Index1, int Index2) override {
            char *s1 = FList[Index1-(OneBased?1:0)].FString;
            char *s2 = FList[Index2-(OneBased?1:0)].FString;
            return utils::sameText(s1, s2);
        }

        int Count() const {
            return FCount;
        }

        size_t MemoryUsed() const {
            return FListMemory + FStrMemory;
        }
    };

    const char NON_EMPTY {'='};
    const int   HASHMULT = 31,
                HASHMULT_6 = 887503681,
                HASHMULT2 = 71,
                HASHMOD2 = 32;
    const double    HASH2_MAXFULLRATIO = 0.75,
                    HASH2_NICEFULLRATIO = 0.55;
    const int   SCHASH_FACTOR_MAX = 13,
                SCHASH_FACTOR_MIN = 6;

    struct THashRecord {
        THashRecord *PNext;
        int RefNr;
    };
    using PHashRecord = THashRecord*;

    template<typename T>
    class TXHashedStringList : public TXCustomStringList<T> {
        PHashRecord *pHashSC{};
        int hashCount{}, trigger{-1};
        size_t hashBytes{};

        virtual int compareEntry(const std::string &s, int EN) {
            auto p { this->FList[EN].FString };
            return !p ? (!s.empty() ? 1 : 0) : utils::sameTextPChar(s.c_str(), p);
        }

        void ClearHashList() {
            if(pHashSC) {
                for(int n{}; n<hashCount; n++) {
                    PHashRecord p1 {pHashSC[n]};
                    pHashSC[n] = nullptr;
                    while(p1) {
                        auto p2 = p1->PNext;
                        delete p1;
                        p1 = p2;
                    }
                }
                //int64_t nBytes = sizeof(PHashRecord) * hashCount;
                delete pHashSC;
                pHashSC = nullptr;
                hashCount = 0;
                trigger = -1;
                hashBytes = 0;

            }
        }

        int getSCHashSize(int itemCount) {
            // ...
            return 0;
        }

        void setHashSize(int newCount) {
            int newSiz { newCount >= trigger ? getSCHashSize(newCount) : 0 };
            if(newSiz == hashCount) return; // no bump made
            hashCount = newSiz;
            int64_t i64 = hashCount * SCHASH_FACTOR_MAX;
            i64 = std::min<int64_t>(std::numeric_limits<int>::max(), i64);
            trigger = (int)i64;
            hashBytes = sizeof(PHashRecord) * hashCount;
            pHashSC = (PHashRecord *)std::malloc(hashBytes);
            std::memset(pHashSC, 0, hashBytes);


            // ...
        }

        virtual uint32_t hashValue(const std::string &s) {
            int64_t r {};
            int i{1}, n{(int)s.length()};
            while(i+5 <= n) {
                uint32_t t {(uint32_t)std::toupper(s[i++])};
                for(int j{}; j<5; j++)
                    t = (HASHMULT * t) + (uint32_t)std::toupper(s[i++]);
                r = (HASHMULT_6 * r + t) % hashCount;
            }
            while(i <= n)
                r = (HASHMULT * r + (uint32_t)std::toupper(s[i++])) % hashCount;
            return (uint32_t)r;
        }

    public:
        ~TXHashedStringList() {
            Clear();
        }

        void Clear() {
            ClearHashList();
            TXCustomStringList<T>::Clear();
        }

        int AddObject(const std::string &s, T *APointer) {
            if(!pHashSC || this->FCount > trigger) setHashSize(this->FCount);
            auto hv { hashValue(s) };
            PHashRecord PH;
            for(PH=pHashSC[hv]; PH && compareEntry(s, PH->RefNr); PH = PH->PNext);
            if(PH) return PH->RefNr + (this->OneBased?1:0);
            else {
                int res{this->FCount+(this->OneBased?1:0)};
                InsertItem(res, s, APointer);
                PH = new THashRecord;
                hashBytes += sizeof(THashRecord);
                PH->PNext = pHashSC[hv];
                PH->RefNr = res - (this->OneBased ? 1 : 0);
                pHashSC[hv] = PH;
                return res;
            }
        }

        int Add(const std::string &S) {
            return AddObject(S, nullptr);
        }
    };

    template<typename T>
    class TXStrPool : public TXHashedStringList<T> {
        int compareEntry(const std::string &s, int EN) override {
            auto p {this->FList[EN].FString};
            return !p ? (!s.empty() ? 1 : 0) : utils::sameTextPChar(s.c_str(), p, false);
        }

        uint32_t hashValue(const std::string &s) override {
            // TODO: How does this method differ from the base class implementation?
            return TXHashedStringList<T>::hashValue(s);
        }

    public:
        int Compare(int Index1, int Index2) override {
            char    *s1 {this->FList[Index1-(this->OneBased?1:0)].FString},
                    *s2 {this->FList[Index2-(this->OneBased?1:0)].FString};
            return utils::sameTextPChar(s1, s2, false);
        }
    };

}