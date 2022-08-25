#include "doctest.h"
#include "../simplegdx/tgx.h"
#include <array>
#include <filesystem>
#include "../expertapi/gdxcc.h"

using namespace simplegdx;

namespace tests::tgxtests {
    TEST_SUITE_BEGIN("tgx");

    const std::string testfn {"test.gdx"};

    void writeFileWithTGX() {
        GAMSDataExchange gdx {testfn, "TGX", false};
        gdx.WriteSymbolHeader(1);
        SymbolRaw sym;
        sym.name = "demand";
        sym.explanatory_text = "demand data";
        sym.dimension = 1;
        sym.type = 1; // parameter
        gdx.WriteSymbol(sym);
        gdx.WriteUELs({"New-York"});
        std::array<int32_t, 1> keys {1};
        std::array<double, 5> vals {};
        vals[0] = 324.0;
        int minKey{1}, maxKey{255};
        gdx.WriteRecordHeader(sym, &minKey, &maxKey);
        gdx.WriteRecord(sym, keys.data(), vals.data());
    }

    void checkFileWithGDXCC() {
        REQUIRE(std::filesystem::exists(testfn));
        std::array<char, 256> msgBuf {};
        gdxHandle_t pgx {};
        gdxCreate(&pgx, msgBuf.data(), msgBuf.size());

        int ErrNr{};
        REQUIRE(gdxOpenRead(pgx, testfn.c_str(), &ErrNr));
        REQUIRE_EQ(0, ErrNr);

        std::array<char, 256> name{};
        int dim{}, typ{};
        gdxSymbolInfo(pgx, 1, name.data(), &dim, &typ);
        REQUIRE(!strcmp(name.data(), "demand"));
        REQUIRE_EQ(1, dim);
        REQUIRE_EQ(1, typ);

        gdxFree(&pgx);
    }

    TEST_CASE("Test simple gdx file creation") {
        writeFileWithTGX();
        checkFileWithGDXCC();
        std::filesystem::remove("test.gdx");
    }

    TEST_SUITE_END();
}