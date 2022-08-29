#pragma once

#include "global/gmsspecs.h"

namespace gxdefs {

    const int   DOMC_UNMAPPED = -2, // indicator for unmapped index pos
                DOMC_EXPAND = -1, // indicator growing index pos
                DOMC_STRICT = 0; // indicator mapped index pos

    using TgdxUELIndex = global::gmsspecs::TIndex;
    using TgdxStrIndex = global::gmsspecs::TStrIndex;

    using TgdxValues = global::gmsspecs::tvarreca;

    using TgdxSVals = std::array<double, 7>;

    const std::array<std::string, 5>    gdxDataTypStr {"Set", "Par", "Var", "Equ", "Alias"},
                                        gdxDataTypStrL {"Set", "Parameter", "Variable", "Equation", "Alias"};
    const std::array<int, global::gmsspecs::TgdxDataType::dt_alias+1> DataTypSize {1,1,5,5,0};

    const std::array<std::string, 7> gdxSpecialValuesStr {
            "Undf"  /*sv_valund */,
            "NA"    /*sv_valna  */,
            "+Inf"  /*sv_valpin */,
            "-Inf"  /*sv_valmin */,
            "Eps"   /*sv_valeps */,
            "0"     /*sv_normal */,
            "AcroN" /*acronym   */
    };

    bool CanBeQuoted(const std::string &s);
    bool GoodUELString(const std::string &s);

}