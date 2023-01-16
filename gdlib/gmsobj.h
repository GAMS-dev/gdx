#pragma once

#include <cstdint>
#include "datastorage.h"

namespace gdlib::gmsobj {

    using TByteDataArray = uint8_t[10000000];
    using PByteDataArray = TByteDataArray *;

    class TBooleanBitArray {
        PByteDataArray PData;
        int FAllocated, FHighIndex;

        void GetBitMask(int V, int &N, uint8_t &M) {
            N = V >> 3;
            M = 1 << (V & 0x7);
        }

        void SetHighIndex(int V) {
            if(V > FHighIndex) {
                int NewMemSize {(V + 8) / 8};
                if(NewMemSize > FAllocated) {
                    int Delta{};
                    do {
                        if(!FAllocated) Delta += 256;
                        else if(FAllocated < 32 * 256) Delta += FAllocated;
                        else Delta += FAllocated / 4;
                    } while(NewMemSize < FAllocated + Delta);
                    NewMemSize = FAllocated + Delta;
                    PByteDataArray NewMem = reinterpret_cast<PByteDataArray>(new uint8_t[NewMemSize]);
                    memset(NewMem, 0, NewMemSize);
                    if(FAllocated) {
                        memcpy(NewMem, PData, FAllocated);
                        delete [] PData;
                    }
                    PData = NewMem;
                    FAllocated = NewMemSize;
                }
                FHighIndex = V;
            }
        }

    public:
        TBooleanBitArray() : PData{}, FAllocated{}, FHighIndex{-1} {
        }

        ~TBooleanBitArray() {
            if(FAllocated > 0)
                delete [] PData;
        }

        bool GetBit(int N) {
            if(N < 0 || N > FHighIndex) return false;
            int P;
            uint8_t M;
            GetBitMask(N, P, M);
            return reinterpret_cast<PByteDataArray>(PData)[P] && M;
        }

        void SetBit(int N, bool V) {
            if(N >= 0) {
                if(N >= FHighIndex) {
                    if(!V) return;
                    SetHighIndex(N);
                }
                int P;
                uint8_t M;
                GetBitMask(N, P, M);
                if(V) *PData[P] |= M;
                else *PData[P] &= !M;
            }
        }
    };

}