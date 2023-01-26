#pragma once

#include "gmsstrm.h"
#include "../utils.h"
#include "datastorage.h"
#include "gmsdata.h"

#include <vector>
#include <string>
#include <cassert>
#include <numeric>

namespace gdlib::strhash {
    template<typename T>
    struct THashBucket {
        char *StrP {};
        THashBucket *NextBucket {};
        int StrNr {};
        T Obj {};
    };

    template<typename T>
    using PHashBucket = THashBucket<T>*;

    // TODO: This is still deviating significantly from the original P3-Implementation
    // Consider doing a more verbatim line-by-line port by also porting data structures like
    // gdlib/gmsdata/TGrowArrayFxd and gmsheapnew
    template<typename T>
    class TXStrHashList {
    protected:
#ifdef TLD_BATCH_ALLOCS
        datastorage::BatchAllocator<960> batchAllocator;
        datastorage::BatchAllocator<1024> batchStrAllocator;
#endif
        std::vector<PHashBucket<T>> Buckets {}; // sorted by order of insertion, no gaps
        std::unique_ptr<std::vector<PHashBucket<T>>> PHashTable {}; // sorted by hash value, with gaps
        std::unique_ptr<std::vector<int>> SortMap {};
        int HashTableSize {}, ReHashCnt {}, FCount {};
        bool FSorted {};

        void ClearHashTable() {
            PHashTable = nullptr;
            HashTableSize = ReHashCnt = 0;
        }

        void HashTableReset(int ACnt) {
            const int   HashSize_1 = 997,
                        HashSize_2 = 9973,
                        HashSize_3 = 99991,
                        HashSize_4 = 999979,
                        HashSize_5 = 9999991,
                        HashSize_6 = 99999989,
                        Next_1 = 1500,
                        Next_2 = 15000,
                        Next_3 = 150000,
                        Next_4 = 1500000,
                        Next_5 = 15000000,
                        Next_6 = std::numeric_limits<int>::max(); 
            if (ACnt >= Next_5) { HashTableSize = HashSize_6; ReHashCnt = Next_6; }
            else if(ACnt >= Next_4) { HashTableSize = HashSize_5; ReHashCnt = Next_5; }
            else if(ACnt >= Next_3) { HashTableSize = HashSize_4; ReHashCnt = Next_4; }
            else if(ACnt >= Next_2) { HashTableSize = HashSize_3; ReHashCnt = Next_3; }
            else if(ACnt >= Next_1) { HashTableSize = HashSize_2; ReHashCnt = Next_2; }
            else { HashTableSize = HashSize_1; ReHashCnt = Next_1; }
            PHashTable = std::make_unique<std::vector<PHashBucket<T>>>(HashTableSize);
            std::fill_n(PHashTable->begin(), HashTableSize, nullptr);
        }

        virtual int Hash(const std::string &s) {
            int res{};
            for(char c : s)
                res = 211 * res + toupper(c);
            return (res & 0x7FFFFFFF) % HashTableSize;
        }

        virtual bool EntryEqual(const char *ps1, const char *ps2) {
#if defined(_WIN32)
            return !_stricmp(ps1, ps2);
#else
            return !strcasecmp(ps1, ps2);
#endif
        }

        virtual bool EntryEqual(const std::string &ps1, const std::string &ps2) {
            return utils::sameText(ps1, ps2);
        }

        virtual int Compare(const std::string &ps1, const std::string &ps2) {
            return utils::strCompare(ps1, ps2);
        }

        void HashAll() {
            if(PHashTable) PHashTable->clear();
            HashTableReset(FCount);
            for(int N{}; N<FCount; N++) {
                auto &PBuck = Buckets[N];
                int HV = Hash(PBuck->StrP);
                PBuck->NextBucket = GetBucketByHash(HV);
                (*PHashTable)[HV] = PBuck;
            }
        }

        void SetObject(int N, T AObj) {
            Buckets[N-(OneBased ? 1 : 0)]->Obj = AObj;
        }

        void SetSortedObject(int N, T &AObj) {
            if(!FSorted) Sort();
            Buckets[(*SortMap)[N-(OneBased ? 1 : 0)]]->Obj = &AObj;
        }

        std::string &GetSortedString(int N) {
            if(FSorted) Sort();
            return Buckets[(*SortMap)[N-(OneBased ? 1 : 0)]]->StrP;
        }

