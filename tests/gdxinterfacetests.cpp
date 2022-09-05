#include <filesystem>
#include "doctest.h"
#include "../xpwrap.h"
#include "../gxfile.h"
#include "../utils.h"

using namespace std::literals::string_literals;

namespace tests::gdxinterfacetests {
    TEST_SUITE_BEGIN("GDX interface tests");

    template<typename T>
    void testWriteOp(const std::string &fn,
                     const std::function<void(gdxinterface::GDXInterface&)> &cb,
                     bool cleanup = false) {
        {
            std::string ErrMsg;
            T pgx{ErrMsg};
            int ErrNr;
            pgx.gdxOpenWrite(fn, "xpwraptest", ErrNr);
            cb(pgx);
            pgx.gdxClose();
        }
        if(cleanup)
            std::filesystem::remove(fn);
    }

    template<typename T>
    void testReadOp(const std::string &fn,
                    const std::function<void(gdxinterface::GDXInterface&)> &cb,
                    bool cleanup = false) {
        {
            std::string ErrMsg;
            T pgx{ErrMsg};
            int ErrNr;
            pgx.gdxOpenRead(fn, ErrNr);
            cb(pgx);
            pgx.gdxClose();
        }
        if(cleanup)
            std::filesystem::remove(fn);
    }

    void testReads(const std::string &filename1,
                   const std::string &filename2,
                   const std::function<void(gdxinterface::GDXInterface&)> &cb) {
        testReadOp<xpwrap::GDXFile>(filename1, cb);
        testReadOp<gxfile::TGXFileObj>(filename2, cb);
    }

    void testMatchingWrites(const std::string &filename1,
                            const std::string &filename2,
                            const std::function<void(gdxinterface::GDXInterface&)> &cb) {
        testWriteOp<xpwrap::GDXFile>(filename1, cb);
        testWriteOp<gxfile::TGXFileObj>(filename2, cb);
        auto maybeMismatches = utils::binaryFileDiff(filename1, filename2);
        if (maybeMismatches) {
            for (const auto& mm : *maybeMismatches)
                std::cout   << "Mismatch at offset " << mm.offset << " with "
                            << filename1 << " = " << mm.lhs << " and "
                            << filename2 <<"= " << mm.rhs << "\n";
            std::cout << std::endl;
        }
        else std::cout << "No mismatches found!" << std::endl;
        REQUIRE_FALSE(maybeMismatches);
    }

    TEST_CASE("Test adding uels") {
        std::array filenames{ "uel_wrapper.gdx"s, "uel_port.gdx"s };
        auto [f1, f2] = filenames;
        testMatchingWrites(f1, f2, [](gdxinterface::GDXInterface &pgx) {
            pgx.gdxUELRegisterRawStart();
            pgx.gdxUELRegisterRaw("New-York");
            pgx.gdxUELRegisterDone();
        });
        testReads(f1, f2, [](gdxinterface::GDXInterface &pgx) {
            int uelCnt, highMap, uelMap;
            std::string uel;
            pgx.gdxUMUelInfo(uelCnt, highMap);
            REQUIRE_EQ(0, highMap);
            REQUIRE_EQ(1, uelCnt);
            pgx.gdxUMUelGet(1, uel, uelMap);
            REQUIRE_EQ("New-York", uel);
            REQUIRE_EQ(-1, uelMap);
        });
        for (const auto& fn : filenames)
            std::filesystem::remove(fn);
    }

    TEST_SUITE_END();

}