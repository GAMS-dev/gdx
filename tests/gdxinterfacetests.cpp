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
        if(fn.empty()) return;

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
        if(fn.empty()) return;

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
        if(!filename1.empty() && !filename2.empty()) {
            auto maybeMismatches = utils::binaryFileDiff(filename1, filename2);
            if (maybeMismatches) {
                for (const auto &mm: *maybeMismatches)
                    std::cout << "Mismatch at offset " << mm.offset << " with "
                              << filename1 << " = " << mm.lhs << " and "
                              << filename2 << "= " << mm.rhs << "\n";
                std::cout << std::endl;
            } else std::cout << "No mismatches found!" << std::endl;
            REQUIRE_FALSE(maybeMismatches);
        }
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

    TEST_CASE("Test write and read record raw") {
        std::string f1{"rwrecordraw_wrapper.gdx"},
                    f2{"rwrecordraw_port.gdx"};
        gxdefs::TgdxUELIndex keys{};
        gxdefs::TgdxValues values{};
        testMatchingWrites(f1, f2, [&](gdxinterface::GDXInterface &pgx) {
            pgx.gdxUELRegisterRawStart();
            pgx.gdxUELRegisterRaw("TheOnlyUEL");
            pgx.gdxUELRegisterDone();
            pgx.gdxDataWriteRawStart("mysym", "This is my symbol!", 1, global::gmsspecs::gms_dt_par, 0);
            keys[0] = 1;
            values[global::gmsspecs::vallevel] = 3.141;
            pgx.gdxDataWriteRaw(keys, values);
            pgx.gdxDataWriteDone();
        });
        testReads(f1, f2, [&](gdxinterface::GDXInterface &pgx) {
            std::string uel;
            int uelMap;
            pgx.gdxUMUelGet(1, uel, uelMap);
            REQUIRE_EQ("TheOnlyUEL", uel);
            int NrRecs;
            pgx.gdxDataReadRawStart(1, NrRecs);
            REQUIRE_EQ(1, NrRecs);
            int dimFrst;
            pgx.gdxDataReadRaw(keys, values, dimFrst);
            pgx.gdxDataReadDone();
            REQUIRE_EQ(1, keys[0]);
            REQUIRE_EQ(3.141, values[global::gmsspecs::vallevel]);
        });
    }

    TEST_CASE("Test write and read record mapped") {
        std::string f1{"rwrecordmapped_wrapper.gdx"},
                f2/*{"rwrecordmapped_port.gdx"}*/;
        gxdefs::TgdxUELIndex  keys{};
        gxdefs::TgdxValues values{};
        testMatchingWrites(f1, f2, [&](gdxinterface::GDXInterface &pgx) {
            pgx.gdxUELRegisterMapStart();
            pgx.gdxUELRegisterMap(3, "TheOnlyUEL");
            pgx.gdxUELRegisterDone();
            pgx.gdxDataWriteMapStart("mysym", "This is my symbol!", 1, global::gmsspecs::gms_dt_par, 0);
            keys[0] = 3;
            values[global::gmsspecs::vallevel] = 3.141;
            pgx.gdxDataWriteMap(keys, values);
            pgx.gdxDataWriteDone();
        });
        testReads(f1, f2, [&](gdxinterface::GDXInterface &pgx) {
            std::string uel;
            int uelMap;
            pgx.gdxUMUelGet(3, uel, uelMap);
            REQUIRE_EQ("TheOnlyUEL", uel);
            int NrRecs;
            pgx.gdxDataReadMapStart(1, NrRecs);
            REQUIRE_EQ(1, NrRecs);
            int dimFrst;
            pgx.gdxDataReadMap(1, keys, values, dimFrst);
            pgx.gdxDataReadDone();
            REQUIRE_EQ(3, keys[0]);
            REQUIRE_EQ(3.141, values[global::gmsspecs::vallevel]);
        });
    }

    TEST_SUITE_END();

}