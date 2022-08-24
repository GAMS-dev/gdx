#include "doctest.h"
#include "../expertapi/gdxcc.h"
#include <array>
#include <cstring>
#include <map>
#include <string>
#include <filesystem>

using namespace std::literals::string_literals;

namespace tests::xptests {
    TEST_SUITE_BEGIN("expert level api");

    TEST_CASE("Test writing and reading demand data for gamslib/trnsport to/from GDX") {
        std::array<char, 256> msgBuf {};
        gdxHandle_t pgx {};
        REQUIRE(gdxCreate(&pgx, msgBuf.data(), msgBuf.size()));
        REQUIRE_EQ('\0', msgBuf.front());

        // Write data
        int ErrNr{};
        const std::string fn {"demanddata.gdx"};
        REQUIRE(gdxOpenWrite(pgx, fn.c_str(), "xptests" , &ErrNr));
        REQUIRE_EQ(0, ErrNr);
        REQUIRE(gdxDataWriteStrStart(pgx, "Demand", "Demand data", 1, GMS_DT_PAR, 0));
        gdxStrIndex_t strBuffers;
        gdxStrIndexPtrs_t strPtrs;
        GDXSTRINDEXPTRS_INIT(strBuffers, strPtrs);
        gdxValues_t vals;
        std::memset(vals, 0, sizeof(double)*5);
        std::array<std::pair<std::string, double>, 3> exampleData {{
            {"New-York"s, 324.0},
            {"Chicago"s, 299.0},
            {"Topeka"s, 274.0}
        }};
        for(const auto &[key, value] : exampleData) {
            std::strcpy(strBuffers[0], key.c_str());
            vals[GMS_VAL_LEVEL] = value;
            gdxDataWriteStr(pgx, const_cast<const char **>(strPtrs), vals);
        }
        REQUIRE(gdxDataWriteDone(pgx));
        gdxClose(pgx);

        REQUIRE(std::filesystem::exists(fn));

        // Read data
        REQUIRE(gdxOpenRead(pgx, fn.c_str(), &ErrNr));
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

        gdxClose(pgx);

        gdxFree(&pgx);

        std::filesystem::remove(fn);
    }

    TEST_SUITE_END();
}