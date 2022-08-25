#include "doctest.h"
#include "../simplegdx/tgx.h"
#include <array>
#include <filesystem>
#include "../expertapi/gdxcc.h"
#include "../kvbuffers.h"

using namespace simplegdx;
using namespace kvbuffers;

namespace tests::tgxtests {
    TEST_SUITE_BEGIN("tgx");

    const std::string testfn {"tgxtest.gdx"};

    void writeFileWithTGX() {
        GAMSDataExchange gdx {testfn, "TGX", false};

        SymbolRaw sym;
        sym.name = "Demand";
        sym.explanatory_text = "Demand data";
        sym.dimension = 1;
        sym.type = 1; // parameter

        // Data section
        int minKey{1}, maxKey{3};
        REQUIRE(gdx.WriteRecordHeader(sym, &minKey, &maxKey) >= 0);

        auto writeRec = [&sym, &gdx](int32_t key, double val) {
            static std::array<double, 5> vals {};
            vals[0] = val;
            REQUIRE(gdx.WriteRecord(sym, &key, vals.data()));
        };
        writeRec(1, 324.0);
        writeRec(2, 299.0);
        writeRec(3, 274.0);

        // Symbol section
        REQUIRE_NE(-1, gdx.WriteSymbolHeader(1));
        REQUIRE(gdx.WriteSymbol(sym));

        // UEL section
        REQUIRE(gdx.WriteUELHeader(3) >= 0);
        REQUIRE(gdx.WriteUEL("New-York"));
        REQUIRE(gdx.WriteUEL("Chicago"));
        REQUIRE(gdx.WriteUEL("Topeka"));
    }

    void checkFileWithGDXCC() {
        const std::array<std::pair<std::string, double>, 3> exampleData {{
            {"New-York", 324.0},
            {"Chicago", 299.0},
            {"Topeka", 274.0}
        }};

        REQUIRE(std::filesystem::exists(testfn));
        std::array<char, 256> msgBuf {};
        gdxHandle_t pgx {};
        REQUIRE(gdxCreate(&pgx, msgBuf.data(), msgBuf.size()));
        REQUIRE_EQ('\0', msgBuf.front());

        int ErrNr{};
        REQUIRE(gdxOpenRead(pgx, testfn.c_str(), &ErrNr));
        REQUIRE_EQ(0, ErrNr);

        std::array<char, 256> name{};
        int dim{}, typ{};
        gdxSymbolInfo(pgx, 1, name.data(), &dim, &typ);
        REQUIRE(!strcmp(name.data(), "Demand"));
        REQUIRE_EQ(1, dim);
        REQUIRE_EQ(1, typ);

        int nrRecs;
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

        gdxFree(&pgx);
    }

    // TODO: Complete this one
    void checkFileWithTGX() {
        const std::array<std::pair<std::string, double>, 3> exampleData
        {{
                 {"New-York", 324.0},
                 {"Chicago", 299.0},
                 {"Topeka", 274.0}
         }};

        GAMSDataExchange gdx{testfn};
        gdx.ReadSymbolHeader();
        auto sym = gdx.ReadSymbol();
        gdx.ReadRecordHeader(sym);

        auto readRec = [&gdx]() {
            static std::array<int32_t, 20> keys{};
            static std::array<double, 5> vals{};
            gdx.ReadRecord(keys.data(), vals.data());
            return std::make_pair(keys[0], vals[0]);
        };

        for(int i{1}; i<=3; i++) {
            auto pair = readRec();
            REQUIRE_EQ(i, pair.first);
            REQUIRE_EQ(exampleData[i-1].second, pair.second);
        }
    }

    TEST_CASE("Test simple gdx file creation") {
        writeFileWithTGX();
        checkFileWithTGX();
        checkFileWithGDXCC();
        std::filesystem::remove(testfn);
    }

    TEST_SUITE_END();
}