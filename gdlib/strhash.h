#pragma once

#include "gmsstrm.h"
#include "../utils.h"
#include "strutilx.h"

#include <vector>
#include <string>
#include <cassert>
#include <numeric>

namespace gdlib::strhash {
    template<typename T>
    struct THashBucket {
        std::string StrP {};
        size_t NxtBuckIndex {};
        int StrNr {};
        T Obj {};
    };

    template<typename T>
    using PHashBucket = THashBucket<T>*;

    template<typename T>
    class TXStrHashList {
    protected:
        std::vector<THashBucket<T>> Buckets {};
        std::unique_ptr<std::vector<size_t>> PHashTable {};
        std::unique_ptr<std::vector<int>> SortMap {};
        int64_t SizeOfHashTable {};
        int HashTableSize {}, ReHashCnt {}, FCount {};
        bool FSorted {};

        void ClearHashTable() {
            PHashTable = nullptr;
            SizeOfHashTable = 0;
            HashTableSize = ReHashCnt = 0;
        }

        void HashTableReset(int ACnt) {
            const int   HashSize_1 = 97,
                        HashSize_2 = 9973,
                        HashSize_3 = 99991,
                        HashSize_4 = 999979,
                        HashSize_5 = 9999991,
                        HashSize_6 = 99999989,
                        Next_1 = 150,
                        Next_2 = 10000,
                        Next_3 = 100000,
                        Next_4 = 1500000,
                        Next_5 = 15000000,
                        Next_6 = std::numeric_limits<int>::max(); 
            if (ACnt >= Next_5) { HashTableSize = HashSize_6; ReHashCnt = Next_6; }
            else if(ACnt >= Next_4) { HashTableSize = HashSize_5; ReHashCnt = Next_5; }
            else if(ACnt >= Next_3) { HashTableSize = HashSize_4; ReHashCnt = Next_4; }
            else if(ACnt >= Next_2) { HashTableSize = HashSize_3; ReHashCnt = Next_3; }
            else if(ACnt >= Next_1) { HashTableSize = HashSize_2; ReHashCnt = Next_2; }
            else { HashTableSize = HashSize_1; ReHashCnt = Next_1; }
            SizeOfHashTable = HashTableSize * sizeof(void *);
            PHashTable = std::make_unique<std::vector<size_t>>(SizeOfHashTable);
            std::fill_n(PHashTable->begin(), HashTableSize, 0);
        }

        virtual int Hash(const std::string &s) {
            int res{};
            for(char c : s)
                res = 211 * res + toupper(c);
            return (res & 0x7FFFFFFF) % HashTableSize;
        }

        virtual bool EntryEqual(const std::string &ps1, const std::string &ps2) {
            return gdlib::strutilx::PStrUEqual(ps1, ps2);
        }

        virtual int Compare(const std::string &ps1, const std::string &ps2) {
            return gdlib::strutilx::PStrUCmp(ps1, ps2);
        }

        void HashAll() {
            if(PHashTable) PHashTable->clear();
            HashTableReset(FCount);
            for(int N{}; N<FCount; N++) {
                auto &PBuck = Buckets[N];
                int HV = Hash(PBuck.StrP);
                PBuck.NxtBuckIndex = GetBucketIndexByHash(HV);
                (*PHashTable)[HV] = N+1;
            }
        }

        T *GetObject(int N) {
            return &Buckets[N-(OneBased ? 1 : 0)].Obj;
        }

        void SetObject(int N, T AObj) {
            Buckets[N-(OneBased ? 1 : 0)].Obj = AObj;
        }

        void SetSortedObject(int N, T &AObj) {
            if(!FSorted) Sort();
            Buckets[(*SortMap)[N-(OneBased ? 1 : 0)]].Obj = &AObj;
        }

        std::string &GetString(int N) {
            return Buckets[N-(OneBased ? 1 : 0)].StrP;
        }

        std::string &GetSortedString(int N) {
            if(FSorted) Sort();
            return Buckets[(*SortMap)[N-(OneBased ? 1 : 0)]].StrP;
        }

