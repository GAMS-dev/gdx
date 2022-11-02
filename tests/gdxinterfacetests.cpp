#include <filesystem>
#include "doctest.h"
#include "../xpwrap.h"
#include "../gxfile.h"
#include "../utils.h"
#include "../rtl/p3utils.h"
#include <cassert>

using namespace std::literals::string_literals;
using namespace gdxinterface;

namespace tests::gdxinterfacetests {
    TEST_SUITE_BEGIN("GDX interface tests");

    //static std::ostream &mycout = std::cout;
    static std::ostream mycout {&gxfile::null_buffer};

    auto getBuilders() {
        static std::string ErrMsg;
        std::list<std::function<GDXInterface*()>> builders {
                [&]() { return new xpwrap::GDXFile{ErrMsg}; },
                [&]() { return new gxfile::TGXFileObj{ErrMsg}; }
        };
        return builders;
    }

    void basicTest(const std::function<void(GDXInterface&)> &cb, int ub = -1) {
        int i {};
        for(const auto &builder : getBuilders()) {
            if(ub != -1 && i++ >= ub) break;
            GDXInterface *pgx = builder();
            cb(*pgx);
            delete pgx;
        }
    }

    TEST_CASE("Simple setup and teardown of a GDX object") {
        basicTest([](GDXInterface &pgx) {
            REQUIRE_EQ(0, pgx.gdxErrorCount());
        });
    }

    TEST_CASE("Just create a file") {
        const std::string fn {"create.gdx"};
        basicTest([&](GDXInterface &pgx) {
            REQUIRE_EQ(0, pgx.gdxErrorCount());
            int ErrNr;
            REQUIRE_FALSE(pgx.gdxOpenWrite(""s, "gdxinterfacetest", ErrNr));
            REQUIRE_NE(0, ErrNr);
            // TODO: Why is this zero?
            REQUIRE_EQ(0, pgx.gdxErrorCount());
            std::string msg;
            REQUIRE(pgx.gdxErrorStr(pgx.gdxGetLastError(), msg));
            REQUIRE_EQ("File name is empty", msg);
            REQUIRE(pgx.gdxOpenWrite(fn, "gdxinterfacetest", ErrNr));
            REQUIRE_FALSE(ErrNr);
            REQUIRE_EQ(0, pgx.gdxErrorCount());
            pgx.gdxClose();
            REQUIRE(std::filesystem::exists(fn));
        });
        std::filesystem::remove(fn);
    }

    TEST_CASE("Test trying to open a file for reading that does not exist") {
        basicTest([&](GDXInterface &pgx) {
            int ErrNr;
            REQUIRE_FALSE(pgx.gdxOpenRead("doesNotExist", ErrNr));
            REQUIRE_EQ(2, ErrNr);
            std::string errMsg;
            REQUIRE(pgx.gdxErrorStr(ErrNr, errMsg));
            REQUIRE_EQ("No such file or directory", errMsg);
            pgx.gdxClose();
        });
    }

    TEST_CASE("Test renaming an UEL") {
        std::string fn{"rename_uel.gdx"};
        basicTest([&](GDXInterface &pgx) {
            int ErrNr;
            REQUIRE(pgx.gdxOpenWrite(fn, "gdxinterfacetest", ErrNr));

            REQUIRE(pgx.gdxUELRegisterStrStart());
            int uelNr;
            REQUIRE(pgx.gdxUELRegisterStr("myuel", uelNr));
            REQUIRE(pgx.gdxUELRegisterDone());

            std::string queriedUel;
            int uelMap;
            REQUIRE(pgx.gdxUMUelGet(1, queriedUel, uelMap));
            REQUIRE_EQ("myuel", queriedUel);

            pgx.gdxRenameUEL("myuel", "newname");
            REQUIRE(pgx.gdxUMUelGet(1, queriedUel, uelMap));
            REQUIRE_EQ("newname", queriedUel);

            pgx.gdxClose();
        });
        std::filesystem::remove(fn);
    }

    TEST_CASE("Test get current active symbol dimension") {
        std::string fn{"sym_dim.gdx"};
        basicTest([&](GDXInterface &pgx) {
            int ErrNr;
            REQUIRE(pgx.gdxOpenWrite(fn, "gdxinterfacetest", ErrNr));
            REQUIRE(pgx.gdxDataWriteRawStart("mysym", "Some explanatory text.", 2, global::gmsspecs::gms_dt_par, 0));
            REQUIRE_EQ(2, pgx.gdxCurrentDim());
            REQUIRE(pgx.gdxDataWriteDone());
            pgx.gdxClose();
        });
        std::filesystem::remove(fn);
    }

    template<typename T>
    void testWriteOp(const std::string &fn,
                     const std::function<void(GDXInterface&)> &cb,
                     bool cleanup = false) {
        if(fn.empty()) return;

        {
            std::string ErrMsg;
            T pgx{ErrMsg};
            int ErrNr;
            REQUIRE(pgx.gdxOpenWrite(fn, "gdxinterfacetest", ErrNr));
            cb(pgx);
            pgx.gdxClose();
        }
        if(cleanup)
            std::filesystem::remove(fn);
    }

    template<typename T>
    void testReadOp(const std::string &fn,
                    const std::function<void(GDXInterface&)> &cb,
                    bool cleanup = false) {
        if(fn.empty()) return;

        {
            std::string ErrMsg;
            T pgx{ErrMsg};
            int ErrNr, rc;
            rc = pgx.gdxOpenRead(fn, ErrNr);
            REQUIRE(rc);
            cb(pgx);
            rc = pgx.gdxClose();
        }
        if(cleanup)
            std::filesystem::remove(fn);
    }

    void testReads(const std::string &filename1,
                   const std::string &filename2,
                   const std::function<void(GDXInterface&)> &cb) {
        testReadOp<xpwrap::GDXFile>(filename1, cb);
        testReadOp<gxfile::TGXFileObj>(filename2, cb);
    }

    void runGdxToYamlScript(const std::string &filename1 = ""s, const std::string &filename2 = ""s) {
#if defined(__APPLE__) || defined(__linux__)
        const std::string pythonInterpreter {"python3"};
#else
        const std::string pythonInterpreter {"python"};
#endif
        std::string f2 = filename2.empty() ? ""s : " "s + filename2;
        system((pythonInterpreter + " gdxtoyaml/main.py "s + filename1 + f2).c_str());
    }

    void testMatchingWrites(const std::string &filename1,
                            const std::string &filename2,
                            const std::function<void(GDXInterface&)> &cb,
                            bool skipDiffing = false) {
        testWriteOp<xpwrap::GDXFile>(filename1, cb);
        testWriteOp<gxfile::TGXFileObj>(filename2, cb);
        if(!filename1.empty() && !filename2.empty() && !skipDiffing) {
            auto maybeMismatches = utils::binaryFileDiff(filename1, filename2);
            if (maybeMismatches) {
                mycout << "Found " << maybeMismatches->size() << " mismatches between " << filename1 << " and " << filename2 << "\n";
                const int truncCnt{ 10 };
                int cnt{};
                for (const auto& mm : *maybeMismatches) {
                    mycout << "#" << mm.offset << ": " << (int)mm.lhs << " != " << (int)mm.rhs << "\n";
                    if (cnt++ >= truncCnt) {
                        mycout << "Truncating results after " << truncCnt << " entries." << "\n";
                        break;
                    }
                }
                mycout << std::endl;
                runGdxToYamlScript(filename1, filename2);
            } else mycout << "No mismatches found between " << filename1 << " and " << filename2 << std::endl;
            REQUIRE_FALSE(maybeMismatches);
        }
    }

