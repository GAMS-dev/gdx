#include "doctest.h"
#include "../xpwrap.h"
#include <array>
#include <cstring>
#include <map>
#include <string>
#include <filesystem>
#include <iostream>
#include "../global/gmsspecs.h"
#include "../gxdefs.h"

using namespace std::literals::string_literals;
using namespace xpwrap;

namespace tests::xpwraptests {
    TEST_SUITE_BEGIN("expert level wrapper api");

    TEST_CASE("Test writing and reading demand data for gamslib/trnsport to/from GDX") {
        GDXFile pgx;

        // Write data
        int ErrNr{};
        const std::string fn {"xpwraptest.gdx"};
        REQUIRE(pgx.gdxOpenWrite(fn, "xptests" , ErrNr));
        REQUIRE_EQ(0, ErrNr);
        REQUIRE(pgx.gdxDataWriteStrStart("Demand", "Demand data", 1, 1, 0));
        std::array<std::pair<std::string, double>, 3> exampleData {{
                                                                           {"New-York"s, 324.0},
                                                                           {"Chicago"s, 299.0},
                                                                           {"Topeka"s, 274.0}
                                                                   }};
        gdxinterface::TgdxStrIndex keys;
        gdxinterface::TgdxValues  vals;
        for(const auto &[key, value] : exampleData) {
            keys[0] = key;
            vals[0] = value;
            pgx.gdxDataWriteStr( keys, vals);
        }
        REQUIRE(pgx.gdxDataWriteDone());
        pgx.gdxClose();

        REQUIRE(std::filesystem::exists(fn));

        // Read data
        REQUIRE(pgx.gdxOpenRead(fn, ErrNr));
        REQUIRE_EQ(0, ErrNr);
        int nrRecs{};
        REQUIRE(pgx.gdxDataReadStrStart(1, nrRecs));

        gxdefs::TgdxStrIndex index{};
        gxdefs::TgdxValues values{};
        int dimFrst{};

        for(int i{}; i<nrRecs; i++) {
            pgx.gdxDataReadStr(index, values, dimFrst);
            REQUIRE_EQ(index[0], exampleData[i].first);
            REQUIRE(vals[global::gmsspecs::vallevel] - exampleData[i].second < 0.001);
        }
        REQUIRE(pgx.gdxDataReadDone());

        pgx.gdxClose();

        std::filesystem::remove(fn);
    }

    TEST_SUITE_END();
}