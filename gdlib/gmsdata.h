#pragma once

#include <map>
#include <string>
#include <array>
#include <vector>

#include "gmsobj.h"

namespace gdlib::gmsdata {

	const int BufSize = 1024 * 16;

	struct TGADataBuffer {
		int BytesUsed{}, filler{};
		std::array<uint8_t, BufSize> Buffer{};
	};
	using PGADataBuffer = TGADataBuffer;
	using TGADataArray = std::array<PGADataBuffer, 1024 * 1024 * 257 - 2>;
	using PGADataArray = TGADataArray*;

	using TXIntList = std::vector<int>;

    using IndexKeys = std::array<int, 20>;
    using ValueFields = std::array<double, 5>;

    // TODO: The port of this class uses C++ standard library collections instead of Paul's custom GAMS colections
    // evalute performance impact of this choice!
	class TTblGamsData {
        std::map<IndexKeys, ValueFields> mapping;
        std::vector<IndexKeys> keyset;
    public:
		void GetRecord(int N, int * Inx, int InxCnt, double * Vals);
        ValueFields &operator[](const IndexKeys &Key);
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

    class TGrowArrayFxd {
        PGADataArray PBase;
        PGADataBuffer PCurrentBuf;
        int BaseAllocator, BaseUsed, FSize, FStoreFact;
        int64_t FCount;

    public:
        explicit TGrowArrayFxd(int ASize) {}
        ~TGrowArrayFxd() = default;
        void Clear() {}
        void *ReserveMem() { return nullptr; }
        void *ReserveAndClear() { return nullptr; }
        void *AddItem(const void *R) { return nullptr; }
        uint8_t *GetItemPtrIndex(int N) { return nullptr; }
        void GetItem(int N, void **R) {}
        int64_t MemoryUsed() const { return 0; }
        int64_t GetCount() const { return FCount; }
    };

    class TTblGamsDataLegacy {
        TGrowArrayFxd DS;
        gdlib::gmsobj::TXList<uint8_t> FList;
        int FDim, FIndexSize, FDataSize;
        bool FIsSorted;
        int FLastIndex;

    public:
    };

}