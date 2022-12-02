#pragma once

#include <cstring>
#include <vector>
#include <list>
#include <limits>
#include <algorithm>
#include <optional>

#include "../global/modhead.h"
#include "../global/gmsspecs.h"

namespace gdlib::datastorage {

    template<typename KeyType, int maxKeySize, typename ValueType, int maxValueSize>
    struct TLinkedDataRec {
        TLinkedDataRec *RecNext, *HashNext;
        std::array<ValueType, maxValueSize> RecData;
        std::array<KeyType, maxKeySize> RecKeys;
    };

    template<typename KeyType, int maxKeySize, typename ValueType, int maxValueSize>
    class TLinkedDataLegacy {
        int FMinKey, FMaxKey, FDimension, FKeySize, FTotalSize, FDataSize, FCount;
        using RecType = TLinkedDataRec<KeyType, maxKeySize, ValueType, maxValueSize>;
        RecType *FHead, *FTail;

    public:
        using KeyArray = std::array<KeyType, maxKeySize>;
        using ValArray = std::array<ValueType, maxValueSize>;

        TLinkedDataLegacy(int ADimension, int ADataSize) :
            FDimension(ADimension),
            FKeySize(ADimension * (int)sizeof(KeyType)),
            FDataSize(ADataSize),
            FTotalSize(sizeof(RecType)),
            FHead{},
            FTail{},
            FCount{},
            FMaxKey{},
            FMinKey{std::numeric_limits<int>::max()}
        {
        };

        void Clear() {
            RecType *P {FHead}, *Pn{};
            while(P) {
                Pn = P->RecNext;
                delete P;
                P = Pn;
            }
            FCount = 0;
            FHead = FTail = nullptr;
            FMaxKey = 0;
            FMinKey = std::numeric_limits<int>::max();
        }

        int MemoryUsed() {
            return FCount * FTotalSize;
        }

        ValArray &AddItem(const KeyType *AKey, const ValueType *AData) {
            auto node = new RecType {};
            if(!FHead) FHead = node;
            else FTail->RecNext = node;
            FTail = node;
            node->RecNext = nullptr;
            std::memcpy(node->RecKeys.data(), AKey, FKeySize);
            std::memcpy(node->RecData.data(), AData, FDataSize);
            FCount++;
            for(int D{}; D<FDimension; D++) {
                int Key{AKey[D]};
                if(Key > FMaxKey) FMaxKey = Key;
                if(Key < FMinKey) FMinKey = Key;
            }
            return *node;
        }

        // ...
    };

    // TODO: Get rid of this (use standard library collection instead)
    // implement radix sort in a general way for sequences/collections
    template<typename KeyType, int keyMaxSize, typename ValType, int valMaxSize>
    class TLinkedData {
    public:
        using KeyArray = std::array<KeyType, keyMaxSize>;
        using ValArray = std::array<ValType, valMaxSize>;
        using EntryType = std::pair<KeyArray, ValArray>;
        using TLDStorageType = std::list<EntryType>;
        using TLDIterator = typename std::list<EntryType>::iterator;

    private:
        TLDStorageType data;
        int FDimension, FKeySize, FMinKey, FMaxKey;

        bool IsSorted() {
            const KeyType * PrevKey{};
            for(const EntryType &pair : data) {
                const KeyArray &keys = pair.first;
                if(PrevKey) {
                    int KD{};
                    for (int D{}; D < FDimension; D++) {
                        KD = keys[D] - PrevKey[D];
                        if (KD) break;
                    }
                    if (KD < 0) return false;
                }
                PrevKey = keys.data();
            }
            return true;
        };

    public:
        TLinkedData(int ADimension, int ADataSize) :
            FDimension{ADimension},
            FKeySize{static_cast<int>(ADimension*sizeof(int))},
            FMinKey{std::numeric_limits<int>::max()},
            FMaxKey{} {
        }

        ~TLinkedData() = default;

        [[nodiscard]]
        int size() const {
            return (int)data.size();
        }

        void Clear() {
            data.clear();
            FMaxKey = 0;
            FMinKey = std::numeric_limits<int>::max();
        }

        ValArray &AddItem(const KeyType *AKey, const ValType *AData) {
            data.push_back({});
            //data.back().first.resize(FDimension);
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
            if(data.empty() || IsSorted()) return;
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

    private:
        // FIXME: Unfinished!
        void RadixSort(const int *AMap = nullptr) {
            if(data.empty() || IsSorted()) return;
            int KeyBase {FMinKey};
            const int64_t AllocCount = (int64_t)FMaxKey - FMinKey + 1;
            std::vector<ValType *> Head(AllocCount), Tail(AllocCount);
            TLDIterator myhead = data.begin();
            for(int Key{}; Key < FMaxKey - KeyBase; Key++) {
                Head[Key] = nullptr;
                for(int D{FDimension-1}; D >= 0; D--) {
                    for(EntryType & pair : data) {
                        const KeyArray & keys = pair.first;
                        Key = (!AMap ? keys[D] : keys[AMap[D]]) - KeyBase;
                        if(!Head[Key]) Head[Key] = pair.second.data();

                    }
                }
            }
        }
    };

}

