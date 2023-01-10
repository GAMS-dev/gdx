#pragma once

#include <cstring>
#include <vector>
#include <list>
#include <limits>
#include <algorithm>
#include <optional>
#include <array>

// TLinkedData dynamic array toggle
// iff. defined? Use std::vector with actual dimension and value counts,
// otherwise std::array with fixed maximum sizes, see below.
// Maximum sizes are usually:
// - keys:   GLOBAL_MAX_INDEX_DIM = 20
// - values: GMS_VAL_MAX          = 5
//#define TLD_DYN_ARRAYS

#ifdef TLD_DYN_ARRAYS
#define TLD_TEMPLATE_HEADER template<typename KeyType, typename ValueType>
#define TLD_REC_TYPE TLinkedDataRec<KeyType, ValueType>
#else
#define TLD_TEMPLATE_HEADER template<typename KeyType, int maxKeySize, typename ValueType, int maxValueSize>
#define TLD_REC_TYPE TLinkedDataRec<KeyType, maxKeySize, ValueType, maxValueSize>
#endif

namespace gdlib::datastorage {

    TLD_TEMPLATE_HEADER
    struct TLinkedDataRec {
        TLinkedDataRec *RecNext{};
#ifndef TLD_DYN_ARRAYS
        std::array<ValueType, maxValueSize> RecData;
        std::array<KeyType, maxKeySize> RecKeys;
        TLinkedDataRec(int numKeys, int numValues) {}
#else
        std::vector<ValueType> RecData;
        std::vector<KeyType> RecKeys;
        TLinkedDataRec(int numKeys, int numValues) : RecData(numValues), RecKeys(numKeys) {}
#endif
    };

    TLD_TEMPLATE_HEADER
    class TLinkedDataLegacy {
        int FMinKey,
            FMaxKey,
            FDimension, // number of keys / symbol dimension
            FKeySize,   // byte count for key storage
            FDataSize,  // byte count for value storage
            FTotalSize, // byte count for entry
            FCount;
        using RecType = TLD_REC_TYPE;
        RecType *FHead, *FTail;

        bool IsSorted() {
            RecType *R{FHead};
            auto *PrevKey {R->RecKeys.data()};
            R = R->RecNext;
            int KD{};
            while(R) {
                for(int D{}; D<FDimension; D++) {
                    KD = R->RecKeys[D] - PrevKey[D];
                    if(KD) break;
                }
                if(KD < 0) return false;
                PrevKey = R->RecKeys.data();
                R = R->RecNext;
            }
            return true;
        }

    public:
        TLinkedDataLegacy(int ADimension, int ADataSize) :
            FMinKey{std::numeric_limits<int>::max()},
            FMaxKey{},
            FDimension{ADimension},
            FKeySize{ADimension * (int)sizeof(KeyType)},
            FDataSize{ADataSize},
            FTotalSize{2 * (int)sizeof(void *) + FKeySize + FDataSize},
            FCount{},
            FHead{},
            FTail{}
        {
        };

        ~TLinkedDataLegacy() {
            Clear();
        }

        int Count() const {
            return FCount;
        }

        void Clear() {
            RecType *P {FHead};
            while(P) {
                auto Pn = P->RecNext;
                delete P;
                P = Pn;
            }
            FCount = FMaxKey = 0;
            FHead = FTail = nullptr;
            FMinKey = std::numeric_limits<int>::max();
        }

        int MemoryUsed() {
            return FCount * FTotalSize;
        }

        ValueType *AddItem(const KeyType *AKey, const ValueType *AData) {
            auto node = new RecType { FDimension, FDataSize / (int)sizeof(ValueType) };
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
            return node->RecData.data();
        }

        void Sort(const int *AMap = nullptr) {
            if(!FHead || IsSorted()) return;
            const int AllocCount = FMaxKey - FMinKey + 1;
            std::vector<RecType *> Head(AllocCount), Tail(AllocCount);
            int KeyBase {FMinKey};
            // Perform radix sort
            std::fill_n(Head.begin(), FMaxKey-KeyBase, nullptr);
            for(int D{FDimension-1}; D>=0; D--) {
                RecType *R = FHead;
                while(R) {
                    int Key {R->RecKeys[AMap ? AMap[D] : D] - KeyBase};
                    if(!Head[Key]) Head[Key] = R;
                    else Tail[Key]->RecNext = R;
                    Tail[Key] = R;
                    R = R->RecNext;
                }
                R = nullptr;
                for(int Key{FMaxKey-KeyBase}; Key >= 0; Key--) {
                    if(Head[Key]) {
                        Tail[Key]->RecNext = R;
                        R = Head[Key];
                        Head[Key] = nullptr; // so we keep it all nullptr
                    }
                }
                FHead = R;
            }
            FTail = nullptr; // what is the tail???
        }

        std::optional<RecType*> StartRead(const int *AMap = nullptr) {
            if(FCount <= 0) return std::nullopt;
            Sort(AMap);
            return {FHead};
        }

        bool GetNextRecord(RecType **P, KeyType *AKey, ValueType *AData) {
            if(P && *P) {
                const RecType &it = **P;
                std::memcpy(AKey, it.RecKeys.data(), FKeySize);
                std::memcpy(AData, it.RecData.data(), FDataSize);
                *P = it.RecNext;
                return true;
            }
            return false;
        }
    };

#ifdef TLD_DYN_ARRAYS
    template<typename KeyType, typename ValType>
#else
    template<typename KeyType, int keyMaxSize, typename ValType, int valMaxSize>
#endif
    class TLinkedData {
    public:
#ifndef TLD_DYN_ARRAYS
        using KeyArray = std::array<KeyType, keyMaxSize>;
        using ValArray = std::array<ValType, valMaxSize>;
#else
        using KeyArray = std::vector<KeyType>;
        using ValArray = std::vector<ValType>;
#endif
        using EntryType = std::pair<KeyArray, ValArray>;
        using TLDStorageType = std::list<EntryType>;

    private:
        TLDStorageType data;

        int FDimension, // number of keys / symbol dimension
            FKeySize,   // byte count for key storage
            FDataSize;  // byte count for value storage

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
            FDataSize{ADataSize}{
        }

        ~TLinkedData() = default;

        int Count() const {
            return static_cast<int>(data.size());
        }

        void Clear() {
            data.clear();
        }

        ValArray &AddItem(const KeyType *AKey, const ValType *AData) {
#ifdef TLD_DYN_ARRAYS
            KeyArray keys(FDimension);
            ValArray vals(FDataSize / (int)sizeof(double));
            data.emplace_back(std::make_pair(keys, vals));
#else
            data.push_back({});
#endif
            memcpy(data.back().first.data(), AKey, FKeySize);
            memcpy(data.back().second.data(), AData, FDataSize);
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
            memcpy(Data, item.second.data(), FDataSize);
            it++;
            return true;
        }
    };

}

