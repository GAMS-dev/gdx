#pragma once

#include <cstring>
#include "expertapi/gdxcc.h"

namespace kvbuffers {

    class KVBuffers {
    public:
        gdxStrIndex_t strBuffers{};
        gdxStrIndexPtrs_t strPtrs{};
        gdxValues_t vals{};

        KVBuffers() {
            GDXSTRINDEXPTRS_INIT(strBuffers, strPtrs);
            std::memset(vals, 0, sizeof(double)*5);
        }
    };

}