        void QuickSort(int L, int R) {
            int i{L};
            while(i < R) {
                int j {R}, p {(L+R) >> 1};
                std::string pPivotStr = Buckets[(*SortMap)[p]]->StrP;
                do {
                    while(Compare(Buckets[(*SortMap)[i]]->StrP, pPivotStr) < 0) i++;
                    while(Compare(Buckets[(*SortMap)[j]]->StrP, pPivotStr) > 0) j--;
                    if(i < j) {
                        std::swap((*SortMap)[i], (*SortMap)[j]);
                        i++;
                        j--;
                    } else if(i == j) {
                        i++;
                        j--;
                    }
                } while(i <= j);
                // partition finished, now sort left and right
                // starting with the smaller piece to keep recursion in check
                if(j-L > R-i) { // left part is bigger, look right first
                    if(i < R) QuickSort(i, R); // if necessary, sort the right part
                    i = L; // and move to the left part
                    R = j;
                } else { // right part is bigger, look left first
                    if(L < j) QuickSort(L, j); // if necessary, sort the right part
                    L = i;
                }
            }
        }

        void Sort() {
            if(!SortMap) {
                SortMap = std::make_unique<std::vector<int>>(FCount);
                std::iota(SortMap->begin(), SortMap->end(), 0);
                FSorted = false;
            }
            if(!FSorted) {
                if(FCount >= 2) {
                    std::string PSN = Buckets[0]->StrP;
                    for(int N{}; N<FCount-1; N++) {
                        std::string PSN1 = Buckets[N+1]->StrP;
                        if(Compare(PSN, PSN1) > 0) {
                            QuickSort(0, FCount-1);
                            break;
                        }
                        PSN = PSN1;
                    }
                }
                FSorted = true;
            }
        }

    public:
        bool OneBased {}; // When false (default) indices are in the range 0..Count-1
        // when true, indices are in the range 1..Count

        TXStrHashList() {
            ClearHashTable();
        }

        virtual ~TXStrHashList() {
            Clear();
        }

        void Clear() {
#ifndef TLD_BATCH_ALLOCS
            for(auto bucket : Buckets) {
                delete[] bucket->StrP;
                delete bucket;
            }
#else
            batchAllocator.clear();
            batchStrAllocator.clear();
#endif
            Buckets.clear();
            FCount = 0;
            ClearHashTable();
            SortMap = nullptr;
            FSorted = false;
        }

        T *GetObject(int N) {
            return &Buckets[N-(OneBased ? 1 : 0)]->Obj;
        }

        inline PHashBucket<T> GetBucketByHash(int hash) {
            return (*PHashTable)[hash];
        }

        int StoreObject(const std::string& s, T AObj) {
            if (PHashTable) ClearHashTable();
#ifdef TLD_BATCH_ALLOCS
            PHashBucket<T> PBuck = reinterpret_cast<PHashBucket<T>>(batchAllocator.GetBytes(sizeof(THashBucket<T>)));
#else
            PHashBucket<T> PBuck = new THashBucket<T>{};
#endif
            Buckets.push_back(PBuck);
            PBuck->NextBucket = nullptr;
            PBuck->StrNr = FCount; // before it was added!
            int res{ FCount + (OneBased ? 1 : 0) };
            if (SortMap) {
                (*SortMap)[FCount] = FCount;
                FSorted = false;
            }
            FCount++; // ugly
#ifdef TLD_BATCH_ALLOCS
            PBuck->StrP = reinterpret_cast<char *>(batchStrAllocator.GetBytes(s.length()+1));
#else
            PBuck->StrP = new char[s.length()+1];
#endif
            utils::assignStrToBuf(s, PBuck->StrP, (int)s.length()+1);
            PBuck->Obj = std::move(AObj);
            return res;
        }