        void QuickSort(int L, int R) {
            int i{L};
            while(i < R) {
                int j {R}, p {(L+R) >> 1};
                std::string pPivotStr = Buckets[(*SortMap)[p]].StrP;
                do {
                    while(Compare(Buckets[(*SortMap)[i]].StrP, pPivotStr) < 0) i++;
                    while(Compare(Buckets[(*SortMap)[j]].StrP, pPivotStr) > 0) j--;
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
                    std::string PSN = Buckets[0].StrP;
                    for(int N{}; N<FCount-1; N++) {
                        std::string PSN1 = Buckets[N+1].StrP;
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
            Buckets.clear();
            FCount = 0;
            ClearHashTable();
            SortMap = nullptr;
            FSorted = false;
        }

        /*int StoreObject(const std::string& s, T& AObj) {
            if(PHashTable) ClearHashTable();
            Buckets.emplace_back();
            auto &PBuck = Buckets.back();
            PBuck.StrNr = FCount; // before it was added!
            if(SortMap) {
                (*SortMap)[FCount] = FCount;
                FSorted = false;
            }
            FCount++; // ugly
            PBuck.StrP = s;
            PBuck.Obj = &AObj;
            return FCount - 1 + (OneBased ? 1 : 0);
        }*/

        inline size_t GetBucketIndexByHash(int hash) {
            return (*PHashTable)[hash];
        }

        inline PHashBucket<T> GetBucketByHash(int hash) {
            return GetBucket(GetBucketIndexByHash(hash));
        }

        inline PHashBucket<T> GetBucket(size_t index) {
            return !index ? nullptr : &Buckets[index-1];
        }

        int StoreObject(const std::string& s, T AObj) {
            if (PHashTable) ClearHashTable();
            Buckets.push_back({});
            PHashBucket<T> PBuck = &Buckets.back();
            PBuck->NxtBuckIndex = 0;
            PBuck->StrNr = FCount; // before it was added!
            int res{ FCount + (OneBased ? 1 : 0) };
            if (SortMap) {
                (*SortMap)[FCount] = FCount;
                FSorted = false;
            }
            FCount++; // ugly
            PBuck->StrP = s;
            PBuck->Obj = std::move(AObj);
            return res;
        }

        int AddObject(const std::string &s, T AObj) {
            assert(FCount < std::numeric_limits<int>::max());
            if(FCount >= ReHashCnt) HashAll();
            int HV {Hash(s)};
            PHashBucket<T> PBuck = GetBucketByHash(HV);
            while(PBuck) {
                if(!EntryEqual(PBuck->StrP, s)) PBuck = GetBucket(PBuck->NxtBuckIndex);
                else return PBuck->StrNr + (OneBased ? 1 : 0);
            }
            Buckets.push_back({});
            PBuck = &Buckets.back();
            PBuck->NxtBuckIndex = GetBucketIndexByHash(HV);
            (*PHashTable)[HV] = Buckets.size();
            PBuck->StrNr = FCount; // before it was added! zero based
            int res {FCount + (OneBased ? 1 : 0)};
            if(SortMap) {
                (*SortMap)[FCount] = FCount;
                FSorted = false;
            }
            FCount++; // ugly
            PBuck->StrP = s;
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
                if(!EntryEqual(PBuck->StrP, s)) PBuck = GetBucket(PBuck->NxtBuckIndex);
                else return PBuck->StrNr + (OneBased ? 1 : 0);
            }
            return -1;
        }

        void LoadFromStream(gdlib::gmsstrm::TXStream &s) {
            Clear();
            int Cnt{s.ReadInteger()};
            for(int N{}; N<Cnt; N++) {
                std::string s2 = s.ReadString();
                StoreObject(s2, nullptr);
            }
        }

        void SaveToStream(gdlib::gmsstrm::TXStream &s) {
            s.WriteInteger(FCount);
            for(int N{}; N<FCount; N++)
                s.WriteString(GetString(N));
        }

        int GetStringLength(int N) {
            return GetString(N).length();
        }

        int64_t MemoryUsed() {
            return 0;
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
                    size_t PBuckIndex{};
                    for(PBuck = GetBucketByHash(HV0), PBuckIndex = GetBucketIndexByHash(HV0);
                        PBuck->StrNr != N;
                        PBuckIndex = PBuck->NxtBuckIndex, PBuck = GetBucket(PBuck->NxtBuckIndex))
                        PrevBuck = PBuck;
                    if(!PrevBuck) (*PHashTable)[HV0] = PBuck->NxtBuckIndex;
                    else PrevBuck->NxtBuckIndex = PBuck->NxtBuckIndex;
                    PBuck->NxtBuckIndex = GetBucketIndexByHash(HV1);
                    (*PHashTable)[HV1] = PBuckIndex;
                }
            }
            GetString(N+1) = s;
        }

        T* operator[](int N) {
            return GetObject(N);
        }

        T* GetSortedObject(int N) {
            if(!FSorted) Sort();
            return Buckets[(*SortMap)[N-(OneBased ? 1 : 0)]].Obj;
        }

        std::string GetString(int N) const {
            return Buckets[N-(OneBased ? 1 : 0)].StrP;
        }

        std::string GetSortedString(int N) const {
            if(!FSorted) Sort();
            return Buckets[(*SortMap)[N-(OneBased ? 1 : 0)]].StrP;
        }

        int Count() const {
            return FCount;
        }
    };

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
            return gdlib::strutilx::PStrEqual(ps1, ps2);
        }
    };

}
