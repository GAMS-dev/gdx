#include "doctest.h"
#include "../gxfile.h"
#include <filesystem>

using namespace gxfile;
using namespace std::literals::string_literals;

namespace tests::gxfiletests {
    TEST_SUITE_BEGIN("gxfile");

    TEST_CASE("Test creating a simple gdx file with gxfile port") {
        std::string msg;
        {
            TGXFileObj pgx{msg};
            int ErrNr;
            pgx.gdxOpenWrite("mytest.gdx", "TGXFileObjPort", ErrNr);
            pgx.gdxDataWriteStrStart("Demand", "Demand data", 1, 1, 0);
            auto writeRec = [&pgx](const std::string &s, double v) {
                static std::array<std::string, 20> keys{};
                static std::array<double, 5> values{};
                keys.front() = s;
                values.front() = v;
                pgx.gdxDataWriteStr(keys, values);
            };
            writeRec("New-York"s, 324.0);
            writeRec("Chicago"s, 299.0);
            writeRec("Topeka"s, 274.0);
            pgx.gdxDataWriteDone();
            pgx.gdxClose();
        }
        std::filesystem::remove("mytest.gdx");
    }

    TEST_SUITE_END();
}