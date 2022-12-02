#pragma once

#include <vector>
#include <list>
#include "../global/modhead.h"
#include "../global/gmsspecs.h"
#include <limits>
#include <algorithm>
#include <optional>

namespace gdlib::datastorage {

    // TODO: Get rid of this (use standard library collection instead)
    // implement radix sort in a general way for sequences/collections
    template<typename KeyType, typename ValType, int valMaxSize>
    class TLinkedData {
    public:
        using KeyArray = std::vector<KeyType>;
        using ValArray = std::array<ValType, valMaxSize>;
        using EntryType = std::pair<KeyArray, ValArray>;
        using TLDStorageType = std::list<EntryType>;
    private:
        TLDStorageType data;
        int FDimension, FDataSize, FKeySize, FTotalSize, FMinKey, FMaxKey;

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

        [[nodiscard]]
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

        ValArray &AddItem(const KeyType *AKey, const ValType *AData) {
            data.push_back({});
            data.back().first.resize(FDimension);
            memcpy(data.back().first.data(), AKey, sizeof(KeyType)*FDimension);
            memcpy(data.back().second.data(), AData, sizeof(ValType)*valMaxSize);
            for(int D{}; D<FDimension; D++) {
                int Key {AKey[D]};
                if(Key > FMaxKey) FMaxKey = Key;
                if(Key < FMinKey) FMinKey = Key;
            }
            return data.back().second;
        }

        void Sort(const int *AMap = nullptr) {
            // FIXME: Should this consider wildcards? =0?
            data.sort([&](const EntryType & p1, const EntryType & p2) {
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

        bool GetNextRecord(typename TLDStorageType::iterator &it, KeyType *AKey, ValType *Data) {
            if(it == data.end()) return false;
            const EntryType & item = *it;
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
                for(const EntryType &pair : data) {
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
            std::vector<ValType *> Head(AllocCount), Tail(AllocCount);
            for(int Key{}; Key < FMaxKey - KeyBase; Key++) {
                Head[Key] = nullptr;
                for(int D{FDimension-1}; D >= 0; D--) {
                    for(const EntryType & pair : data) {
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

