#include <filesystem>
#include "doctest.h"
#include "../xpwrap.h"
#include "../gxfile.h"

namespace tests::gdxinterfacetests {
    TEST_SUITE_BEGIN("GDX interface tests");

    void writeFileWithSingleUEL(gdxinterface::GDXInterface &pgx, const std::string &fn, int &ErrNr) {
        pgx.gdxOpenWrite(fn, "xptests", ErrNr);
        pgx.gdxUELRegisterRawStart();
        pgx.gdxUELRegisterRaw("New-York");
        pgx.gdxUELRegisterDone();
        pgx.gdxClose();
    }

    void readFileWithSingleUEL(gdxinterface::GDXInterface &pgx, const std::string &fn, int &ErrNr) {
        pgx.gdxOpenRead(fn, ErrNr);
        int uelCnt, highMap, uelMap;
        std::string uel;
        pgx.gdxUMUelInfo(uelCnt, highMap);
        REQUIRE_EQ(0, highMap);
        REQUIRE_EQ(1, uelCnt);
        pgx.gdxUMUelGet(1, uel, uelMap);
        REQUIRE_EQ("New-York", uel);
        REQUIRE_EQ(-1, uelMap);
        pgx.gdxClose();
    }

    template<typename T> void runAddingUELTest() {
        int ErrNr;
        std::string ErrMsg, fn {"gdxinterfacetest.gdx"};
        {
            auto pgxObj {std::make_unique<T>(ErrMsg)};
            writeFileWithSingleUEL(*pgxObj, fn, ErrNr);
        }
        {
            auto pgxObj {std::make_unique<T>(ErrMsg)};
            readFileWithSingleUEL(*pgxObj, fn, ErrNr);
        }
        std::filesystem::remove(fn);
    };

    TEST_CASE("Test adding uels") {
        runAddingUELTest<xpwrap::GDXFile>();
        //runAddingUELTest<gxfile::TGXFileObj>();
    }

    TEST_SUITE_END();

}