#include "gmsdata.h"

#include <algorithm>

namespace gdlib::gmsdata {

    void TTblGamsData::GetRecord(int N, int * Inx,  int InxCnt, double * Vals) {
        const auto InxArr = keyset[N];
        memcpy(Inx, InxArr.data(), InxCnt * sizeof(int));
        memcpy(Vals, mapping[InxArr].data(), GMS_VAL_MAX * sizeof(double));
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

}