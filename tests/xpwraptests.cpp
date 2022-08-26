#include "doctest.h"
#include "../xpwrap.h"
#include <array>
#include <cstring>
#include <map>
#include <string>
#include <filesystem>

using namespace std::literals::string_literals;
using namespace xpwrap;

namespace tests::xpwraptests {
    TEST_SUITE_BEGIN("expert level wrapper api");

    TEST_CASE("Test writing and reading demand data for gamslib/trnsport to/from GDX") {
        GDXFile pgx;

        // Write data
        int ErrNr{};
        const std::string fn {"xpwraptest.gdx"};
        REQUIRE(pgx.gdxOpenWrite(fn.c_str(), "xptests" , ErrNr));
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
        /*REQUIRE(gdxOpenRead(pgx, fn.c_str(), &ErrNr));
        REQUIRE_EQ(0, ErrNr);
        int nrRecs{};
        REQUIRE(gdxDataReadStrStart(pgx, 1, &nrRecs));
        int dimFrst{};
        for(int i{}; i<nrRecs; i++) {
            gdxDataReadStr(pgx, strPtrs, vals, &dimFrst);
            REQUIRE_FALSE(strcmp(strPtrs[0], exampleData[i].first.c_str()));
            REQUIRE_EQ(vals[GMS_VAL_LEVEL], exampleData[i].second);
        }
        REQUIRE(gdxDataReadDone(pgx));

        pgx.gdxClose();*/

        std::filesystem::remove(fn);
    }

    TEST_SUITE_END();
}