        int AddObject(const std::string &s, T AObj) {
            assert(FCount < std::numeric_limits<int>::max());
            if(FCount >= ReHashCnt) HashAll();
            int HV {Hash(s)};
            PHashBucket<T> PBuck = GetBucketByHash(HV);
            while(PBuck) {
                if(!EntryEqual(PBuck->StrP, s)) PBuck = PBuck->NextBucket;
                else return PBuck->StrNr + (OneBased ? 1 : 0);
            }
#ifdef TLD_BATCH_ALLOCS
            PBuck = reinterpret_cast<PHashBucket<T>>(batchAllocator.GetBytes(sizeof(THashBucket<T>)));
#else
            PBuck = new THashBucket<T>{};
#endif
            Buckets.push_back(PBuck);
            PBuck->NextBucket = GetBucketByHash(HV);
            (*PHashTable)[HV] = PBuck;
            PBuck->StrNr = FCount; // before it was added! zero based
            int res {FCount + (OneBased ? 1 : 0)};
            if(SortMap) {
                (*SortMap)[FCount] = FCount;
                FSorted = false;
            }
            FCount++; // ugly
#ifdef TLD_BATCH_ALLOCS
            PBuck->StrP = reinterpret_cast<char *>(batchStrAllocator.GetBytes(s.length()+1));
#else
            PBuck->StrP = new char[s.length()+1];
#endif
            utils::assignStrToBuf(s, PBuck->StrP, (int)s.length()+1);
            PBuck->Obj = std::move(AObj);
            return res;
        }

        virtual void FreeItem(int N) {
            // noop by default
        }

        int Add(const std::string &s) {
            return AddObject(s, nullptr);
        }

        int IndexOf(const std::string &s) {
            if(!PHashTable) HashAll();
            int HV {Hash(s)};
            PHashBucket<T> PBuck = GetBucketByHash(HV);
            while(PBuck) {
                if(!EntryEqual(PBuck->StrP, s)) PBuck = PBuck->NextBucket;
                else return PBuck->StrNr + (OneBased ? 1 : 0);
            }
            return -1;
        }

        template<typename T2>
        void LoadFromStream(T2 &s) {
            Clear();
            int Cnt{s.ReadInteger()};
            for(int N{}; N<Cnt; N++)
                StoreObject(s.ReadString(), nullptr);
        }

        template<typename T2>
        void SaveToStream(T2 &s) {
            s.WriteInteger(FCount);
            for(int N{OneBased?1:0}; N<FCount+(OneBased?1:0); N++)
                s.WriteString(GetString(N));
        }

        int GetStringLength(int N) {
            return GetString(N).length();
        }

        int64_t MemoryUsed() const {
            int64_t res{};
            for(int N{}; N<Count(); N++)
                res += strlen(Buckets[N]->StrP)+1;
            res += (int)(Buckets.size() * sizeof(THashBucket<T>));
            if(PHashTable) res += (int)(PHashTable->size() * sizeof(THashBucket<T>));
            if(SortMap) res += (int)(SortMap->size()*sizeof(int));
            return res;
        }

        void RenameEntry(int N, const std::string &s) {
            N -= OneBased ? 1 : 0;
            if(FSorted) {
                SortMap = nullptr;
                FSorted = false;
            }
            if(PHashTable) {
                int HV0{ Hash(GetString(N+1)) }, HV1 {Hash(s)};
                if(HV0 != HV1) {
                    PHashBucket<T> PrevBuck {}, PBuck;
                    for(PBuck = GetBucketByHash(HV0);
                        PBuck->StrNr != N; PBuck = PBuck->NextBucket)
                        PrevBuck = PBuck;
                    if(!PrevBuck) (*PHashTable)[HV0] = PBuck->NextBucket;
                    else PrevBuck->NextBucket = PBuck->NextBucket;
                    PBuck->NextBucket = GetBucketByHash(HV1);
                    (*PHashTable)[HV1] = PBuck;
                }
            }
            SetString(N+1, s);
        }

        T* operator[](int N) {
            return GetObject(N);
        }

        T& operator[](const std::string &key) {
            return *GetObject(IndexOf(key));
        }

        T* GetSortedObject(int N) {
            if(!FSorted) Sort();
            return Buckets[(*SortMap)[N-(OneBased ? 1 : 0)]].Obj;
        }

        const std::string GetString(int N) const {
            return Buckets[N-(OneBased ? 1 : 0)]->StrP;
        }

        void SetString(int N, const std::string s) {
            auto bucket = Buckets[N-(OneBased ? 1 : 0)];
#ifdef TLD_BATCH_ALLOCS
            // Storage for old string will leak temporarily but will be collected by batchAllocator.clear call
            bucket->StrP = reinterpret_cast<char *>(batchStrAllocator.GetBytes(s.length()+1));
#else
            delete [] bucket->StrP;
            bucket->StrP = new char[s.length()+1];
#endif
            utils::assignStrToBuf(s, bucket->StrP, (int)s.length()+1);
        }

        std::string GetSortedString(int N) const {
            if(!FSorted) Sort();
            return Buckets[(*SortMap)[N-(OneBased ? 1 : 0)]]->StrP;
        }

