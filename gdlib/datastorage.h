#pragma once

#include <vector>
#include "../global/modhead.h"
#include "../global/gmsspecs.h"
#include <limits>

namespace gdlib::datastorage {

    template<typename  T>
    class TLinkedData {
        std::vector<std::pair<global::gmsspecs::TIndex, T>> data;
        int FMinKey, FMaxKey, FDimension, FKeySize, FTotalSize, FDataSize;
    public:
        TLinkedData(int ADimension, int ADataSize) :
            FDimension{ADimension},
            FDataSize{ADataSize},
            FKeySize{static_cast<int>(ADimension*sizeof(int))},
            FTotalSize{sizeof(T)},
            FMinKey{},
            FMaxKey{std::numeric_limits<int>::max()} {
        }

        ~TLinkedData() = default;

        int size() const { return data.size(); }

        auto begin() {
            return data.begin();
        }

        auto end() {
            return data.end();
        }

        void clear() {
            data.clear();
        }

        /*T &operator[](const global::gmsspecs::TIndex &index) {
            for(int i{}; i<data.size(); i++) {
                bool match{true};
                for (int d{}; d < FDimension; d++) {
                    if (data[i].first[d] != index[d]) {
                        match = false;
                        break;
                    }
                }
                if(match) return data[i].first;
            }
            data.emplace_back(std::make_pair());
        }*/

        void Clear() {
            data.clear();
            FMaxKey = 0;
            FMinKey = std::numeric_limits<int>::max();
        }

        T &AddItem(const global::gmsspecs::TIndex &AKey, const T &AData) {
            data.push_back(std::make_pair(AKey, AData));
            for(int D{}; D<FDimension; D++) {
                int Key {AKey[D]};
                if(Key > FMaxKey) FMaxKey = Key;
                if(Key < FMinKey) FMinKey = Key;
            }
            return data.back().second;
        }

        void Sort(const std::vector<int> &AMap) {

            auto IsSorted = [&]() {
                int KD{};
                std::vector<int> *PrevKey{};
                for(const auto &[keys, obj] : data) {
                    if(PrevKey) {
                        for (int D{}; D < FDimension; D++) {
                            KD = keys[D] - PrevKey[D];
                            if (KD) break;
                        }
                        if (KD < 0) return false;
                    }
                    PrevKey = keys;
                }
            };

            if(data.empty() || IsSorted()) return;

            int KeyBase {FMinKey};
            const int64_t AllocCount = (int64_t)FMaxKey - FMinKey + 1;
            std::vector<T*> Head(AllocCount), Tail(AllocCount);
            for(int Key{}; Key < FMaxKey - KeyBase; Key++) {
                Head[Key] = nullptr;
                for(int D{FDimension-1}; D >= 0; D--) {
                    for(const auto &[keys, obj] : data) {
                        Key = (AMap.empty() ? keys[D] : keys[AMap[D]]) - KeyBase;
                    }
                    /*for(int Key{FMaxKey}; Key >= 0; Key--) {
                        // ...
                    }*/
                }
            }

            // ...
            STUBWARN();
        }
    };

}

