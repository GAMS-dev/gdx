#include "doctest.h"
#include "../simplegdx/tgx.h"
#include <array>
#include <filesystem>

using namespace simplegdx;

namespace tests::tgxtests {
    TEST_SUITE_BEGIN("tgx");

    void writeFileWithTGX() {
        GAMSDataExchange gdx {"test.gdx"};
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
        gdx.WriteRecord(sym, keys.data(), vals.data());
    }

    TEST_CASE("Test simple gdx file creation") {
        writeFileWithTGX();
        std::filesystem::remove("test.gdx");
    }

    TEST_SUITE_END();
}