        int Count() const {
            return FCount;
        }

        int size() const {
            return FCount;
        }

        bool empty() const {
            return !FCount;
        }

        std::string GetString(int N) {
            return Buckets[N-(OneBased ? 1 : 0)]->StrP;
        }
    };

    // Specialization when it is not a pointer type
    template<>
    inline int TXStrHashList<uint8_t>::Add(const std::string &s) {
        return AddObject(s, 0);
    }

    template<>
    template<typename T2>
    inline void TXStrHashList<uint8_t >::LoadFromStream(T2 &s) {
        Clear();
        int Cnt{s.ReadInteger()};
        for(int N{}; N<Cnt; N++)
            StoreObject(s.ReadString(), 0);
    }

    template<typename T>
    class TXCSStrHashList : public TXStrHashList<T> {
    protected:
        int Hash(const std::string &s) override {
            int res{};
            for(char c : s)
                res = 211 * res + c;
            return (res & 0x7FFFFFFF) % this->HashTableSize;
        }

        bool EntryEqual(const std::string &ps1, const std::string &ps2) override {
            return ps1 == ps2;
        }

        bool EntryEqual(const char *ps1, const char *ps2) override {
            return !strcmp(ps1, ps2);
        }
    };

    // This port of the central hashmap class used by the GDX object is as
    // close to the original P3/Delphi implementation as possible
    // in order to hopefully match its performance and memory characteristics.
    template<typename T>
    class TXStrHashListLegacy {
        THashBucket<T> **PHashTable{};
        std::unique_ptr<gdlib::gmsdata::TXIntList> SortMap{};
        int64_t SizeOfHashTable{};
        int HashTableSize{}, ReHashCnt{};
        bool FSorted{};

        void ClearHashTable() {
            if(PHashTable) std::free(PHashTable);
            PHashTable = nullptr;
            HashTableSize = ReHashCnt = 0;
            SizeOfHashTable = 0;
        }

    protected:
        int FCount{};

        T *GetObject(int N) {
            return &Buckets.GetItemPtrIndex(N-(OneBased ? 1 : 0))->Obj;
        }

        gdlib::gmsdata::TGrowArrayFxd<THashBucket<T>> Buckets;
    public:
        bool OneBased{};

        virtual ~TXStrHashListLegacy() {
            Clear();
            if(PHashTable) std::free(PHashTable);
            SortMap = nullptr;
        }

        void Clear() {
            for(int N{OneBased ? 1 : 0}; N<FCount + (OneBased ? 1 : 0); N++)
                FreeItem(N);
            for(int N{}; N<FCount; N++)
                delete [] Buckets.GetItemPtrIndex(N)->StrP;
            Buckets.Clear();
            FCount = 0;
            ClearHashTable();
            SortMap = nullptr;
            FSorted = false;
        }

        virtual void FreeItem(int N) {
            // noop
        }

        virtual int Hash(const std::string &s) {
            int res{};
            for(char c : s)
                res = 211 * res + toupper(c);
            return (res & 0x7FFFFFFF) % HashTableSize;
        }

        virtual bool EntryEqual(const char *ps1, const char *ps2) {
#if defined(_WIN32)
            return !_stricmp(ps1, ps2);
#else
            return !strcasecmp(ps1, ps2);
#endif
        }

        virtual bool EntryEqual(const std::string &ps1, const std::string &ps2) {
            return utils::sameText(ps1, ps2);
        }

        virtual int Compare(const std::string &ps1, const std::string &ps2) {
            return utils::strCompare(ps1, ps2);
        }

        void HashTableReset(int ACnt) {
            const int   HashSize_1 = 997,
                    HashSize_2 = 9973,
                    HashSize_3 = 99991,
                    HashSize_4 = 999979,
                    HashSize_5 = 9999991,
                    HashSize_6 = 99999989,
                    Next_1 = 1500,
                    Next_2 = 15000,
                    Next_3 = 150000,
                    Next_4 = 1500000,
                    Next_5 = 15000000,
                    Next_6 = std::numeric_limits<int>::max();
            if (ACnt >= Next_5) { HashTableSize = HashSize_6; ReHashCnt = Next_6; }
            else if(ACnt >= Next_4) { HashTableSize = HashSize_5; ReHashCnt = Next_5; }
            else if(ACnt >= Next_3) { HashTableSize = HashSize_4; ReHashCnt = Next_4; }
            else if(ACnt >= Next_2) { HashTableSize = HashSize_3; ReHashCnt = Next_3; }
            else if(ACnt >= Next_1) { HashTableSize = HashSize_2; ReHashCnt = Next_2; }
            else { HashTableSize = HashSize_1; ReHashCnt = Next_1; }
            SizeOfHashTable = HashTableSize * sizeof(THashBucket<T>*);
            PHashTable = (THashBucket<T> **)std::malloc(SizeOfHashTable);
            std::memset(PHashTable, 0, SizeOfHashTable);
        }

