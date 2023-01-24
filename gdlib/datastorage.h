#pragma once

#include <cstring>
#include <vector>
#include <list>
#include <limits>
#include <algorithm>
#include <optional>
#include <array>
#include <cstdint>
#include <cassert>

// TLinkedData dynamic array toggle
// iff. defined? Use std::vector with actual dimension and value counts,
// otherwise std::array with fixed maximum sizes, see below.
// Maximum sizes are usually:
// - keys:   GLOBAL_MAX_INDEX_DIM = 20
// - values: GMS_VAL_MAX          = 5
#define TLD_DYN_ARRAYS

// Batch allocations
// When TLD_DYN_ARRAYS is active: No single item #TotalSize-bytes new allocations
// but instead allocate big blocks (potentially wasting a couple bytes if items don't fit tightly)
// also has space overhead for list of blocks to free it later
#define TLD_BATCH_ALLOCS

#ifdef TLD_DYN_ARRAYS
#define TLD_TEMPLATE_HEADER template<typename KeyType, typename ValueType>
#define TLD_REC_TYPE TLinkedDataRec<KeyType, ValueType>
#else
#define TLD_TEMPLATE_HEADER template<typename KeyType, int maxKeySize, typename ValueType, int maxValueSize>
#define TLD_REC_TYPE TLinkedDataRec<KeyType, maxKeySize, ValueType, maxValueSize>
#endif

namespace gdlib::datastorage {

#ifdef TLD_BATCH_ALLOCS
    struct DataBatch {
        DataBatch *next;
        uint8_t *ptr;
        explicit DataBatch(size_t count) : next{}, ptr{new uint8_t[count]} {}
        ~DataBatch() {
            delete [] ptr;
        }
    };

    template<int batchSize>
    class BatchAllocator {
        DataBatch *head, *tail;
        size_t offsetInTail;

    public:
        BatchAllocator() : head{}, tail{}, offsetInTail{} {}

        ~BatchAllocator() {
            clear();
        }

        void clear() {
            if(!head) return;
            DataBatch *next;
            for (DataBatch* it = head; it; it = next) {
                next = it->next;
                delete it;
            }
            head = tail = nullptr;
        }

        uint8_t *GetBytes(size_t count) {
            assert(count <= batchSize);
            if(!head) {
                head = tail = new DataBatch{ batchSize };
                offsetInTail = 0;
            }
            else if(batchSize - offsetInTail < count) {
                tail->next = new DataBatch{ batchSize };
                tail = tail->next;
                offsetInTail = 0;
            }
            auto res {tail->ptr + offsetInTail};
            offsetInTail += count;
            return res;
        }
    };
#endif

    TLD_TEMPLATE_HEADER
    struct TLinkedDataRec {
        TLinkedDataRec *RecNext{};
#ifndef TLD_DYN_ARRAYS
        ValueType RecData[maxValueSize];
        KeyType RecKeys[maxKeySize];
#else
        // when RecData is used, first dim * sizeof(int) bytes are keys and then datasize * sizeof(double) bytes for values
        // hence data bytes start at offset FKeySize
        // when RecKeys is used corresponds directly to key entries (as integers)
        union {
            uint8_t RecData[5];
            int RecKeys[20];
        };
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

#ifdef TLD_BATCH_ALLOCS
        BatchAllocator<960> batchAllocator;
#endif

        bool IsSorted() {
            RecType *R{FHead};
            auto *PrevKey {R->RecKeys};
            R = R->RecNext;
            int KD{};
            while(R) {
                for(int D{}; D<FDimension; D++) {
                    KD = R->RecKeys[D] - PrevKey[D];
                    if(KD) break;
                }
                if(KD < 0) return false;
                PrevKey = R->RecKeys;
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
            FTotalSize{1 * (int)sizeof(void *) + FKeySize + FDataSize},
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
#ifdef TLD_BATCH_ALLOCS
            batchAllocator.clear();
#else
            RecType *P {FHead};
            while(P) {
                auto Pn = P->RecNext;
                delete P;
                P = Pn;
            }
#endif
            FCount = FMaxKey = 0;
            FHead = FTail = nullptr;
            FMinKey = std::numeric_limits<int>::max();
        }

        int MemoryUsed() {
            return FCount * FTotalSize;
        }

        RecType *AddItem(const KeyType *AKey, const ValueType *AData) {
#ifdef TLD_DYN_ARRAYS
    #ifdef TLD_BATCH_ALLOCS
            auto* node = reinterpret_cast<RecType *>(batchAllocator.GetBytes(FTotalSize));
    #else
            auto* node = reinterpret_cast<RecType *>(new uint8_t[FTotalSize]);
    #endif
#else
            RecType *node = new RecType { FDimension, FDataSize / (int)sizeof(ValueType) };
#endif
            if(!FHead) FHead = node;
            else FTail->RecNext = node;
            FTail = node;
            node->RecNext = nullptr;
#ifndef TLD_DYN_ARRAYS
            std::memcpy(node->RecKeys, AKey, FKeySize);
            std::memcpy(node->RecData, AData, FDataSize);
#else
            std::memcpy(node->RecData, AKey, FKeySize); // first FKeySize bytes for keys (integers)
            std::memcpy(&node->RecData[FKeySize], AData, FDataSize); // rest for actual data (doubles)
#endif
            FCount++;
            for(int D{}; D<FDimension; D++) {
                int Key{AKey[D]};
                if(Key > FMaxKey) FMaxKey = Key;
                if(Key < FMinKey) FMinKey = Key;
            }
            return node;
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
#ifndef TLD_DYN_ARRAYS
                std::memcpy(AKey, it.RecKeys, FKeySize);
                std::memcpy(AData, it.RecData, FDataSize);
#else
                std::memcpy(AKey, it.RecData, FKeySize); // first FKeySize bytes for keys (integers)
                std::memcpy(AData, &it.RecData[FKeySize], FDataSize); // rest actual data bytes (doubles)
#endif
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
        using KeyArray = KeyType *;
        using ValArray = ValType *;
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
                PrevKey = keys;
            }
            return true;
        };

    public:
        TLinkedData(int ADimension, int ADataSize) :
            FDimension{ADimension},
            FKeySize{static_cast<int>(ADimension*sizeof(int))},
            FDataSize{ADataSize}{
        }

        ~TLinkedData() {
            Clear();
        }

        int Count() const {
            return static_cast<int>(data.size());
        }

        void Clear() {
            for (auto &[k,v] : data) {
                delete[] k;
                delete[] v;
            }
            data.clear();
        }

        ValArray &AddItem(const KeyType *AKey, const ValType *AData) {
#ifdef TLD_DYN_ARRAYS
            KeyArray keys{ new KeyType[FDimension] };
            ValArray vals{ new ValType[FDataSize / (int)sizeof(double)] };
            data.emplace_back(std::make_pair(keys, vals));
#else
            data.push_back({});
#endif
            memcpy(data.back().first, AKey, FKeySize);
            memcpy(data.back().second, AData, FDataSize);
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
            memcpy(AKey, item.first, FKeySize);
            memcpy(Data, item.second, FDataSize);
            it++;
            return true;
        }

        int MemoryUsed() const {
            return (int)(data.size() * sizeof(EntryType));
        }
    };

}

