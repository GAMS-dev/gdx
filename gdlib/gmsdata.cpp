#include "gmsdata.h"

#include <algorithm>

namespace gdlib::gmsdata {
    TTblGamsData::TTblGamsData(int ADim, int ADataSize) : FDim{ADim}, FDataSize{ADataSize} {
    }

    void TTblGamsData::GetRecord(int N, int * Inx, double * Vals) {
        const auto InxArr = keyset[N];
        std::memcpy(Inx, InxArr.data(), FDim * sizeof(int));
        std::memcpy(Vals, mapping[InxArr].data(), GMS_VAL_MAX * sizeof(double));
    }

    void TTblGamsData::AddRecord(const int* AElements, const double* AVals) {
        (*this)[utils::asArrayN<int, GLOBAL_MAX_INDEX_DIM>(AElements, FDim)] = utils::asArray<double, 5>(AVals);
    }

    ValueFields &TTblGamsData::operator[](const IndexKeys &Key) {
        if (std::find(keyset.begin(), keyset.end(), Key) == keyset.end())
            keyset.push_back(Key);
        return mapping[Key];
    }

    void TTblGamsData::clear() {
        keyset.clear();
        mapping.clear();
    }

    int TTblGamsData::size() const {
        return (int)keyset.size();
    }

    std::map<IndexKeys, ValueFields>::iterator TTblGamsData::begin() {
        return mapping.begin();
    }

    std::map<IndexKeys, ValueFields>::iterator TTblGamsData::end() {
        return mapping.end();
    }

    bool TTblGamsData::empty() const {
        return keyset.empty();
    }

    void TTblGamsData::sort() {
        std::sort(keyset.begin(), keyset.end());
    }

    int TTblGamsData::GetDimension() const { return FDim; }

}