#include "doctest.h"
#include "../expertapi/gdxcc.h"
#include <array>
#include <cstring>
#include <map>
#include <string>
#include <filesystem>
#include "../utils.h"
#include "../kvbuffers.h"

using namespace std::literals::string_literals;
using namespace kvbuffers;

namespace tests::xptests {
    TEST_SUITE_BEGIN("expert level api");

    void writeDemandData(gdxHandle_t pgx, const std::string &fn, const std::array<std::pair<std::string, double>, 3> &exampleData);
    void readDemandData(gdxHandle_t pgx, const std::string &fn, const std::array<std::pair<std::string, double>, 3> &exampleData);
    gdxHandle_t setupGdxObject();

    void writeDemandData(gdxHandle_t pgx,
                         const std::string &fn,
                         const std::array<std::pair<std::string, double>, 3> &exampleData) {
        int ErrNr{};
        REQUIRE(gdxOpenWrite(pgx, fn.c_str(), "xptests" , &ErrNr));
        REQUIRE_EQ(0, ErrNr);
        REQUIRE(gdxDataWriteStrStart(pgx, "Demand", "Demand data", 1, GMS_DT_PAR, 0));

        KVBuffers bufs;
        for(const auto &[key, value] : exampleData) {
            utils::assignStrToBuf(key, bufs.strBuffers[0]);
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
        REQUIRE_EQ(3, nrRecs);
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

    gdxHandle_t setupGdxObject() {
        std::array<char, 256> msgBuf {};
        gdxHandle_t pgx {};
        REQUIRE(gdxCreate(&pgx, msgBuf.data(), (int)msgBuf.size()));
        REQUIRE_EQ('\0', msgBuf.front());
        return pgx;
    };

    TEST_CASE("Test writing and reading demand data for gamslib/trnsport to/from GDX") {
        auto pgx = setupGdxObject();

        const std::array<std::pair<std::string, double>, 3> exampleData
        {
            {
                {"New-York"s, 324.0},
                {"Chicago"s, 299.0},
                {"Topeka"s, 274.0}
            }
        };
        const std::string fn {"xptest.gdx"};

        writeDemandData(pgx, fn, exampleData);
        readDemandData(pgx, fn, exampleData);

        gdxFree(&pgx);
        std::filesystem::remove(fn);
    }

    TEST_CASE("Playing around with mapped writing and reading") {
        auto pgx = setupGdxObject();
        int ErrNr;
        std::string fn {"mappedread.gdx"};
        gdxOpenWrite(pgx, fn.c_str(), "xptests", &ErrNr);
        gdxDataWriteMapStart(pgx, "a", "A symbol", 1, GMS_DT_PAR, 0);
        std::array<int, 20> keys{};
        std::array<double, 5> vals{};
        for(int i{}; i<3; i++) {
            keys[i] = i+1;
            vals[i] = i+1;
            gdxDataWriteMap(pgx, keys.data(), vals.data());
        }
        gdxDataWriteDone(pgx);
        gdxFree(&pgx);
        std::filesystem::remove(fn);
    }

    TEST_SUITE_END();
}