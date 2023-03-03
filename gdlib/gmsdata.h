#pragma once

#include "../expertapi/gclgms.h"

#include <map>
#include <string>
#include <array>
#include <vector>

#include "gmsobj.h"

namespace gdlib::gmsdata {
    template<typename T>
    class TTblGamsData {
        int FDim, FDataSize;
        std::vector<std::pair<int *, T *>> keyValues {};
    public:
        TTblGamsData(int ADim, int ADataSize) : FDim{ADim}, FDataSize{ADataSize} {}
        
        virtual ~TTblGamsData() {
            Clear();
        }

        void GetRecord(int N, int* Inx, T* Vals) {
            std::memcpy(Inx, keyValues[N].first, FDim * sizeof(int));
            std::memcpy(Vals, keyValues[N].second, FDataSize);
        }

        void GetKeys(int N, int *Inx) {
            std::memcpy(Inx, keyValues[N].first, FDim * sizeof(int));
        }

        void GetData(int N, T *Vals) {
            std::memcpy(Vals, keyValues[N].second, FDataSize);
        }
        double *GetDataPtr(int N) {
            return keyValues[N].second;
        }

        void AddRecord(const int* AElements, const T* AVals) {
            // TODO: Maybe use batch allocator vs. single new allocs here for performance?
            auto vals {new T[(size_t)FDataSize/sizeof(T)]};
            std::memcpy(vals, AVals, FDataSize);
            auto keys{new int[FDim]};
            std::memcpy(keys, AElements, FDim*sizeof(int));
            keyValues.emplace_back(keys, vals);
        }

        void Clear() {
            for(auto [k,v] : keyValues) {
                delete [] k;
                delete [] v;
            }
            keyValues.clear();
        }

        int size() const {
            return (int)keyValues.size();
        }

        int GetCount() const {
            return size();
        }

        bool empty() const {
            return keyValues.empty();
        }

        void Sort() {
            std::sort(keyValues.begin(), keyValues.end(), [this](const auto &pair1, const auto &pair2) {
                for(int i=0; i<FDim; i++)
                    if(pair1.first[i] > pair2.first[i]) return false;
                return true;
            });
        }

        int MemoryUsed() const {
            return static_cast<int>(keyValues.size() * (FDim*sizeof(int) + FDataSize*sizeof(T)) + keyValues.capacity());
        }

