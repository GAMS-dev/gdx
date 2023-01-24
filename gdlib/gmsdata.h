#pragma once

#include <map>
#include <string>
#include <array>
#include <vector>

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

}