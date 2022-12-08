#include "doctest.h"
#include "../xpwrap.h"
#include <array>
#include <cstring>
#include <map>
#include <string>
#include <filesystem>
#include <iostream>

using namespace std::literals::string_literals;
using namespace xpwrap;
using namespace gdxinterface;

namespace tests::xpwraptests {
    TEST_SUITE_BEGIN("expert level wrapper api");

    const std::string fn {"xpwraptest.gdx"};

    const std::array<std::pair<std::string, double>, 3> exampleData {{
        {"New-York"s, 324.0},
        {"Chicago"s, 299.0},
        {"Topeka"s, 274.0}
    }};

    TEST_CASE("Test writing and reading demand data for gamslib/trnsport to/from GDX") {
        std::string ErrMsg;
        int ErrNr{};
        TgdxStrIndex keys;
        TgdxValues  vals;

        // Write data
        {
            GDXFile pgx{ ErrMsg };
            REQUIRE(ErrMsg.empty());
            REQUIRE(pgx.gdxOpenWrite(fn, "xptests", ErrNr));
            REQUIRE_EQ(0, ErrNr);
            REQUIRE(pgx.gdxDataWriteStrStart("Demand", "Demand data", 1, 1, 0));
            
            for (const auto& [key, value] : exampleData) {
                keys[0] = key;
                vals[0] = value;
                const char *keyptrs[1] = { key.c_str() };
                pgx.gdxDataWriteStr(keyptrs, vals.data());
            }
            REQUIRE(pgx.gdxDataWriteDone());
            pgx.gdxClose();
        }

        REQUIRE(std::filesystem::exists(fn));

        // Read data
        {
            GDXFile pgx{ ErrMsg };
            REQUIRE(ErrMsg.empty());
            REQUIRE(pgx.gdxOpenRead(fn, ErrNr));
            REQUIRE_EQ(0, ErrNr);
            int nrRecs{};
            REQUIRE(pgx.gdxDataReadStrStart(1, nrRecs));

            std::array<char, 256> buf {};
            char *index[] = { buf.data() };
            TgdxValues values{};
            int dimFrst{};

            for (int i{}; i < nrRecs; i++) {
                pgx.gdxDataReadStr(index, values.data(), dimFrst);
                REQUIRE_EQ(index[0], exampleData[i].first);
                REQUIRE(vals[GMS_VAL_LEVEL] - exampleData[i].second < 0.001);
            }
            REQUIRE(pgx.gdxDataReadDone());

            pgx.gdxClose();
        }

        std::filesystem::remove(fn);
    }



    TEST_SUITE_END();
}