        void HashAll() {
            if(PHashTable) std::free(PHashTable);
            HashTableReset(FCount);
            for(int N{}; N<FCount; N++) {
                PHashBucket<T> PBuck = Buckets.GetItemPtrIndex(N);
                int HV{Hash(PBuck->StrP)};
                PBuck->NextBucket = PHashTable[HV];
                PHashTable[HV] = PBuck;
            }
        }

        int IndexOf(const std::string &S) {
            if(!PHashTable) HashAll();
            int HV{Hash(S)};
            PHashBucket<T> PBuck {PHashTable[HV]};
            while(PBuck) {
                if(!EntryEqual(PBuck->StrP, S)) PBuck = PBuck->NextBucket;
                else return PBuck->StrNr + (OneBased ? 1 : 0);
            }
            return -1;
        }

        int AddObject(const std::string &s, T AObj) {
            if(FCount >= ReHashCnt) HashAll();
            int HV{Hash(s)};
            THashBucket<T> *PBuck {PHashTable[HV]};
            while(PBuck) {
                if(!EntryEqual(PBuck->StrP, s)) PBuck = PBuck->NextBucket;
                else return PBuck->StrNr + (OneBased ? 1 : 0);
            }
            PBuck = Buckets.ReserveMem();
            auto obj = PBuck;
            obj->NextBucket = PHashTable[HV];
            PHashTable[HV] = PBuck;
            obj->StrNr = FCount; // before it was added! zero based
            int res{FCount + (OneBased ? 1 : 0)};
            if(SortMap) {
                (*SortMap)[FCount] = FCount;
                FSorted = false;
            }
            FCount++; // ugly
            obj->StrP = gmsobj::NewString(s);
            obj->Obj = AObj;
            return res;
        }

        int StoreObject(const std::string &s, T AObj) {
            if(PHashTable) ClearHashTable();
            THashBucket<T> *PBuck = Buckets.ReserveMem();
            auto obj = PBuck;
            obj->NextBucket = nullptr;
            obj->StrNr = FCount; // before it was added!
            int res{FCount + (OneBased ? 1 : 0)};
            if(SortMap) {
                (*SortMap)[FCount] = FCount;
                FSorted = false;
            }
            FCount++; // ugly
            obj->StrP  = gmsobj::NewString(s);
            obj->Obj = AObj;
            return res;
        }

        std::string GetString(int N) const {
            return Buckets.GetItemPtrIndexConst(N-(OneBased ? 1 : 0))->StrP;
        }

        void RenameEntry(int N, const std::string &s) {
            N -= OneBased ? 1 : 0;
            if(FSorted) {
                SortMap = nullptr;
                FSorted = false;
            }
            if(PHashTable) {
                int HV0 {Hash(Buckets.GetItemPtrIndexConst(N)->StrP)};
                int HV1 {Hash(s)};
                if(HV0 != HV1) {
                    THashBucket<T> *PrevBuck{}, *PBuck {PHashTable[HV0]};
                    while(true) {
                        if(PBuck->StrNr == N) break;
                        PrevBuck = PBuck;
                        PBuck = PBuck->NextBucket;
                    }
                    if(!PrevBuck) PHashTable[HV0] = PBuck->NextBucket;
                    else PrevBuck->NextBucket = PBuck->NextBucket;
                    PBuck->NextBucket = PHashTable[HV1];
                    PHashTable[HV1] = PBuck;
                }
            }
            char **strRef = &Buckets.GetItemPtrIndex(N)->StrP;
            delete [] *strRef;
            *strRef = gmsobj::NewString(s);
        }

        int64_t MemoryUsed() const {
            int64_t res{};
            for(int N{}; N<FCount; N++)
                res += strlen(Buckets.GetItemPtrIndexConst(N)->StrP) + 1;
            res += Buckets.MemoryUsed();
            if(PHashTable) res += SizeOfHashTable;
            if(SortMap) res += SortMap->MemoryUsed();
            return res;
        }

    };

}
