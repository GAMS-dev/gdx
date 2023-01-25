#pragma once

#include <map>
#include <string>
#include <array>
#include <vector>

#include "../expertapi/gclgms.h"

#include "gmsobj.h"

namespace gdlib::gmsdata {

	const int BufSize = 1024 * 16;

	struct TGADataBuffer {
		int BytesUsed{},
            filler{}; //so a buffer can start on 8 byte boundary
		std::array<uint8_t, BufSize> Buffer{};
	};

	using PGADataBuffer = TGADataBuffer *;
	using PGADataArray = TGADataBuffer *; // dynamic heap array of pointers

	using TXIntList = std::vector<int>;

    using IndexKeys = std::array<int, GLOBAL_MAX_INDEX_DIM>;
    using ValueFields = std::array<double, GMS_VAL_MAX>;

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
        PGADataArray PBase {};
        PGADataBuffer PCurrentBuf {};
        int BaseAllocated {}, BaseUsed {-1}, FSize, FStoreFact;
        int64_t FCount {};

    public:
        explicit TGrowArrayFxd(int ASize);
        ~TGrowArrayFxd();
        void Clear();
        void *ReserveMem();
        void *ReserveAndClear();
        void *AddItem(const void *R);
        uint8_t *GetItemPtrIndex(int N);
        void GetItem(int N, void **R);
        int64_t MemoryUsed() const;
        int64_t GetCount() const;
    };

    // FIXME: Work in progress!
    class TTblGamsDataLegacy {
        TGrowArrayFxd DS;
        gdlib::gmsobj::TXList<uint8_t> FList;
        int FDim, FIndexSize, FDataSize;
        bool FIsSorted;
        int FLastIndex;

    public:
    };

}