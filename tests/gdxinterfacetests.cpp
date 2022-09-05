#include <filesystem>
#include "doctest.h"
#include "../xpwrap.h"
#include "../gxfile.h"
#include "../utils.h"

using namespace std::literals::string_literals;

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

    enum UELTestOps {
        read = 0x01,
        write = 0x10,
        readAndWrite = 0x11
    };

    template<typename T> void runAddingUELTest(const std::string &fn, UELTestOps ops, bool cleanup=false) {
        int ErrNr;
        std::string ErrMsg;
        if(ops & write)
        {
            auto pgxObj {std::make_unique<T>(ErrMsg)};
            writeFileWithSingleUEL(*pgxObj, fn, ErrNr);
        }
        if(ops & read)
        {
            auto pgxObj {std::make_unique<T>(ErrMsg)};
            readFileWithSingleUEL(*pgxObj, fn, ErrNr);
        }
        if(cleanup)
            std::filesystem::remove(fn);
    };

    TEST_CASE("Test adding uels") {
        std::array filenames{ "uel_wrapper.gdx"s, "uel_port.gdx"s };
        runAddingUELTest<xpwrap::GDXFile>(filenames.front(), write);
        runAddingUELTest<gxfile::TGXFileObj>(filenames.back(), write);
        auto maybeMismatches = utils::binaryFileDiff(filenames.front(), filenames.back());
        if (maybeMismatches) {
            for (const auto& mm : *maybeMismatches)
                std::cout << "Mismatch at offset " << mm.offset << " with left hand side = " << mm.lhs << " and right hand side = " << mm.rhs << "\n";
            std::cout << std::endl;
        }
        else std::cout << "No mismatches found!" << std::endl;
        runAddingUELTest<xpwrap::GDXFile>(filenames.front(), read, true);
        runAddingUELTest<gxfile::TGXFileObj>(filenames.back(), read, true);
    }

    TEST_SUITE_END();

}