    TEST_CASE("Test adding uels (raw mode)") {
        std::array filenames{ "uel_wrapper.gdx"s, "uel_port.gdx"s };
        auto [f1, f2] = filenames;
        testMatchingWrites(f1, f2, [](GDXInterface &pgx) {
            REQUIRE(pgx.gdxUELRegisterRawStart());
            REQUIRE(pgx.gdxUELRegisterRaw(""));
            REQUIRE(pgx.gdxUELRegisterRaw("New-York"));
            REQUIRE(pgx.gdxUELRegisterDone());
        });
        testReads(f1, f2, [](GDXInterface &pgx) {
            int uelCnt, highMap, uelMap;
            std::string uel;
            REQUIRE(pgx.gdxUMUelInfo(uelCnt, highMap));
            REQUIRE_EQ(0, highMap);
            REQUIRE_EQ(2, uelCnt);
            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));
            REQUIRE(uel.empty());
            REQUIRE_EQ(-1, uelMap);
            REQUIRE(pgx.gdxUMUelGet(2, uel, uelMap));
            REQUIRE_EQ("New-York", uel);
            REQUIRE_EQ(-1, uelMap);
            REQUIRE_FALSE(pgx.gdxGetUEL(2, uel));
            REQUIRE_NE("New-York", uel);
        });
        for (const auto& fn : filenames)
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test adding uels (string mode)") {
        std::array filenames{ "uel_str_wrapper.gdx"s, "uel_str_port.gdx"s };
        auto [f1, f2] = filenames;
        testMatchingWrites(f1, f2, [](GDXInterface& pgx) {
            REQUIRE(pgx.gdxUELRegisterStrStart());
            int uelNr;
            REQUIRE(pgx.gdxUELRegisterStr("TheOnlyUEL", uelNr));
            REQUIRE_EQ(1, uelNr);
            REQUIRE(pgx.gdxUELRegisterDone());
        });
        testReads(f1, f2, [](GDXInterface& pgx) {
            int uelCnt, highMap, uelMap;
            std::string uel;
            REQUIRE(pgx.gdxUMUelInfo(uelCnt, highMap));
            REQUIRE_EQ(0, highMap);
            REQUIRE_EQ(1, uelCnt);
            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));
            REQUIRE_EQ("TheOnlyUEL", uel);
            REQUIRE_EQ(-1, uelMap);
            REQUIRE_FALSE(pgx.gdxGetUEL(1, uel));
            REQUIRE_NE("TheOnlyUEL", uel);
        });
        for (const auto& fn : filenames)
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test adding uels (mapped mode)") {
        std::array filenames{ "uel_mapped_wrapper.gdx"s, "uel_mapped_port.gdx"s };
        auto [f1, f2] = filenames;
        testMatchingWrites(f1, f2, [](GDXInterface& pgx) {
            REQUIRE(pgx.gdxUELRegisterMapStart());
            REQUIRE(pgx.gdxUELRegisterMap(3, "TheOnlyUEL"));
            REQUIRE(pgx.gdxUELRegisterDone());
        });
        testReads(f1, f2, [](GDXInterface& pgx) {
            int uelCnt, highMap, uelMap;
            std::string uel;

            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));
            REQUIRE_EQ("TheOnlyUEL", uel);
            REQUIRE_EQ(-1, uelMap);
            REQUIRE(pgx.gdxUMUelInfo(uelCnt, highMap));
            REQUIRE_EQ(0, highMap);
            REQUIRE_EQ(1, uelCnt);

            REQUIRE(pgx.gdxUELRegisterMapStart());
            REQUIRE(pgx.gdxUELRegisterMap(3, "TheOnlyUEL"));
            REQUIRE(pgx.gdxUELRegisterDone());

            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));
            REQUIRE_EQ("TheOnlyUEL", uel);
            REQUIRE_EQ(3, uelMap);

            REQUIRE(pgx.gdxUMUelInfo(uelCnt, highMap));
            REQUIRE_EQ(3, highMap);
            REQUIRE_EQ(1, uelCnt);
            
            REQUIRE(pgx.gdxGetUEL(3, uel));
            REQUIRE_EQ("TheOnlyUEL", uel);
        });
        for (const auto& fn : filenames)
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test write and read record raw") {
        std::string f1{"rwrecordraw_wrapper.gdx"}, f2{"rwrecordraw_port.gdx"};
        int key;
        gxdefs::TgdxValues values{};
        testMatchingWrites(f1, f2, [&](GDXInterface &pgx) {
            REQUIRE(pgx.gdxUELRegisterRawStart());
            REQUIRE(pgx.gdxUELRegisterRaw("TheOnlyUEL"));
            REQUIRE(pgx.gdxUELRegisterDone());
            REQUIRE(pgx.gdxDataWriteRawStart("mysym", "This is my symbol!", 1, global::gmsspecs::gms_dt_par, 0));
            key = 1;
            values[global::gmsspecs::vallevel] = 3.141;
            REQUIRE(pgx.gdxDataWriteRaw(&key, values.data()));
            REQUIRE(pgx.gdxDataWriteDone());

            REQUIRE(pgx.gdxDataWriteRawStart("myscalar", "This is a scalar!", 0, global::gmsspecs::gms_dt_par, 0));
            REQUIRE(pgx.gdxDataWriteDone());
        });
        testReads(f1, f2, [&](GDXInterface &pgx) {
            std::string uel;
            int uelMap;
            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));
            REQUIRE_EQ("TheOnlyUEL", uel);
            REQUIRE_EQ(-1, uelMap);
            int NrRecs;
            REQUIRE(pgx.gdxDataReadRawStart(1, NrRecs));
            REQUIRE_EQ(1, NrRecs);
            int dimFrst;
            REQUIRE(pgx.gdxDataReadRaw(&key, values.data(), dimFrst));
            REQUIRE(pgx.gdxDataReadDone());
            REQUIRE_EQ(1, key);
            REQUIRE_EQ(3.141, values[global::gmsspecs::vallevel]);

            REQUIRE(pgx.gdxDataReadRawStart(2, NrRecs));
            REQUIRE_EQ(1, NrRecs);
            REQUIRE(pgx.gdxDataReadDone());
        });
        for (const auto& fn : {f1, f2})
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test write and read record in string mode") {
        std::string f1{"rwrecordstr_wrapper.gdx"}, f2{"rwrecordstr_port.gdx"};
        StrIndexBuffers keyNames;
        gxdefs::TgdxValues values{};
        testMatchingWrites(f1, f2, [&](GDXInterface &pgx) {
            REQUIRE(pgx.gdxDataWriteStrStart("mysym", "This is my symbol!", 1, global::gmsspecs::gms_dt_par, 0));
            values[global::gmsspecs::vallevel] = 3.141;

            char empty = '\0';
            const char *emptyPtr = &empty;
            REQUIRE(pgx.gdxDataWriteStr(&emptyPtr, values.data()));

            keyNames[0] = "TheOnlyUEL"s;
            const char *keyptrs[] = {keyNames[0].c_str()};
            REQUIRE(pgx.gdxDataWriteStr(keyptrs, values.data()));

            REQUIRE(pgx.gdxDataWriteDone());
        });
        testReads(f1, f2, [&](GDXInterface &pgx) {
            int NrRecs;
            REQUIRE(pgx.gdxDataReadStrStart(1, NrRecs));
            REQUIRE_EQ(2, NrRecs);

            int dimFrst;
            REQUIRE(pgx.gdxDataReadStr(keyNames.ptrs(), values.data(), dimFrst));
            REQUIRE(keyNames[0].str().empty());
            REQUIRE_EQ(3.141, values[global::gmsspecs::vallevel]);

            REQUIRE(pgx.gdxDataReadStr(keyNames.ptrs(), values.data(), dimFrst));
            REQUIRE_EQ("TheOnlyUEL"s, keyNames[0].str());
            REQUIRE_EQ(3.141, values[global::gmsspecs::vallevel]);

            REQUIRE(pgx.gdxDataReadDone());
        });
        for (const auto& fn : {f1, f2})
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test getting special values") {
        gxdefs::TgdxSVals specialValuesFromWrap{}, specialValuesFromPort{};
        std::string ErrMsg;
        {
            xpwrap::GDXFile pgx{ErrMsg};
            REQUIRE(pgx.gdxGetSpecialValues(specialValuesFromWrap));
        }
        {
            gxfile::TGXFileObj pgx{ErrMsg};
            REQUIRE(pgx.gdxGetSpecialValues(specialValuesFromPort));
        }
        for(int i{}; i<specialValuesFromPort.size(); i++) {
            const double eps = 0.001;
            REQUIRE(specialValuesFromWrap[i] - specialValuesFromPort[i] < eps);
        }
    }

    TEST_CASE("Test setting special values") {
        basicTest([&](GDXInterface &pgx) {
            gxdefs::TgdxSVals moddedSpecVals, queriedSpecVals;
            pgx.gdxGetSpecialValues(moddedSpecVals);
            moddedSpecVals[gxfile::TgdxIntlValTyp::vm_valpin] = 0.0;
            pgx.gdxSetSpecialValues(moddedSpecVals);
            pgx.gdxGetSpecialValues(queriedSpecVals);
            REQUIRE_EQ(0.0, queriedSpecVals[gxfile::TgdxIntlValTyp::vm_valpin]);
        });
    }

    void writeMappedRecordsOutOfOrder(GDXInterface &pgx) {
        // very irregular user uel nr -> uel name mapping
        const std::map<int, std::string> userUelMapping {
                {3, "z"},
                {8, "a"},
                {1, "y"},
                {10, "b"}
        };
        // also weird record write ordering
        const std::array<int, 4> randomOrder { 8, 10, 1, 3 };

        REQUIRE(pgx.gdxUELRegisterMapStart());
        for(const auto &pair : userUelMapping) {
            REQUIRE(pgx.gdxUELRegisterMap(pair.first, pair.second));
        }
        REQUIRE(pgx.gdxUELRegisterDone());

        REQUIRE(pgx.gdxDataWriteMapStart("irregularSym", "So out of order!", 1, global::gmsspecs::gms_dt_par, 0));
        int key;
        gxdefs::TgdxValues values{};
        for(const auto ix : randomOrder) {
            key = ix;
            values[global::gmsspecs::vallevel] = 3.141 * ix;
            REQUIRE(pgx.gdxDataWriteMap(&key, values.data()));
        }
        REQUIRE(pgx.gdxDataWriteDone());
        REQUIRE_EQ(0, pgx.gdxErrorCount());
        REQUIRE_EQ(0, pgx.gdxDataErrorCount());
    }

    TEST_CASE("Test writing mapped records out of order") {
        std::string f1{ "mapped_outoforder_wrapper.gdx" }, f2 {"mapped_outoforder_port.gdx"};
        testMatchingWrites(f1, f2, writeMappedRecordsOutOfOrder);
        for (const auto& fn : { f1, f2 })
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test write and read record mapped - out of order") {
        std::string f1{ "rwrecordmapped_ooo_wrapper.gdx" }, f2 {"rwrecordmapped_ooo_port.gdx"};
        int key;
        gxdefs::TgdxValues values{};
        testMatchingWrites(f1, f2, [&](GDXInterface &pgx) {
            int uelCnt, highMap;
            REQUIRE(pgx.gdxUMUelInfo(uelCnt, highMap));
            REQUIRE_EQ(0, uelCnt);
            REQUIRE_EQ(0, highMap);
            REQUIRE(pgx.gdxUELRegisterMapStart());
            REQUIRE(pgx.gdxUELRegisterMap(3, "First"));
            REQUIRE(pgx.gdxUELRegisterMap(4, "Second"));
            REQUIRE(pgx.gdxUELRegisterMap(5, "Third"));
            REQUIRE(pgx.gdxUELRegisterMap(2, "Fourth"));
            REQUIRE(pgx.gdxUELRegisterDone());
            std::string uel;
            REQUIRE(pgx.gdxGetUEL(3, uel));
            REQUIRE_EQ("First", uel);
            REQUIRE(pgx.gdxDataWriteMapStart("mysym", "Single record", 1, global::gmsspecs::gms_dt_par, 0));
            key = 3;
            values[global::gmsspecs::vallevel] = 3.141;
            REQUIRE(pgx.gdxDataWriteMap(&key, values.data()));
            REQUIRE(pgx.gdxDataWriteDone());

            // User UEL indices non-increasing
            REQUIRE(pgx.gdxDataWriteMapStart("mysym2", "Four records", 1, global::gmsspecs::gms_dt_par, 0));
            std::array<int, 4> expKey { 3, 4, 5, 2 };
            for(int i : expKey)
                REQUIRE(pgx.gdxDataWriteMap(&i, values.data()));
            REQUIRE(pgx.gdxDataWriteDone());

            REQUIRE_EQ(0, pgx.gdxErrorCount());
            REQUIRE_EQ(0, pgx.gdxDataErrorCount());
        });
        testReads(f1, f2, [&](GDXInterface& pgx) {
            std::string uel;
            int uelMap;
            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));
            REQUIRE_EQ("First", uel);
            REQUIRE_EQ(-1, uelMap);

            REQUIRE(pgx.gdxUELRegisterMapStart());
            REQUIRE(pgx.gdxUELRegisterMap(3, "First"));
            REQUIRE(pgx.gdxUELRegisterMap(4, "Second"));
            REQUIRE(pgx.gdxUELRegisterMap(5, "Third"));
            REQUIRE(pgx.gdxUELRegisterMap(2, "Fourth"));
            REQUIRE(pgx.gdxUELRegisterDone());

            REQUIRE(pgx.gdxGetUEL(3, uel));
            REQUIRE_EQ("First", uel);

            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));            
            REQUIRE_EQ("First", uel);
            REQUIRE_EQ(3, uelMap);

            int NrRecs;
            REQUIRE(pgx.gdxDataReadMapStart(1, NrRecs));
            REQUIRE_EQ(1, NrRecs);
            int dimFrst;
            REQUIRE(pgx.gdxDataReadMap(1, &key, values.data(), dimFrst));
            REQUIRE_EQ(0, pgx.gdxDataErrorCount());
            REQUIRE_EQ(3, key);
            REQUIRE_EQ(3.141, values[global::gmsspecs::vallevel]);
            REQUIRE_EQ(1, dimFrst);
            REQUIRE(pgx.gdxDataReadDone());

            REQUIRE(pgx.gdxDataReadMapStart(2, NrRecs));
            REQUIRE_EQ(4, NrRecs);
            // unordered mapped write comes out ordered
            std::array<int, 4> expKey { 2, 3, 4, 5 };
            for(int i{1}; i<=expKey.size(); i++) {
                REQUIRE(pgx.gdxDataReadMap(i, &key, values.data(), dimFrst));
                REQUIRE_EQ(expKey[i-1], key);
            }
            REQUIRE_EQ(0, pgx.gdxDataErrorCount());
            REQUIRE(pgx.gdxDataReadDone());
        });
        for (const auto& fn : { f1, f2 })
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test write and read record mapped - in order") {
        std::string f1{ "rwrecordmapped_io_wrapper.gdx" }, f2 {"rwrecordmapped_io_port.gdx"};
        int key;
        gxdefs::TgdxValues values{};
        testMatchingWrites(f1, f2, [&](GDXInterface &pgx) {
            int uelCnt, highMap;
            REQUIRE(pgx.gdxUMUelInfo(uelCnt, highMap));
            REQUIRE_EQ(0, uelCnt);
            REQUIRE_EQ(0, highMap);
            REQUIRE(pgx.gdxUELRegisterMapStart());
            REQUIRE(pgx.gdxUELRegisterMap(5, "First"));
            REQUIRE(pgx.gdxUELRegisterMap(6, "Second"));
            REQUIRE(pgx.gdxUELRegisterMap(7, "Third"));
            REQUIRE(pgx.gdxUELRegisterMap(8, "Fourth"));
            REQUIRE(pgx.gdxUELRegisterDone());
            std::string uel;
            REQUIRE(pgx.gdxGetUEL(5, uel));
            REQUIRE_EQ("First", uel);

            // User UEL indices increasing
            REQUIRE(pgx.gdxDataWriteMapStart("mysym", "Four records", 1, global::gmsspecs::gms_dt_par, 0));
            std::array<int, 4> expKey { 5, 6, 7, 8 };
            for(int i : expKey)
                REQUIRE(pgx.gdxDataWriteMap(&i, values.data()));
            REQUIRE(pgx.gdxDataWriteDone());

            REQUIRE_EQ(0, pgx.gdxErrorCount());
            REQUIRE_EQ(0, pgx.gdxDataErrorCount());
        });
        testReads(f1, f2, [&](GDXInterface& pgx) {
            std::string uel;
            int uelMap;
            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));
            REQUIRE_EQ("First", uel);
            REQUIRE_EQ(-1, uelMap);

            REQUIRE(pgx.gdxUELRegisterMapStart());
            REQUIRE(pgx.gdxUELRegisterMap(5, "First"));
            REQUIRE(pgx.gdxUELRegisterMap(6, "Second"));
            REQUIRE(pgx.gdxUELRegisterMap(7, "Third"));
            REQUIRE(pgx.gdxUELRegisterMap(8, "Fourth"));
            REQUIRE(pgx.gdxUELRegisterDone());

            REQUIRE(pgx.gdxGetUEL(5, uel));
            REQUIRE_EQ("First", uel);

            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));
            REQUIRE_EQ("First", uel);
            REQUIRE_EQ(5, uelMap);

            int NrRecs;
            REQUIRE(pgx.gdxDataReadMapStart(1, NrRecs));
            REQUIRE_EQ(4, NrRecs);
            // should still be ordered
            std::array<int, 4> expKey { 5, 6, 7, 8 };
            for(int i{1}; i<=expKey.size(); i++) {
                int dimFrst;
                REQUIRE(pgx.gdxDataReadMap(i, &key, values.data(), dimFrst));
                REQUIRE_EQ(expKey[i-1], key);
            }
            REQUIRE_EQ(0, pgx.gdxDataErrorCount());
            REQUIRE(pgx.gdxDataReadDone());
        });
        for (const auto& fn : { f1, f2 })
            std::filesystem::remove(fn);
    }

    void domainSetGetTestSetupPrefix(GDXInterface &pgx) {
        int numSyms, numUels;
        REQUIRE(pgx.gdxSystemInfo(numSyms, numUels));
        REQUIRE_EQ(0, numUels);
        REQUIRE_EQ(0, numSyms);

        REQUIRE(pgx.gdxDataWriteStrStart("i", "", 1, global::gmsspecs::dt_set, 0));
        REQUIRE(pgx.gdxDataWriteDone());
        REQUIRE(pgx.gdxSystemInfo(numSyms, numUels));
        REQUIRE_EQ(1, numSyms);

        REQUIRE(pgx.gdxDataWriteStrStart("j", "", 1, global::gmsspecs::dt_set, 0));
        REQUIRE(pgx.gdxDataWriteDone());

        REQUIRE(pgx.gdxAddAlias("k", "i"));

        REQUIRE(pgx.gdxSystemInfo(numSyms, numUels));
        REQUIRE_EQ(3, numSyms);

        REQUIRE(pgx.gdxDataWriteStrStart("newd", "Same domain as d", 2, global::gmsspecs::dt_par, 0));
    }

    void commonSetGetDomainTests(const std::vector<std::string> &domainNames,
                                 const std::vector<int> &domainIndices) {

        assert(domainNames.size() == 2);
        assert(domainIndices.size() == 2);

        const std::array<std::string, 4> fns {
                "setgetdomainx_wrap.gdx"s,
                "setgetdomainx_port.gdx"s,
                "setgetdomain_wrap.gdx"s,
                "setgetdomain_port.gdx"s,
        };

        gxdefs::TgdxStrIndex newSymDomainNames {};
        std::array<int, 2> newSymDomainIndices {};
        for(int i{}; i<domainNames.size(); i++) {
            newSymDomainNames[i] = domainNames[i];
            newSymDomainIndices[i] = domainIndices[i];
        }

        auto domainSetGetTestSetupSuffix = [](GDXInterface &pgx) {
            int numSyms, numUels;
            REQUIRE(pgx.gdxDataWriteDone());
            REQUIRE(pgx.gdxSystemInfo(numSyms, numUels));
            REQUIRE_EQ(4, numSyms);

            int newSymNr;
            REQUIRE(pgx.gdxFindSymbol("newd", newSymNr));
            REQUIRE_EQ(4, newSymNr);
        };

        auto testSetGetDomainX = [&](GDXInterface &pgx) {
            StrIndexBuffers sib {&newSymDomainNames};
            REQUIRE(pgx.gdxSymbolSetDomainX(4, sib.cptrs()));
            StrIndexBuffers domainIds {};
            REQUIRE(pgx.gdxSymbolGetDomainX(4, domainIds.ptrs()));
            REQUIRE_EQ(newSymDomainNames.front(), domainIds[0].str());
            REQUIRE_EQ(newSymDomainNames[1], domainIds[1].str());
        };

        auto testSetGetDomain = [&](GDXInterface &pgx) {
            domainSetGetTestSetupPrefix(pgx);
            StrIndexBuffers sib {&newSymDomainNames};
            REQUIRE(pgx.gdxSymbolSetDomain(sib.cptrs()));
            domainSetGetTestSetupSuffix(pgx);
            std::array<int, 2> domainSyNrs {};
            REQUIRE(pgx.gdxSymbolGetDomain(4, domainSyNrs.data()));
            REQUIRE_EQ(newSymDomainIndices[0], domainSyNrs[0]);
            REQUIRE_EQ(newSymDomainIndices[1], domainSyNrs[1]);
        };

        testMatchingWrites(fns[0], fns[1], [&](GDXInterface &pgx) {
            domainSetGetTestSetupPrefix(pgx);
            domainSetGetTestSetupSuffix(pgx);
            testSetGetDomainX(pgx);
        });

        testReads(fns[0], fns[1], [&](GDXInterface &pgx) {
            StrIndexBuffers domainIds {};
            REQUIRE(pgx.gdxSymbolGetDomainX(4, domainIds.ptrs()));
            REQUIRE_EQ(newSymDomainNames[0], domainIds[0].str());
            REQUIRE_EQ(newSymDomainNames[1], domainIds[1].str());
        });

        testMatchingWrites(fns[2], fns[3], [&](GDXInterface &pgx) {
            testSetGetDomain(pgx);
        });

        for (const auto &fn: fns)
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test writing with set/get domain normal and variant") {
        commonSetGetDomainTests({"i", "j"}, {1, 2});
        commonSetGetDomainTests({"i", "i"}, {1, 1});
        commonSetGetDomainTests({"i", "k"}, {1, 3});
    }

    TEST_CASE("Test adding a set alias") {
        const std::string f1 {"addalias_wrap.gdx"}, f2 {"addalias_port.gdx"};
        testMatchingWrites(f1, f2, [&](GDXInterface &pgx) {
            REQUIRE(pgx.gdxDataWriteStrStart("i", "A set", 1, global::gmsspecs::dt_set, 0));
            REQUIRE(pgx.gdxDataWriteDone());
            int numSymbols, numUels, symbolCountBefore;
            REQUIRE(pgx.gdxSystemInfo(symbolCountBefore, numUels));
            REQUIRE_EQ(1, symbolCountBefore);
            REQUIRE(pgx.gdxAddAlias("i", "aliasI"));
            REQUIRE(pgx.gdxSystemInfo(numSymbols, numUels));
            REQUIRE_EQ(symbolCountBefore + 1, numSymbols);
            int aliasIx;
            REQUIRE(pgx.gdxFindSymbol("aliasI", aliasIx));
            REQUIRE_EQ(2, aliasIx);
            int numRecords, userInfo, dim, typ;
            std::string explText, symbolName;
            REQUIRE(pgx.gdxSymbolInfoX(2, numRecords, userInfo, explText));
            REQUIRE(pgx.gdxSymbolInfo(2, symbolName, dim, typ));
            REQUIRE_EQ(0, numRecords);
            REQUIRE_EQ(1, userInfo); // symbol index of "i"
            REQUIRE_EQ("Aliased with i", explText);
            REQUIRE_EQ("aliasI", symbolName);
            REQUIRE_EQ(1, dim);
            REQUIRE_EQ(global::gmsspecs::dt_alias, typ);
        });
        for (const auto& fn : { f1, f2 })
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test creating and querying element text for sets") {
        const std::string f1{ "elemtxt_wrap.gdx" }, f2{ "elemtxt_port.gdx" };
        testMatchingWrites(f1, f2, [&](GDXInterface& pgx) {
            REQUIRE(pgx.gdxUELRegisterRawStart());
            REQUIRE(pgx.gdxUELRegisterRaw("onlyuel"));
            REQUIRE(pgx.gdxUELRegisterDone());
            REQUIRE(pgx.gdxDataWriteRawStart("i", "expl", 1, global::gmsspecs::gms_dt_set, 0));
            gxdefs::TgdxUELIndex keys;
            gxdefs::TgdxValues values;
            keys[0] = 1;
            values[global::gmsspecs::tvarvaltype::vallevel] = 1;
            REQUIRE(pgx.gdxDataWriteRaw(keys.data(), values.data()));
            REQUIRE(pgx.gdxDataWriteDone());
            int txtNr;
            REQUIRE(pgx.gdxAddSetText("set text", txtNr));
            REQUIRE_EQ(1, txtNr);
            int elemNode;
            std::string elemTxt;
            REQUIRE(pgx.gdxGetElemText(txtNr, elemTxt, elemNode));
            REQUIRE_EQ("set text", elemTxt);
            REQUIRE_EQ(0, elemNode);
        });
        for (const auto& fn : { f1, f2 })
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test invalid raw writing error processing") {
        const std::string fn {"tmpfile.gdx"s};
        int key;
        gxdefs::TgdxValues values{};
        basicTest([&](GDXInterface& pgx) {
            if(std::filesystem::exists(fn))
                std::filesystem::remove(fn);
            int errNr;
            REQUIRE(pgx.gdxOpenWrite(fn, "gdxinterfacetests", errNr));
            REQUIRE(pgx.gdxUELRegisterRawStart());
            REQUIRE(pgx.gdxUELRegisterRaw("onlyuel1"));
            REQUIRE(pgx.gdxUELRegisterRaw("onlyuel2"));
            REQUIRE(pgx.gdxUELRegisterRaw("onlyuel3"));
            REQUIRE(pgx.gdxUELRegisterDone());
            REQUIRE(pgx.gdxDataWriteRawStart("i", "expl", 1, global::gmsspecs::gms_dt_set, 0));
            key = 3;
            REQUIRE(pgx.gdxDataWriteRaw(&key, values.data()));
            key = 1;
            REQUIRE_FALSE(pgx.gdxDataWriteRaw(&key, values.data()));
            REQUIRE(pgx.gdxDataWriteDone());
            REQUIRE_EQ(1, pgx.gdxErrorCount());
            REQUIRE_EQ(1, pgx.gdxDataErrorCount());
            REQUIRE(pgx.gdxDataErrorRecord(1, &key, values.data()));
            REQUIRE_EQ(1, key);
            REQUIRE(pgx.gdxDataErrorRecordX(1, &key, values.data()));
            REQUIRE_EQ(1, key);
            int ec = pgx.gdxGetLastError();
            std::string errMsg;
            REQUIRE(pgx.gdxErrorStr(ec, errMsg));
            REQUIRE_EQ("Data not sorted when writing raw", errMsg);
            pgx.gdxClose();
        });
        std::filesystem::remove(fn);
    }

    std::string acquireGDXforModel(const std::string &model) {
        const std::string   model_fn = model + ".gms"s,
                            log_fn = model + "Log.txt"s,
                            fnpf = "model_data"s,
                            gdxfn = fnpf+".gdx"s;
        std::system(("gamslib "s + model + " > gamslibLog.txt"s).c_str());
        std::filesystem::remove("gamslibLog.txt");
        REQUIRE(std::filesystem::exists(model_fn));
        std::system(("gams " + model_fn + " gdx="s + fnpf + " lo=0 o=lf > " + log_fn).c_str());
        std::filesystem::remove(log_fn);
        std::filesystem::remove(model_fn);
        std::filesystem::remove("lf");
        REQUIRE(std::filesystem::exists(gdxfn));
        return gdxfn;
    }

    TEST_CASE("Test reading/extracting data from gamslib/trnsport example") {
        const std::array expectedSymbolNames {
            "a"s, "b"s, "c"s,
            "cost"s, "d"s, "demand"s,
            "f"s, "i"s, "j"s, "supply"s,
            "x"s, "z"s
        };
        const std::array expectedUels {
            "seattle"s, "san-diego"s, "new-york"s, "chicago"s, "topeka"s
        };
        const std::string gdxfn = acquireGDXforModel("trnsport"s);
        testReads(gdxfn, gdxfn, [&](GDXInterface &pgx) {
            int numSymbols, numUels;
            pgx.gdxSystemInfo(numSymbols, numUels);
            std::list<std::string> uelsSeen {}, symbolsSeen {};
            for(int i{1}; i<=numUels; i++) {
                std::string uel;
                int uelMap;
                REQUIRE(pgx.gdxUMUelGet(i, uel, uelMap));
                REQUIRE(!uel.empty());
                uelsSeen.push_back(uel);
            }
            REQUIRE_EQ(expectedUels.size(), uelsSeen.size());
            for(const auto &uel : expectedUels)
                REQUIRE(std::find(uelsSeen.begin(), uelsSeen.end(), uel) != uelsSeen.end());

            for(int i{1}; i<=numSymbols; i++) {
                std::string name;
                int dim, typ;
                REQUIRE(pgx.gdxSymbolInfo(i, name, dim, typ));
                REQUIRE(dim >= 0);
                REQUIRE(typ >= 0);
                REQUIRE(!name.empty());
                symbolsSeen.push_back(name);

                std::string explanatoryText;
                int userInfo, numRecords;
                REQUIRE(pgx.gdxSymbolInfoX(i, numRecords, userInfo, explanatoryText));
                if(numRecords) {
                    int numRecords2;
                    REQUIRE(pgx.gdxDataReadStrStart(i, numRecords2));
                    REQUIRE_EQ(numRecords, numRecords2);
                    StrIndexBuffers keyNames;
                    gxdefs::TgdxValues values;
                    int dimFirst;
                    for(int j{}; j<numRecords; j++) {
                        REQUIRE(pgx.gdxDataReadStr(keyNames.ptrs(), values.data(), dimFirst));
                        REQUIRE(dimFirst >= 0);
                        for(int d{}; d<dim; d++)
                            REQUIRE(!keyNames[d].empty());
                    }
                    REQUIRE(pgx.gdxDataReadDone());
                }
            }
            REQUIRE_EQ(expectedSymbolNames.size(), symbolsSeen.size());
            for(const auto &expName : expectedSymbolNames)
                REQUIRE(std::find(symbolsSeen.begin(), symbolsSeen.end(), expName) != symbolsSeen.end());

            int symNr;
            REQUIRE(pgx.gdxFindSymbol("i", symNr));
            REQUIRE_EQ(1, symNr);
            REQUIRE_EQ(1, pgx.gdxSymbolDim(1));

            REQUIRE(pgx.gdxFindSymbol("*", symNr));
            REQUIRE_EQ(0, symNr);

            // d(i,j) is symbol 5
            REQUIRE(pgx.gdxFindSymbol("d", symNr));
            REQUIRE_EQ(5, symNr);
            std::array<int, 2> domainSyNrs {};
            REQUIRE(pgx.gdxSymbolGetDomain(5, domainSyNrs.data()));
            REQUIRE_EQ(2, pgx.gdxSymbolDim(5));
            // first domain of d(i,j) is i (symbol 1)
            REQUIRE_EQ(1, domainSyNrs[0]);
            // second domain of d(i,j) is j (symbol 2)
            REQUIRE_EQ(2, domainSyNrs[1]);

            StrIndexBuffers domainIds;
            REQUIRE(pgx.gdxSymbolGetDomainX(5, domainIds.ptrs()));
            REQUIRE_EQ("i"s, domainIds[0].str());
            REQUIRE_EQ("j"s, domainIds[1].str());
        });
        std::filesystem::remove(gdxfn);
    }

    TEST_CASE("Tests related to universe") {
        basicTest([](GDXInterface &pgx) {
            int errNr;
            auto fn = "universe_tests.gdx"s;
            REQUIRE(pgx.gdxOpenWrite(fn, "gdxinterfacetests", errNr));
            REQUIRE(pgx.gdxDataWriteStrStart("i", "A set", 1, global::gmsspecs::dt_set, 0));
            StrIndexBuffers keys{};
            gxdefs::TgdxValues vals{};
            for(int i=1; i<=8; i++) {
                keys[0] = "uel_" + std::to_string(i);
                const char *keyptrs[] = {keys[0].c_str()};
                REQUIRE(pgx.gdxDataWriteStr(keyptrs, vals.data()));
            }
            REQUIRE(pgx.gdxDataWriteDone());

            int symNr;
            REQUIRE(pgx.gdxFindSymbol("*", symNr));
            REQUIRE_EQ(0, symNr);

            std::string symName;
            int dim, typ;
            REQUIRE(pgx.gdxSymbolInfo(0, symName, dim, typ));
            REQUIRE_EQ("*", symName);
            REQUIRE_EQ(1, dim);
            REQUIRE_EQ(global::gmsspecs::dt_set, typ);

            int recCnt, userInfo;
            std::string explText;
            REQUIRE(pgx.gdxSymbolInfoX(0, recCnt, userInfo, explText));
            REQUIRE_EQ(0, recCnt);
            REQUIRE_EQ(0, userInfo);
            REQUIRE_EQ("Universe", explText);

            pgx.gdxClose();

            REQUIRE(pgx.gdxOpenRead(fn, errNr));

            REQUIRE(pgx.gdxSymbolInfoX(0, recCnt, userInfo, explText));
            REQUIRE(pgx.gdxFindSymbol("*", symNr));
            REQUIRE_EQ(8, recCnt);

            REQUIRE(pgx.gdxDataReadStrStart(0, recCnt));
            int dimFirst;
            REQUIRE_EQ(8, recCnt);
            for(int i=1; i<=recCnt; i++) {
                REQUIRE(pgx.gdxDataReadStr(keys.ptrs(), vals.data(), dimFirst));
                REQUIRE_EQ("uel_"+std::to_string(i), keys.front().str());
            }
            REQUIRE(pgx.gdxDataReadDone());

            pgx.gdxClose();

            std::filesystem::remove(fn);
        });
    }

    TEST_CASE("Test domain checking for subset") {
        const std::string fn {"xyz.gdx"};
        basicTest([&](GDXInterface &pgx) {
            int errnr;
            REQUIRE(pgx.gdxOpenWrite(fn, "gdxinterfacetests", errnr));
            REQUIRE(pgx.gdxDataWriteStrStart("i", "expl", 1, global::gmsspecs::gms_dt_set, 0));
            std::string key;
            gxdefs::TgdxValues vals{};
            for(int i{}; i<6; i++) {
                key = "i"s+std::to_string(i+1);
                const char *keyptrs[] = {key.c_str()};
                REQUIRE(pgx.gdxDataWriteStr(keyptrs, vals.data()));
            }
            REQUIRE(pgx.gdxDataWriteDone());

            REQUIRE(pgx.gdxDataWriteStrStart("j", "subset of i", 1, global::gmsspecs::gms_dt_set, 0));
            key = "i"s;
            const char *keyptrs[] = {key.c_str()};
            pgx.gdxSymbolSetDomain(keyptrs);
            //REQUIRE(pgx.gdxSymbolSetDomain(keys));
            std::array<int, 2> subset = {2, 4};
            for(int i : subset) {
                key = "i"s+std::to_string(i);
                const char *keyptrs[] = { key.c_str() };
                REQUIRE(pgx.gdxDataWriteStr(keyptrs, vals.data()));
            }

            // adding an uel not from superset should fail
            key = "not_in_i"s;
            keyptrs[0] = key.c_str();
            REQUIRE(pgx.gdxDataWriteStr(keyptrs, vals.data()));

            REQUIRE(pgx.gdxDataWriteDone());

            REQUIRE_EQ(1, pgx.gdxErrorCount());
            std::string msg;
            REQUIRE(pgx.gdxErrorStr(pgx.gdxGetLastError(), msg));
            REQUIRE_EQ("Domain violation"s, msg);

            // check if error uels was correctly set (more specific domain violation details)
            REQUIRE_EQ(1, pgx.gdxDataErrorCount());
            std::array<int, 20> errRecKeys {};
            std::array<double, 5> errRecVals {};
            REQUIRE(pgx.gdxDataErrorRecord(1, errRecKeys.data(), errRecVals.data()));
            std::string uelNotInSuperset;
            int uelMap;
            REQUIRE(pgx.gdxUMUelGet(errRecKeys.front(), uelNotInSuperset, uelMap));
            REQUIRE_EQ("not_in_i"s, uelNotInSuperset);
            pgx.gdxClose();
        });
        std::filesystem::remove(fn);
    }

    TEST_CASE("Test writing a duplicate uel in string mode") {
        basicTest([](GDXInterface &pgx) {
            int errNr;
            auto fn = "dup.gdx"s;
            REQUIRE(pgx.gdxOpenWrite(fn, "gdxinterfacetests", errNr));
            REQUIRE(pgx.gdxDataWriteStrStart("i", "A set", 1, global::gmsspecs::dt_set, 0));
            gxdefs::TgdxStrIndex keys{};
            gxdefs::TgdxValues vals{};
            for(int i=1; i<=8; i++) {
                keys[0] = "uel_" + std::to_string(i);
                const char *keyptrs[] = { keys[0].c_str() };
                REQUIRE(pgx.gdxDataWriteStr(keyptrs, vals.data()));
            }
            const char *keyptrs[] = { keys[0].c_str() };
            REQUIRE(pgx.gdxDataWriteStr(keyptrs, vals.data()));
            REQUIRE(pgx.gdxDataWriteDone());
            REQUIRE_EQ(1, pgx.gdxErrorCount());
            std::string msg;
            pgx.gdxErrorStr(pgx.gdxGetLastError(), msg);
            REQUIRE_EQ("Duplicate keys", msg);
            pgx.gdxClose();
            std::filesystem::remove(fn);
        });
    }

    TEST_CASE("Test acronym facilities") {
        std::array filenames{ "acronyms_wrapper.gdx"s, "acronyms_port.gdx"s };
        auto [f1, f2] = filenames;
        testMatchingWrites(f1, f2, [](GDXInterface &pgx) {
            REQUIRE_EQ(0, pgx.gdxAcronymCount());
            REQUIRE(pgx.gdxAcronymAdd("myacr"s, "my acronym"s, 23));
            std::string acroName, acroText;
            int acroIndex;
            REQUIRE(pgx.gdxAcronymGetInfo(1, acroName, acroText, acroIndex));
            REQUIRE_EQ("myacr"s, acroName);
            REQUIRE_EQ("my acronym"s, acroText);
            REQUIRE_EQ(23, acroIndex);
        });
        for (const auto& fn : { f1, f2 })
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test comments addition and querying") {
        std::array filenames{ "comments_wrapper.gdx"s, "comments_port.gdx"s };
        auto [f1, f2] = filenames;
        testMatchingWrites(f1, f2, [](GDXInterface &pgx) {
            REQUIRE(pgx.gdxDataWriteStrStart("i", "expl text", 1, global::gmsspecs::gms_dt_set, 0));
            REQUIRE(pgx.gdxDataWriteDone());
            const auto commentStrExp {"A fancy comment!"s};
            REQUIRE(pgx.gdxSymbolAddComment(1, commentStrExp));
            std::string commentStrGot;
            REQUIRE(pgx.gdxSymbolGetComment(1, 1, commentStrGot));
            REQUIRE_EQ(commentStrExp, commentStrGot);
        });
        for (const auto& fn : { f1, f2 })
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test writing and reading set element texts more exhaustively") {
        std::string f1{ "setelemtxt_wr_wrapper.gdx" },
                    f2 {"setelemtxt_wr_port.gdx"};
        // no set text
        testMatchingWrites(f1, f2, [&](GDXInterface &pgx) {
            REQUIRE(pgx.gdxUELRegisterRawStart());
            REQUIRE(pgx.gdxUELRegisterRaw("seattle"s));
            REQUIRE(pgx.gdxUELRegisterDone());

            pgx.gdxDataWriteRawStart("i", "cities", 1, global::gmsspecs::dt_set, 0);
            gxdefs::TgdxValues vals {};
            vals[global::gmsspecs::tvarvaltype::vallevel] = 3.141;
            int key {1};
            pgx.gdxDataWriteRaw(&key, vals.data());
            pgx.gdxDataWriteDone();
        });
        testReads(f1, f2, [&](GDXInterface& pgx) {
            std::string txt;
            int node;
            REQUIRE(pgx.gdxGetElemText(0, txt, node));
            int hi {std::numeric_limits<int>::max()};
            REQUIRE_FALSE(pgx.gdxGetElemText(hi, txt, node));
            REQUIRE_FALSE(pgx.gdxGetElemText(1, txt, node));
        });
        for (const auto& fn : { f1, f2 })
            std::filesystem::remove(fn);
        /*testReads("concat.gdx"s, "concat.gdx"s, [&](GDXInterface& pgx) {
            std::string txt;
            int node;
            REQUIRE_FALSE(pgx.gdxGetElemText(1, txt, node));
            StrIndexBuffers domainIds;
            std::string syid;
            int dim, typ;
            int sycnt, uelcnt;
            pgx.gdxSystemInfo(sycnt, uelcnt);
            int rc = pgx.gdxSymbolInfo(2, syid, dim, typ);
            rc = pgx.gdxSymbolGetDomainX(2, domainIds.ptrs());
            for(int i=0; i<pgx.gdxSymbolDim(2); i++)
                REQUIRE(!domainIds[i].empty());
        });*/
    }

    TEST_CASE("Debug issue with SymbolSetDomain and write raw domain check uncovered by emp_oa_gams_jams test") {
        std::string f1{ "domaincheck_wrapper.gdx" },
                    f2 {"domaincheck_port.gdx"};
        testMatchingWrites(f1, f2, [&](GDXInterface &pgx) {
            pgx.gdxStoreDomainSetsSet(false);
            pgx.gdxUELRegisterRawStart();
            for(int i=0; i<50; i++)
                pgx.gdxUELRegisterRaw(("uel_"s+std::to_string(i+1)));
            pgx.gdxUELRegisterDone();

            pgx.gdxDataWriteRawStart("j", "", 1, global::gmsspecs::dt_set, 0);
            gxdefs::TgdxValues vals {};
            vals[global::gmsspecs::tvarvaltype::vallevel] = 3.141;
            int key;
            for(key=17; key <= 32; key++)
                pgx.gdxDataWriteRaw(&key, vals.data());
            REQUIRE(pgx.gdxDataWriteDone());
            REQUIRE(pgx.gdxDataWriteRawStart("jb", "", 1, global::gmsspecs::dt_set, 0));
            gxdefs::TgdxStrIndex domainNames {};
            domainNames.front() = "j";
            StrIndexBuffers domainIds {&domainNames};
            REQUIRE(pgx.gdxSymbolSetDomain(domainIds.cptrs()));
            key = 17;
            REQUIRE(pgx.gdxDataWriteRaw(&key, vals.data()));
            REQUIRE(pgx.gdxDataWriteDone());
            if(pgx.gdxErrorCount() > 0) {
                int errNr = pgx.gdxGetLastError();
                std::string errMsg;
                pgx.gdxErrorStr(errNr, errMsg);
                std::cout << errMsg << std::endl;
            }
        });
        for (const auto& fn : { f1, f2 })
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test writing and reading some special value scalars") {
        std::string f1{ "sv_scalars_wrapper.gdx" },
                f2 {"sv_scalars_port.gdx"};
        testMatchingWrites(f1, f2, [&](GDXInterface &pgx) {
            gxdefs::TgdxValues vals {};
            vals[global::gmsspecs::tvarvaltype::vallevel] = 1e+300;
            REQUIRE(pgx.gdxDataWriteRawStart("undef", "", 0, global::gmsspecs::dt_par, 0));
            REQUIRE(pgx.gdxDataWriteRaw(nullptr, vals.data()));
            REQUIRE(pgx.gdxDataWriteDone());
        });
        testReads(f1, f2, [&](GDXInterface& pgx) {
            //std::string txt;
            //int node;
            //REQUIRE(pgx.gdxGetElemText(1, txt, node));
            int recCnt;
            REQUIRE(pgx.gdxDataReadRawStart(1, recCnt));
            int dimFirst;
            gxdefs::TgdxValues vals {};
            pgx.gdxDataReadRaw(nullptr, vals.data(), dimFirst);
            REQUIRE(pgx.gdxDataReadDone());
            double undef = vals[global::gmsspecs::tvarvaltype::vallevel];
            REQUIRE(1e+300 - undef < 0.1);
        });
        for (const auto& fn : { f1, f2 })
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test open append to rename a single uel") {
        const std::array infixes {"wrap"s, "port"s};
        auto getfn = [&](int i) {
            return "append_"s + infixes[i] + "_rename.gdx"s;
        };
        const std::string prod {"gdxinterfacetest"s};
        int errNr, cnt {};
        basicTest([&](GDXInterface &pgx) {
            pgx.gdxOpenWrite(getfn(cnt), prod, errNr);
            pgx.gdxUELRegisterRawStart();
            pgx.gdxUELRegisterRaw("a"s);
            pgx.gdxUELRegisterDone();
            pgx.gdxClose();
            pgx.gdxOpenAppend(getfn(cnt), prod, errNr);
            pgx.gdxRenameUEL("a"s, "b");
            pgx.gdxClose();
            cnt++;
        });
        cnt = 0;
        basicTest([&](GDXInterface &pgx) {
            pgx.gdxOpenRead(getfn(cnt), errNr);
            int uelMap;
            std::string uelStr;
            pgx.gdxUMUelGet(1, uelStr, uelMap);
            REQUIRE_EQ("b"s, uelStr);
            pgx.gdxClose();
            std::filesystem::remove(getfn(cnt));
            cnt++;
        });
    }

    void testWithCompressConvert(bool compress = false, const std::string &convert = ""s) {
        const std::string f1 {"conv_compr_wrap.gdx"}, f2 {"conv_compr_port.gdx"};
        rtl::p3utils::P3SetEnv("GDXCOMPRESS", compress ? "1"s : "0"s);
        rtl::p3utils::P3SetEnv("GDXCONVERT", convert);
        testMatchingWrites(f1, f2, [&](GDXInterface &pgx) {
            gxdefs::TgdxValues vals {};
            REQUIRE(pgx.gdxDataWriteRawStart("undef", "", 0, global::gmsspecs::dt_par, 0));
            REQUIRE(pgx.gdxDataWriteRaw(nullptr, vals.data()));
            REQUIRE(pgx.gdxDataWriteDone());
        });
        for (const auto& fn : { f1, f2 })
            std::filesystem::remove(fn);
        rtl::p3utils::P3UnSetEnv("GDXCOMPRESS");
        rtl::p3utils::P3UnSetEnv("GDXCONVERT");
    }

    TEST_CASE("Test convert and compress") {
        testWithCompressConvert();
        testWithCompressConvert(false, "v5");
        testWithCompressConvert(true,  "v5");
        testWithCompressConvert(false, "v7");
        testWithCompressConvert(true,  "v7");
    }

    TEST_CASE("Test symbol index max UEL length") {
        const std::string gdxfn = acquireGDXforModel("trnsport");
        testReads(gdxfn, gdxfn, [&](GDXInterface &pgx) {
            std::array<int, global::gmsspecs::MaxDim> lengthInfo {};
            int maxUelLen = pgx.gdxSymbIndxMaxLength(7, lengthInfo.data()); // c
            REQUIRE_EQ(9, maxUelLen); // len(san-diego)=9
            REQUIRE_EQ(9, lengthInfo[0]); // san-diego
            REQUIRE_EQ(8, lengthInfo[1]); // new-york
        });
        std::filesystem::remove(gdxfn);
    }

    TEST_SUITE_END();

}