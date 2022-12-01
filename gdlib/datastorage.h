#pragma once

#include <vector>
#include "../global/modhead.h"
#include "../global/gmsspecs.h"
#include <limits>
#include <algorithm>
#include <optional>

namespace gdlib::datastorage {

    // TODO: Get rid of this (use standard library collection instead)
    // implement radix sort in a general way for sequences/collections
    template<typename T>
    class TLinkedData {
    public:
        using TLDStorageType = std::list<std::pair<global::gmsspecs::TIndex, T>>;
    private:
        TLDStorageType data;
        int FMinKey, FMaxKey, FDimension, FKeySize, FTotalSize, FDataSize;

    public:
        TLinkedData(int ADimension, int ADataSize) :
            FDimension{ADimension},
            FDataSize{ADataSize},
            FKeySize{static_cast<int>(ADimension*sizeof(int))},
            FTotalSize{2*(int)sizeof(void*)+FKeySize+FDataSize},
            FMinKey{std::numeric_limits<int>::max()},
            FMaxKey{} {
        }

        ~TLinkedData() = default;

        int size() const { return (int)data.size(); }

        auto begin() {
            return data.begin();
        }

        auto end() {
            return data.end();
        }

        void clear() {
            data.clear();
        }

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

        void Sort(const int *AMap = nullptr) {
            // FIXME: Should this consider wildcards? =0?
            data.sort([&](const std::pair<global::gmsspecs::TIndex, T>& p1, const std::pair<global::gmsspecs::TIndex, T>& p2) {
                for (int D{}; D < FDimension; D++) {
                    if (p1.first[D] < p2.first[D]) return true;
                    else if (p1.first[D] > p2.first[D]) return false;
                }
                return false;
            });
        }

        typename TLDStorageType::iterator StartRead(const int *AMap = nullptr) {
            if (!data.empty()) {
                Sort(AMap);
                return data.begin();
            }
            return data.end();
        }

        bool GetNextRecord(typename TLDStorageType::iterator &it, int *AKey, double *Data) {
            if(it == data.end()) return false;
            const std::pair<global::gmsspecs::TIndex, T>& item = *it;
            memcpy(AKey, item.first.data(), FKeySize);
            memcpy(Data, item.second.data(), sizeof(double)*item.second.size());
            it++;
            return true;
        }

        // TODO: AS: Fully port and test this!
        void SortDelphiStyle(const std::optional<std::vector<int>> &AMap = std::nullopt) {
            auto IsSorted = [&]() {
                int KD{};
                const global::gmsspecs::TIndex * PrevKey{};
                for(const std::pair<global::gmsspecs::TIndex, T> &pair : data) {
                    const global::gmsspecs::TIndex &keys = pair.first;
                    if(PrevKey) {
                        for (int D{}; D < FDimension; D++) {
                            KD = keys[D] - (*PrevKey)[D];
                            if (KD) break;
                        }
                        if (KD < 0) return false;
                    }
                    PrevKey = &keys;
                }
                return true;
            };

            if(data.empty() || IsSorted()) return;

            int KeyBase {FMinKey};
            const int64_t AllocCount = (int64_t)FMaxKey - FMinKey + 1;
            std::vector<T*> Head(AllocCount), Tail(AllocCount);
            for(int Key{}; Key < FMaxKey - KeyBase; Key++) {
                Head[Key] = nullptr;
                for(int D{FDimension-1}; D >= 0; D--) {
                    for(const std::pair<global::gmsspecs::TIndex, T>& pair : data) {
                        const global::gmsspecs::TIndex& keys = pair.first;
                        Key = (!AMap ? keys[D] : keys[(*AMap)[D]]) - KeyBase;
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