        int GetDimension() const {
            return FDim;
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

        explicit TGrowArrayFxd(int ASize) : FSize{ASize}, FStoreFact{BufSize/FSize}
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
                    if (!PBase) PBase = static_cast<TGADataBuffer**>(std::malloc(newByteCount));
                    else PBase = static_cast<TGADataBuffer**>(std::realloc(PBase, newByteCount));
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

    template<typename T>
    class TTblGamsDataLegacy {
        TGrowArrayFxd<uint8_t> DS;
        gdlib::gmsobj::TXList<uint8_t> FList {};
        int FDim, FIndexSize, FDataSize;
        bool FIsSorted{true};
        int FLastIndex{-1};

        void QuickSort(int L, int R) {
            int i{L};
            while(i < R) {
                int j{R};
                int p{(L+R) >> 1};
                auto pivot {reinterpret_cast<int*>(FList[p])};
                do {
                    while(CompareWithRecPtr(i, pivot) < 0) i++;
                    while(CompareWithRecPtr(j, pivot) > 0) j--;
                    if(i < j)
                        Exchange(i++, j--);
                    else if(i == j) {
                        i++;
                        j--;
                    }
                } while(i <= j);
                if((j-L) > (R-i)) {
                    if(i < R) QuickSort(i, R);
                    i = L;
                    R = j;
                } else {
                    if(L < j) QuickSort(L, j);
                    L = i;
                }
            }
        }

        int Compare(int Index1, int Index2) {
            const int   *P1 {reinterpret_cast<const int *>(FList[Index1])},
                        *P2 {reinterpret_cast<const int *>(FList[Index2])};
            for(int D{}; D<FDim; D++) {
                int diff {P1[D] - P2[D]};
                if(diff) return diff;
            }
            return 0;
        }

        int CompareWithRecord(const int *Inx, int N) {
            auto P1 {reinterpret_cast<const int *>(FList[N])};
            for(int D{}; D<FDim; D++) {
                int diff {Inx[D]-P1[D]};
                if(diff) return diff;
            }
            return 0;
        }

        int CompareWithRecPtr(int i1, const int *p2) {
            auto P1 {reinterpret_cast<const int *>(FList[i1])};
            for(int k{}; k<FDim; k++) {
                int diff {P1[k] - p2[k]};
                if(diff) return diff;
            }
            return 0;
        }

        void Exchange(int Index1, int Index2) {
            auto P {*FList[Index1]};
            *FList[Index1] = *FList[Index2];
            *FList[Index2] = P;
        }

        void InsertRecord(int N, const int *Inx, const T *Buffer) {
            auto P {DS.ReserveMem()};
            std::memcpy(P, Inx, FIndexSize);
            std::memcpy(&P[FIndexSize], Buffer, FDataSize);
            FList.Insert(N, P);
            FIsSorted = false;
        }

    public:
        TTblGamsDataLegacy(int ADim, int ADataSize) :
            DS{static_cast<int>(ADim*sizeof(int) + ADataSize)},
            FDim{ADim},
            FIndexSize{static_cast<int>(FDim * sizeof(int))},
            FDataSize{ADataSize}
        {
        }

        virtual ~TTblGamsDataLegacy() = default;

        inline void AddRecord(const int *Inx, const T *Buffer) {
            InsertRecord(FList.size(), Inx, Buffer);
        }

        bool AddUniqueRecord(const int *Inx, T *Buffer) {
            int N;
            bool res{!SearchRecord(Inx, N)};
            if(res) InsertRecord(N, Inx, Buffer);
            return res;
        }

        void GetRecord(int N, int *Inx, T *Buffer) {
            auto P {FList[N]};
            std::memcpy(Inx, P, FIndexSize);
            std::memcpy(Buffer, &P[FIndexSize], FDataSize);
        }

        void Sort() {
            if(!FIsSorted) {
                bool SortNeeded {};
                for(int N{}; N<FList.size()-1; N++) {
                    if(Compare(N, N+1) > 0) {
                        SortNeeded = true;
                        break;
                    }
                }
                if(SortNeeded) QuickSort(0, FList.size()-1);
                FIsSorted = true;
            }
        }

        void GetKeys(int N, int *Inx) {
            std::memcpy(Inx, FList[N], FIndexSize);
        }

        void GetData(int N, T *Buffer) {
            std::memcpy(Buffer, &FList[N][FIndexSize], FDataSize);
        }

        T *GetDataPtr(int N) {
            return reinterpret_cast<T*>(&FList[N][FIndexSize]);
        }

        bool SearchRecord(const int *Inx, int &RecNr) {
            int H{FList.size()-1};
            if(H < 0) {
                RecNr = 0;
                FLastIndex = 0;
                return false;
            }
            int L{};
            FLastIndex++;
            if(FLastIndex >= 0 && FLastIndex <= H) {
                int C {CompareWithRecord(Inx, FLastIndex)};
                if(!C) {
                    RecNr = FLastIndex;
                    return true;
                }
                if(C < 0) H = FLastIndex - 1;
                else L = FLastIndex + 1;
            }
            // binary search
            bool res{};
            while(L <= H) {
                int I {(L+H) >> 1};
                int C{CompareWithRecord(Inx, I)};
                if(C > 0) L = I + 1;
                else if(C)
                    H = I - 1;
                else {
                    res = true;
                    L = I;
                    break;
                }
            }
            RecNr = FLastIndex = L;
            return res;
        }

        void Clear() {
            DS.Clear();
            FList.Clear();
        }

        int64_t MemoryUsed() const {
            return DS.MemoryUsed() + FList.MemoryUsed();
        }

        int GetCount() const {
            return FList.size();
        }

        int GetCapacity() const {
            return FList.GetCapacity();
        }

        void SetCapacity(int N) {
            FList.SetCapacity(N);
        }

        int GetDimension() const {
            return FDim;
        }
    };
}
