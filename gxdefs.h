#pragma once

#include "expertapi/gclgms.h"
#include <array>
#include <string>

namespace gxdefs {
    const int   DOMC_UNMAPPED = -2, // indicator for unmapped index pos
                DOMC_EXPAND = -1, // indicator growing index pos
                DOMC_STRICT = 0; // indicator mapped index pos
    using TgdxUELIndex = std::array<int, GMS_MAX_INDEX_DIM>;
    using TgdxStrIndex = std::array<std::string, GMS_MAX_INDEX_DIM>;
    using TgdxValues = std::array<double, GMS_VAL_SCALE+ 1>;
    using TgdxSVals = std::array<double, 7>;
    const std::array<int, GMS_DT_ALIAS+1> DataTypSize {1,1,5,5,0};
    bool GoodUELString(const std::string &s);
}