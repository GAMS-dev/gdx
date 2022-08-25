#include "doctest.h"
#include "../expertapi/gdxcc.h"
#include <array>
#include <cstring>
#include <map>
#include <string>
#include <filesystem>
#include "../kvbuffers.h"

using namespace std::literals::string_literals;
using namespace kvbuffers;

namespace tests::xptests {
    TEST_SUITE_BEGIN("expert level api");

    void writeDemandData(gdxHandle_t pgx,
                         const std::string &fn,
                         const std::array<std::pair<std::string, double>, 3> &exampleData) {
        int ErrNr{};
        REQUIRE(gdxOpenWrite(pgx, fn.c_str(), "xptests" , &ErrNr));
        REQUIRE_EQ(0, ErrNr);
        REQUIRE(gdxDataWriteStrStart(pgx, "Demand", "Demand data", 1, GMS_DT_PAR, 0));

        KVBuffers bufs;
        for(const auto &[key, value] : exampleData) {
            std::strcpy(bufs.strBuffers[0], key.c_str());
            bufs.vals[GMS_VAL_LEVEL] = value;
            gdxDataWriteStr(pgx, const_cast<const char **>(bufs.strPtrs), bufs.vals);
        }
        REQUIRE(gdxDataWriteDone(pgx));
        gdxClose(pgx);

        REQUIRE(std::filesystem::exists(fn));
    }

    void readDemandData(gdxHandle_t pgx,
                        const std::string &fn,
                        const std::array<std::pair<std::string, double>, 3> &exampleData) {
        int ErrNr{};
        REQUIRE(gdxOpenRead(pgx, fn.c_str(), &ErrNr));
        REQUIRE_EQ(0, ErrNr);
        int nrRecs{};
        REQUIRE(gdxDataReadStrStart(pgx, 1, &nrRecs));
        int dimFrst{};
        KVBuffers bufs;
        for(int i{}; i<nrRecs; i++) {
            gdxDataReadStr(pgx, bufs.strPtrs, bufs.vals, &dimFrst);
            REQUIRE_FALSE(strcmp(bufs.strPtrs[0], exampleData[i].first.c_str()));
            REQUIRE_EQ(bufs.vals[GMS_VAL_LEVEL], exampleData[i].second);
        }
        REQUIRE(gdxDataReadDone(pgx));

        gdxClose(pgx);
    }

    TEST_CASE("Test writing and reading demand data for gamslib/trnsport to/from GDX") {
        std::array<char, 256> msgBuf {};
        gdxHandle_t pgx {};
        REQUIRE(gdxCreate(&pgx, msgBuf.data(), msgBuf.size()));
        REQUIRE_EQ('\0', msgBuf.front());

        const std::array<std::pair<std::string, double>, 3> exampleData
        {
            {
                {"New-York"s, 324.0},
                {"Chicago"s, 299.0},
                {"Topeka"s, 274.0}
            }
        };
        const std::string fn {"demanddata.gdx"};

        writeDemandData(pgx, fn, exampleData);
        readDemandData(pgx, fn, exampleData);

        gdxFree(&pgx);
        std::filesystem::remove(fn);
    }

    TEST_SUITE_END();
}