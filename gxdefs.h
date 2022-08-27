#pragma once

#include "global/gmsspecs.h"

namespace gxdefs {

    const int   DOMC_UNMAPPED = -2, // indicator for unmapped index pos
                DOMC_EXPAND = -1, // indicator growing index pos
                DOMC_STRICT = 0; // indicator mapped index pos

    using TgdxUELIndex = global::gmsspecs::TIndex;
    using TgdxStrIndex = global::gmsspecs::TStrIndex;

    using TgdxValues = global::gmsspecs::tvarreca;

    // ...

}