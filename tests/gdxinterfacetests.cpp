#include <filesystem>
#include "doctest.h"
#include "../xpwrap.h"
#include "../gxfile.h"
#include <cassert>
#include <numeric>
#include <random>

using namespace std::literals::string_literals;
using namespace gdxinterface;

namespace tests::gdxinterfacetests {
    TEST_SUITE_BEGIN("GDX interface tests");

    std::list<std::function<GDXInterface*()>> getBuilders();
    void basicTest(const std::function<void(GDXInterface&)> &cb, int ub = -1);
    void testReads(const std::string &filename1,
                   const std::string &filename2,
                   const std::function<void(GDXInterface&)> &cb);
    void runGdxToYamlScript(const std::string &filename1 = ""s, const std::string &filename2 = ""s);
    void checkForMismatches(const std::string &filename1, const std::string &filename2, bool skipDiffing);
    std::string makeCorporateGDXAvailable();
    void testMatchingWrites(const std::string &filename1,
                            const std::string &filename2,
                            const std::function<void(GDXInterface&)> &cb,
                            bool skipDiffing = false);
    void writeMappedRecordsOutOfOrder(GDXInterface &pgx);
    void domainSetGetTestSetupPrefix(GDXInterface &pgx);
    std::string acquireGDXforModel(const std::string &model);
    void commonSetGetDomainTests(const std::vector<std::string> &domainNames,
                                 const std::vector<int> &domainIndices);
    void testReadModelGDX(const std::string &model, const std::function<void(GDXInterface&)> &func);
    void testWithCompressConvert(bool compress = false, const std::string &convert = ""s);

    //static std::ostream &my_cout = std::cout;
    static std::ostream mycout {&gxfile::null_buffer};

    static std::ofstream slowdownReport {"slowdown.txt"};

    std::list<std::function<GDXInterface*()>> getBuilders() {
        static std::string ErrMsg;
        std::list<std::function<GDXInterface*()>> builders {
                [&]() { return new xpwrap::GDXFile{ErrMsg}; },
                [&]() { return new gxfile::TGXFileObj{ErrMsg}; }
        };
        return builders;
    }

    void basicTest(const std::function<void(GDXInterface&)> &cb, int ub) {
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
            char msg[GMS_SSSIZE];
            REQUIRE(pgx.gdxErrorStr(pgx.gdxGetLastError(), msg));
            REQUIRE(!strcmp("File name is empty", msg));
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
            char errMsg[GMS_SSSIZE];
            REQUIRE(pgx.gdxErrorStr(ErrNr, errMsg));
            REQUIRE(!strcmp("No such file or directory", errMsg));
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

            char queriedUel[GMS_SSSIZE];
            int uelMap;
            REQUIRE(pgx.gdxUMUelGet(1, queriedUel, uelMap));
            REQUIRE(!strcmp("myuel", queriedUel));

            pgx.gdxRenameUEL("myuel", "newname");
            REQUIRE(pgx.gdxUMUelGet(1, queriedUel, uelMap));
            REQUIRE(!strcmp("newname", queriedUel));

            pgx.gdxClose();
        });
        std::filesystem::remove(fn);
    }

    TEST_CASE("Test get current active symbol dimension") {
        std::string fn{"sym_dim.gdx"};
        basicTest([&](GDXInterface &pgx) {
            int ErrNr;
            REQUIRE(pgx.gdxOpenWrite(fn, "gdxinterfacetest", ErrNr));
            REQUIRE(pgx.gdxDataWriteRawStart("mysym", "Some explanatory text.", 2, dt_par, 0));
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

    void runGdxToYamlScript(const std::string &filename1, const std::string &filename2) {
#if defined(__APPLE__) || defined(__linux__)
        const std::string pythonInterpreter {"/usr/bin/python3"};
#else
        const std::string pythonInterpreter {"python"};
#endif
        std::string f2 = filename2.empty() ? ""s : " "s + filename2;
        int rc {system((pythonInterpreter + " gdxtoyaml/main.py "s + filename1 + f2).c_str())};
        REQUIRE_FALSE(rc);
    }

    void checkForMismatches(const std::string &filename1, const std::string &filename2, bool skipDiffing) {
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

    void testMatchingWrites(const std::string &filename1,
                            const std::string &filename2,
                            const std::function<void(GDXInterface&)> &cb,
                            bool skipDiffing) {
        testWriteOp<xpwrap::GDXFile>(filename1, cb);
        testWriteOp<gxfile::TGXFileObj>(filename2, cb);
        checkForMismatches(filename1, filename2, skipDiffing);
    }

    TEST_CASE("Test adding uels (raw mode)") {
        std::array filenames{ "uel_wrapper.gdx"s, "uel_port.gdx"s };
        auto [f1, f2] = filenames;
        testMatchingWrites(f1, f2, [](GDXInterface &pgx) {
            REQUIRE(pgx.gdxUELRegisterRawStart());
            REQUIRE(pgx.gdxUELRegisterRaw(""));
            REQUIRE(pgx.gdxUELRegisterRaw("New-York"));
            std::string stillOk(63, 'i'), tooLong(64, 'i');
            REQUIRE(pgx.gdxUELRegisterRaw(stillOk.c_str()));
            REQUIRE_FALSE(pgx.gdxUELRegisterRaw(tooLong.c_str()));
            REQUIRE(pgx.gdxUELRegisterDone());
        });
        testReads(f1, f2, [](GDXInterface &pgx) {
            int uelCnt, highMap, uelMap;
            char uel[GMS_SSSIZE];
            REQUIRE(pgx.gdxUMUelInfo(uelCnt, highMap));
            REQUIRE_EQ(0, highMap);
            REQUIRE_EQ(3, uelCnt);
            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));
            REQUIRE(uel[0] == '\0');
            REQUIRE_EQ(-1, uelMap);
            REQUIRE(pgx.gdxUMUelGet(2, uel, uelMap));
            REQUIRE(!strcmp("New-York", uel));
            REQUIRE_EQ(-1, uelMap);
            REQUIRE_FALSE(pgx.gdxGetUEL(2, uel));
            REQUIRE(strcmp("New-York", uel));
            REQUIRE_FALSE(pgx.gdxUMUelGet(23, uel, uelMap));
            REQUIRE_EQ("?L__23"s, uel);
            REQUIRE_EQ(-1, uelMap);
            int uelNr;
            REQUIRE(pgx.gdxUMFindUEL("New-York", uelNr, uelMap));
            REQUIRE_EQ(2, uelNr);
            REQUIRE_EQ(-1, uelMap);
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
            std::string stillOk(63, 'i'), tooLong(64, 'i');
            REQUIRE(pgx.gdxUELRegisterStr(stillOk.c_str(), uelNr));
            REQUIRE_FALSE(pgx.gdxUELRegisterStr(tooLong.c_str(), uelNr));
            REQUIRE(pgx.gdxUELRegisterDone());
        });
        testReads(f1, f2, [](GDXInterface& pgx) {
            int uelCnt, highMap, uelMap;
            char uel[GMS_SSSIZE];
            REQUIRE(pgx.gdxUMUelInfo(uelCnt, highMap));
            REQUIRE_EQ(0, highMap);
            REQUIRE_EQ(2, uelCnt);
            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));
            REQUIRE(!strcmp("TheOnlyUEL", uel));
            REQUIRE_EQ(-1, uelMap);
            REQUIRE_FALSE(pgx.gdxGetUEL(1, uel));
            REQUIRE(strcmp("TheOnlyUEL", uel));
        });
        for (const auto& fn : filenames)
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test adding uels (mapped mode)") {
        std::array filenames{ "uel_mapped_wrapper.gdx"s, "uel_mapped_port.gdx"s };
        auto [f1, f2] = filenames;
        testMatchingWrites(f1, f2, [](GDXInterface& pgx) {
            REQUIRE(pgx.gdxUELRegisterMapStart());
            REQUIRE_FALSE(pgx.gdxUELRegisterMap(3, std::string(64, 'i').c_str()));
            REQUIRE(pgx.gdxUELRegisterMap(3, "TheOnlyUEL"));
            REQUIRE(pgx.gdxUELRegisterMap(8, std::string(63, 'i').c_str()));
            REQUIRE(pgx.gdxUELRegisterDone());
        });
        testReads(f1, f2, [](GDXInterface& pgx) {
            int uelCnt, highMap, uelMap;
            char uel[GMS_SSSIZE];

            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));
            REQUIRE(!strcmp("TheOnlyUEL", uel));
            REQUIRE_EQ(-1, uelMap);
            REQUIRE(pgx.gdxUMUelInfo(uelCnt, highMap));
            REQUIRE_EQ(0, highMap);
            REQUIRE_EQ(2, uelCnt);

            REQUIRE(pgx.gdxUELRegisterMapStart());
            REQUIRE(pgx.gdxUELRegisterMap(3, "TheOnlyUEL"));
            REQUIRE(pgx.gdxUELRegisterDone());

            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));
            REQUIRE(!strcmp("TheOnlyUEL", uel));
            REQUIRE_EQ(3, uelMap);

            REQUIRE(pgx.gdxUMUelInfo(uelCnt, highMap));
            REQUIRE_EQ(3, highMap);
            REQUIRE_EQ(2, uelCnt);
            
            REQUIRE(pgx.gdxGetUEL(3, uel));
            REQUIRE(!strcmp("TheOnlyUEL", uel));
        });
        for (const auto& fn : filenames)
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test write and read record raw") {
        std::string f1{"rwrecordraw_wrapper.gdx"}, f2{"rwrecordraw_port.gdx"};
        int key;
        TgdxValues values{};
        testMatchingWrites(f1, f2, [&](GDXInterface &pgx) {
            REQUIRE(pgx.gdxUELRegisterRawStart());
            REQUIRE(pgx.gdxUELRegisterRaw("TheOnlyUEL"));
            REQUIRE(pgx.gdxUELRegisterDone());
            REQUIRE(pgx.gdxDataWriteRawStart("mysym", "This is my symbol!", 1, dt_par, 0));
            key = 1;
            values[GMS_VAL_LEVEL] = 3.141;
            REQUIRE(pgx.gdxDataWriteRaw(&key, values.data()));
            REQUIRE(pgx.gdxDataWriteDone());

            REQUIRE(pgx.gdxDataWriteRawStart("myscalar", "This is a scalar!", 0, dt_par, 0));
            REQUIRE(pgx.gdxDataWriteDone());
        });
        testReads(f1, f2, [&](GDXInterface &pgx) {
            char uel[GMS_SSSIZE];
            int uelMap;
            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));
            REQUIRE(!strcmp("TheOnlyUEL", uel));
            REQUIRE_EQ(-1, uelMap);
            int NrRecs;
            REQUIRE(pgx.gdxDataReadRawStart(1, NrRecs));
            REQUIRE_EQ(1, NrRecs);
            int dimFrst;
            REQUIRE(pgx.gdxDataReadRaw(&key, values.data(), dimFrst));
            REQUIRE(pgx.gdxDataReadDone());
            REQUIRE_EQ(1, key);
            REQUIRE_EQ(3.141, values[GMS_VAL_LEVEL]);

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
        TgdxValues values{};
        testMatchingWrites(f1, f2, [&](GDXInterface &pgx) {
            REQUIRE(pgx.gdxDataWriteStrStart("mysym", "This is my symbol!", 1, dt_par, 0));
            values[GMS_VAL_LEVEL] = 3.141;

            char empty = '\0';
            const char *emptyPtr = &empty;
            REQUIRE(pgx.gdxDataWriteStr(&emptyPtr, values.data()));

            keyNames[0] = "TheOnlyUEL"s;
            const char *keyptrs[] = {keyNames[0].c_str()};
            REQUIRE(pgx.gdxDataWriteStr(keyptrs, values.data()));

            std::string almostTooLongButStillOk(63, 'i');
            keyNames[0] = almostTooLongButStillOk;
            keyptrs[0] = keyNames[0].c_str();
            REQUIRE(pgx.gdxDataWriteStr(keyptrs, values.data()));

            std::string oneCharTooLong(64, 'i');
            keyNames[0] = oneCharTooLong;
            keyptrs[0] = keyNames[0].c_str();
            REQUIRE_FALSE(pgx.gdxDataWriteStr(keyptrs, values.data()));

            REQUIRE(pgx.gdxDataWriteDone());
        });
        testReads(f1, f2, [&](GDXInterface &pgx) {
            int NrRecs;
            REQUIRE(pgx.gdxDataReadStrStart(1, NrRecs));
            REQUIRE_EQ(3, NrRecs);

            int dimFrst;
            REQUIRE(pgx.gdxDataReadStr(keyNames.ptrs(), values.data(), dimFrst));
            REQUIRE(keyNames[0].str().empty());
            REQUIRE_EQ(3.141, values[GMS_VAL_LEVEL]);

            REQUIRE(pgx.gdxDataReadStr(keyNames.ptrs(), values.data(), dimFrst));
            REQUIRE_EQ("TheOnlyUEL"s, keyNames[0].str());
            REQUIRE_EQ(3.141, values[GMS_VAL_LEVEL]);

            REQUIRE(pgx.gdxDataReadStr(keyNames.ptrs(), values.data(), dimFrst));
            REQUIRE_EQ(std::string(63, 'i'), keyNames[0].str());
            REQUIRE_EQ(3.141, values[GMS_VAL_LEVEL]);

            REQUIRE(pgx.gdxDataReadDone());
        });
        for (const auto& fn : {f1, f2})
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test getting special values") {
        std::array<double, GMS_SVIDX_MAX> specialValuesFromWrap{}, specialValuesFromPort{};
        std::string ErrMsg;
        {
            xpwrap::GDXFile pgx{ErrMsg};
            REQUIRE(pgx.gdxGetSpecialValues(specialValuesFromWrap.data()));
        }
        {
            gxfile::TGXFileObj pgx{ErrMsg};
            REQUIRE(pgx.gdxGetSpecialValues(specialValuesFromPort.data()));
        }
        for(int i{}; i<(int)specialValuesFromPort.size(); i++) {
            const double eps = 0.001;
            REQUIRE(specialValuesFromWrap[i] - specialValuesFromPort[i] < eps);
        }
    }

    TEST_CASE("Test setting special values") {
        basicTest([&](GDXInterface &pgx) {
            std::array<double, GMS_SVIDX_MAX> moddedSpecVals {}, queriedSpecVals {};
            pgx.gdxGetSpecialValues(moddedSpecVals.data());
            moddedSpecVals[gxfile::TgdxIntlValTyp::vm_valpin] = 0.0;
            pgx.gdxSetSpecialValues(moddedSpecVals.data());
            pgx.gdxGetSpecialValues(queriedSpecVals.data());
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
            REQUIRE(pgx.gdxUELRegisterMap(pair.first, pair.second.c_str()));
        }
        REQUIRE(pgx.gdxUELRegisterDone());

        REQUIRE(pgx.gdxDataWriteMapStart("irregularSym", "So out of order!", 1, dt_par, 0));
        int key;
        TgdxValues values{};
        for(const auto ix : randomOrder) {
            key = ix;
            values[GMS_VAL_LEVEL] = 3.141 * ix;
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
        TgdxValues values{};
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
            char uel[GMS_SSSIZE];
            REQUIRE(pgx.gdxGetUEL(3, uel));
            REQUIRE(!strcmp("First", uel));
            REQUIRE(pgx.gdxDataWriteMapStart("mysym", "Single record", 1, dt_par, 0));
            key = 3;
            values[GMS_VAL_LEVEL] = 3.141;
            REQUIRE(pgx.gdxDataWriteMap(&key, values.data()));
            REQUIRE(pgx.gdxDataWriteDone());

            // User UEL indices non-increasing
            REQUIRE(pgx.gdxDataWriteMapStart("mysym2", "Four records", 1, dt_par, 0));
            std::array<int, 4> expKey { 3, 4, 5, 2 };
            for(int i : expKey)
                REQUIRE(pgx.gdxDataWriteMap(&i, values.data()));
            REQUIRE(pgx.gdxDataWriteDone());

            REQUIRE_EQ(0, pgx.gdxErrorCount());
            REQUIRE_EQ(0, pgx.gdxDataErrorCount());
        });
        testReads(f1, f2, [&](GDXInterface& pgx) {
            char uel[GMS_SSSIZE];
            int uelMap;
            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));
            REQUIRE(!strcmp("First", uel));
            REQUIRE_EQ(-1, uelMap);

            REQUIRE(pgx.gdxUELRegisterMapStart());
            REQUIRE(pgx.gdxUELRegisterMap(3, "First"));
            REQUIRE(pgx.gdxUELRegisterMap(4, "Second"));
            REQUIRE(pgx.gdxUELRegisterMap(5, "Third"));
            REQUIRE(pgx.gdxUELRegisterMap(2, "Fourth"));
            REQUIRE(pgx.gdxUELRegisterDone());

            REQUIRE(pgx.gdxGetUEL(3, uel));
            REQUIRE(!strcmp("First", uel));

            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));            
            REQUIRE(!strcmp("First", uel));
            REQUIRE_EQ(3, uelMap);

            int NrRecs;
            REQUIRE(pgx.gdxDataReadMapStart(1, NrRecs));
            REQUIRE_EQ(1, NrRecs);
            int dimFrst;
            REQUIRE(pgx.gdxDataReadMap(1, &key, values.data(), dimFrst));
            REQUIRE_EQ(0, pgx.gdxDataErrorCount());
            REQUIRE_EQ(3, key);
            REQUIRE_EQ(3.141, values[GMS_VAL_LEVEL]);
            REQUIRE_EQ(1, dimFrst);
            REQUIRE(pgx.gdxDataReadDone());

            REQUIRE(pgx.gdxDataReadMapStart(2, NrRecs));
            REQUIRE_EQ(4, NrRecs);
            // unordered mapped write comes out ordered
            std::array<int, 4> expKey { 2, 3, 4, 5 };
            for(int i{1}; i<=(int)expKey.size(); i++) {
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
        TgdxValues values{};
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
            char uel[GMS_SSSIZE];
            REQUIRE(pgx.gdxGetUEL(5, uel));
            REQUIRE(!strcmp("First", uel));

            // User UEL indices increasing
            REQUIRE(pgx.gdxDataWriteMapStart("mysym", "Four records", 1, dt_par, 0));
            std::array<int, 4> expKey { 5, 6, 7, 8 };
            for(int i : expKey)
                REQUIRE(pgx.gdxDataWriteMap(&i, values.data()));
            REQUIRE(pgx.gdxDataWriteDone());

            REQUIRE_EQ(0, pgx.gdxErrorCount());
            REQUIRE_EQ(0, pgx.gdxDataErrorCount());
        });
        testReads(f1, f2, [&](GDXInterface& pgx) {
            char uel[GMS_SSSIZE];
            int uelMap;
            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));
            REQUIRE(!strcmp("First", uel));
            REQUIRE_EQ(-1, uelMap);

            REQUIRE(pgx.gdxUELRegisterMapStart());
            REQUIRE(pgx.gdxUELRegisterMap(5, "First"));
            REQUIRE(pgx.gdxUELRegisterMap(6, "Second"));
            REQUIRE(pgx.gdxUELRegisterMap(7, "Third"));
            REQUIRE(pgx.gdxUELRegisterMap(8, "Fourth"));
            REQUIRE(pgx.gdxUELRegisterDone());

            REQUIRE(pgx.gdxGetUEL(5, uel));
            REQUIRE(!strcmp("First", uel));

            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));
            REQUIRE(!strcmp("First", uel));
            REQUIRE_EQ(5, uelMap);

            int NrRecs;
            REQUIRE(pgx.gdxDataReadMapStart(1, NrRecs));
            REQUIRE_EQ(4, NrRecs);
            // should still be ordered
            std::array<int, 4> expKey { 5, 6, 7, 8 };
            for(int i{1}; i<=(int)expKey.size(); i++) {
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

        REQUIRE(pgx.gdxDataWriteStrStart("i", "", 1, dt_set, 0));
        REQUIRE(pgx.gdxDataWriteDone());
        REQUIRE(pgx.gdxSystemInfo(numSyms, numUels));
        REQUIRE_EQ(1, numSyms);

        REQUIRE(pgx.gdxDataWriteStrStart("j", "", 1, dt_set, 0));
        REQUIRE(pgx.gdxDataWriteDone());

        REQUIRE(pgx.gdxAddAlias("k", "i"));

        REQUIRE(pgx.gdxSystemInfo(numSyms, numUels));
        REQUIRE_EQ(3, numSyms);

        REQUIRE(pgx.gdxDataWriteStrStart("newd", "Same domain as d", 2, dt_par, 0));
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

        TgdxStrIndex newSymDomainNames {};
        std::array<int, 2> newSymDomainIndices {};
        for(int i{}; i<(int)domainNames.size(); i++) {
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
            REQUIRE(pgx.gdxDataWriteStrStart("i", "A set", 1, dt_set, 0));
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
            char explText[GMS_SSSIZE], symbolName[GMS_SSSIZE];
            REQUIRE(pgx.gdxSymbolInfoX(2, numRecords, userInfo, explText));
            REQUIRE(pgx.gdxSymbolInfo(2, symbolName, dim, typ));
            REQUIRE_EQ(0, numRecords);
            REQUIRE_EQ(1, userInfo); // symbol index of "i"
            REQUIRE(!strcmp("Aliased with i", explText));
            REQUIRE(!strcmp("aliasI", symbolName));
            REQUIRE_EQ(1, dim);
            REQUIRE_EQ(dt_alias, typ);
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
            REQUIRE(pgx.gdxDataWriteRawStart("i", "expl", 1, dt_set, 0));
            TgdxUELIndex keys;
            TgdxValues values;
            keys[0] = 1;
            values[GMS_VAL_LEVEL] = 1;
            REQUIRE(pgx.gdxDataWriteRaw(keys.data(), values.data()));
            REQUIRE(pgx.gdxDataWriteDone());
            int txtNr;
            REQUIRE(pgx.gdxAddSetText("set text", txtNr));
            REQUIRE_EQ(1, txtNr);
            // Check adding twice: Should give same text number!
            REQUIRE(pgx.gdxAddSetText("set text", txtNr));
            REQUIRE_EQ(1, txtNr);
            int elemNode;
            char elemTxt[GMS_SSSIZE];
            REQUIRE(pgx.gdxGetElemText(txtNr, elemTxt, elemNode));
            REQUIRE(!strcmp("set text", elemTxt));
            REQUIRE_EQ(0, elemNode);
            REQUIRE_FALSE(pgx.gdxSetTextNodeNr(200, 42));
            REQUIRE(pgx.gdxSetTextNodeNr(1, 23));
            REQUIRE(pgx.gdxGetElemText(1, elemTxt, elemNode));
            REQUIRE(!strcmp("set text", elemTxt));
            REQUIRE_EQ(23, elemNode);
        });
        for (const auto& fn : { f1, f2 })
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test invalid raw writing error processing") {
        const std::string fn {"tmpfile.gdx"s};
        int key;
        TgdxValues values{};
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
            REQUIRE(pgx.gdxDataWriteRawStart("i", "expl", 1, dt_set, 0));
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
            char errMsg[GMS_SSSIZE];
            REQUIRE(pgx.gdxErrorStr(ec, errMsg));
            REQUIRE(!strcmp("Data not sorted when writing raw", errMsg));
            pgx.gdxClose();
        });
        std::filesystem::remove(fn);
    }

    std::string acquireGDXforModel(const std::string &model) {
        const std::string model_fn = model + ".gms"s,
                          log_fn = model + "Log.txt"s,
                          fnpf = "model_data"s;
        std::string gdxfn = fnpf+".gdx"s; // non-const so we get automatic move
        int rc = std::system(("gamslib "s + model + " > gamslibLog.txt"s).c_str());
        REQUIRE_FALSE(rc);
        std::filesystem::remove("gamslibLog.txt");
        REQUIRE(std::filesystem::exists(model_fn));
        rc = std::system(("gams " + model_fn + " gdx="s + fnpf + " lo=0 o=lf > " + log_fn).c_str());
        REQUIRE_FALSE(rc);
        std::filesystem::remove(log_fn);
        std::filesystem::remove(model_fn);
        std::filesystem::remove("lf");
        REQUIRE(std::filesystem::exists(gdxfn));
        return gdxfn;
    }

    void testReadModelGDX(const std::string &model, const std::function<void(GDXInterface&)> &func) {
        const std::string gdxfn = acquireGDXforModel(model);
        testReads(gdxfn, gdxfn, func);
        std::filesystem::remove(gdxfn);
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
                char uel[GMS_SSSIZE];
                int uelMap;
                REQUIRE(pgx.gdxUMUelGet(i, uel, uelMap));
                REQUIRE(uel[0] != '\0');
                uelsSeen.emplace_back(uel);
            }
            REQUIRE_EQ(expectedUels.size(), uelsSeen.size());
            for(const auto &uel : expectedUels)
                REQUIRE(std::find(uelsSeen.begin(), uelsSeen.end(), uel) != uelsSeen.end());

            for(int i{1}; i<=numSymbols; i++) {
                char name[GMS_SSSIZE];
                int dim, typ;
                REQUIRE(pgx.gdxSymbolInfo(i, name, dim, typ));
                REQUIRE(dim >= 0);
                REQUIRE(typ >= 0);
                REQUIRE(name[0] != '\0');
                symbolsSeen.emplace_back(name);

                char explanatoryText[GMS_SSSIZE];
                int userInfo, numRecords;
                REQUIRE(pgx.gdxSymbolInfoX(i, numRecords, userInfo, explanatoryText));
                if(numRecords) {
                    int numRecords2;
                    REQUIRE(pgx.gdxDataReadStrStart(i, numRecords2));
                    REQUIRE_EQ(numRecords, numRecords2);
                    StrIndexBuffers keyNames;
                    TgdxValues values;
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
            REQUIRE(pgx.gdxDataWriteStrStart("i", "A set", 1, dt_set, 0));
            StrIndexBuffers keys{};
            TgdxValues vals{};
            for(int i=1; i<=8; i++) {
                keys[0] = "uel_" + std::to_string(i);
                const char *keyptrs[] = {keys[0].c_str()};
                REQUIRE(pgx.gdxDataWriteStr(keyptrs, vals.data()));
            }
            REQUIRE(pgx.gdxDataWriteDone());

            int symNr;
            REQUIRE(pgx.gdxFindSymbol("*", symNr));
            REQUIRE_EQ(0, symNr);

            char symName[GMS_SSSIZE];
            int dim, typ;
            REQUIRE(pgx.gdxSymbolInfo(0, symName, dim, typ));
            REQUIRE(!strcmp("*", symName));
            REQUIRE_EQ(1, dim);
            REQUIRE_EQ(dt_set, typ);

            int recCnt, userInfo;
            char explText[GMS_SSSIZE];
            REQUIRE(pgx.gdxSymbolInfoX(0, recCnt, userInfo, explText));
            REQUIRE_EQ(0, recCnt);
            REQUIRE_EQ(0, userInfo);
            REQUIRE(!strcmp("Universe", explText));

            REQUIRE_FALSE(pgx.gdxSymbolInfoX(999, recCnt, userInfo, explText));
            REQUIRE_EQ(0, recCnt);
            REQUIRE_EQ(0, userInfo);
            REQUIRE(explText[0] == '\0');

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
            REQUIRE(pgx.gdxDataWriteStrStart("i", "expl", 1, dt_set, 0));
            std::string key;
            TgdxValues vals{};
            for(int i{}; i<6; i++) {
                key = "i"s+std::to_string(i+1);
                const char *keyptrs[] = {key.c_str()};
                REQUIRE(pgx.gdxDataWriteStr(keyptrs, vals.data()));
            }
            REQUIRE(pgx.gdxDataWriteDone());

            REQUIRE(pgx.gdxDataWriteStrStart("j", "subset of i", 1, dt_set, 0));
            key = "i"s;
            const char *keyptrs[] = {key.c_str()};
            pgx.gdxSymbolSetDomain(keyptrs);
            //REQUIRE(pgx.gdxSymbolSetDomain(keys));
            std::array<int, 2> subset = {2, 4};
            for(int i : subset) {
                key = "i"s+std::to_string(i);
                keyptrs[0] = key.c_str();
                REQUIRE(pgx.gdxDataWriteStr(keyptrs, vals.data()));
            }

            // adding an uel not from superset should fail
            key = "not_in_i"s;
            keyptrs[0] = key.c_str();
            REQUIRE(pgx.gdxDataWriteStr(keyptrs, vals.data()));

            REQUIRE(pgx.gdxDataWriteDone());

            REQUIRE_EQ(1, pgx.gdxErrorCount());
            char msg[GMS_SSSIZE];
            REQUIRE(pgx.gdxErrorStr(pgx.gdxGetLastError(), msg));
            REQUIRE_EQ("Domain violation"s, msg);

            // check if error uels was correctly set (more specific domain violation details)
            REQUIRE_EQ(1, pgx.gdxDataErrorCount());
            std::array<int, 20> errRecKeys {};
            std::array<double, 5> errRecVals {};
            REQUIRE(pgx.gdxDataErrorRecord(1, errRecKeys.data(), errRecVals.data()));
            char uelNotInSuperset[GMS_SSSIZE];
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
            REQUIRE(pgx.gdxDataWriteStrStart("i", "A set", 1, dt_set, 0));
            TgdxStrIndex keys{};
            TgdxValues vals{};
            for(int i=1; i<=8; i++) {
                keys[0] = "uel_" + std::to_string(i);
                const char *keyptrs[] = { keys[0].c_str() };
                REQUIRE(pgx.gdxDataWriteStr(keyptrs, vals.data()));
            }
            const char *keyptrs[] = { keys[0].c_str() };
            REQUIRE(pgx.gdxDataWriteStr(keyptrs, vals.data()));
            REQUIRE(pgx.gdxDataWriteDone());
            REQUIRE_EQ(1, pgx.gdxErrorCount());
            char msg[GMS_SSSIZE];
            pgx.gdxErrorStr(pgx.gdxGetLastError(), msg);
            REQUIRE(!strcmp("Duplicate keys", msg));
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
            char acroName[GMS_SSSIZE], acroText[GMS_SSSIZE];
            int acroIndex;
            REQUIRE(pgx.gdxAcronymGetInfo(1, acroName, acroText, acroIndex));
            REQUIRE_EQ("myacr"s, acroName);
            REQUIRE_EQ("my acronym"s, acroText);
            REQUIRE_EQ(23, acroIndex);
            REQUIRE_EQ(1, pgx.gdxAcronymCount());

            REQUIRE(pgx.gdxAcronymSetInfo(1, "myacr_mod"s, "my acronym_mod"s, 23));
            REQUIRE(pgx.gdxAcronymGetInfo(1, acroName, acroText, acroIndex));
            REQUIRE_EQ("myacr_mod"s, acroName);
            REQUIRE_EQ("my acronym_mod"s, acroText);
            REQUIRE_EQ(23, acroIndex);

            REQUIRE_FALSE(pgx.gdxAcronymGetInfo(999, acroName, acroText, acroIndex));
            REQUIRE(acroName[0] == '\0');
            REQUIRE(acroText[0] == '\0');
            REQUIRE_EQ(0, acroIndex);

            REQUIRE(pgx.gdxAcronymAdd("anotherOne"s, "my second acronym"s, 2));
            REQUIRE_EQ(2, pgx.gdxAcronymCount());
        });
        testReads(f1, f2, [](GDXInterface &pgx) {
            REQUIRE_EQ(2, pgx.gdxAcronymCount());
            char acroName[GMS_SSSIZE], acroText[GMS_SSSIZE];
            int acroIndex;
            pgx.gdxAcronymGetInfo(1, acroName, acroText, acroIndex);
            REQUIRE(!strcmp("myacr_mod", acroName));
        });
        for (const auto& fn : { f1, f2 })
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test comments addition and querying") {
        std::array filenames{ "comments_wrapper.gdx"s, "comments_port.gdx"s };
        auto [f1, f2] = filenames;
        testMatchingWrites(f1, f2, [](GDXInterface &pgx) {
            REQUIRE(pgx.gdxDataWriteStrStart("i", "expl text", 1, dt_set, 0));
            REQUIRE(pgx.gdxDataWriteDone());
            const auto commentStrExp {"A fancy comment!"s};
            REQUIRE(pgx.gdxSymbolAddComment(1, commentStrExp.c_str()));
            char commentStrGot[GMS_SSSIZE];
            REQUIRE(pgx.gdxSymbolGetComment(1, 1, commentStrGot));
            REQUIRE_EQ(commentStrExp, commentStrGot);
            REQUIRE_FALSE(pgx.gdxSymbolAddComment(-5, "should not work"));
            REQUIRE_FALSE(pgx.gdxSymbolAddComment(std::numeric_limits<int>::max(), "should not work"));
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
            REQUIRE(pgx.gdxUELRegisterRaw("seattle"));
            REQUIRE(pgx.gdxUELRegisterDone());

            pgx.gdxDataWriteRawStart("i", "cities", 1, dt_set, 0);
            TgdxValues vals {};
            vals[GMS_VAL_LEVEL] = 3.141;
            int key {1};
            pgx.gdxDataWriteRaw(&key, vals.data());
            pgx.gdxDataWriteDone();
        });
        testReads(f1, f2, [&](GDXInterface& pgx) {
            char txt[GMS_SSSIZE];
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
                pgx.gdxUELRegisterRaw(("uel_"s+std::to_string(i+1)).c_str());
            pgx.gdxUELRegisterDone();

            pgx.gdxDataWriteRawStart("j", "", 1, dt_set, 0);
            TgdxValues vals {};
            vals[GMS_VAL_LEVEL] = 3.141;
            int key;
            for(key=17; key <= 32; key++)
                pgx.gdxDataWriteRaw(&key, vals.data());
            REQUIRE(pgx.gdxDataWriteDone());
            REQUIRE(pgx.gdxDataWriteRawStart("jb", "", 1, dt_set, 0));
            TgdxStrIndex domainNames {};
            domainNames.front() = "j";
            StrIndexBuffers domainIds {&domainNames};
            REQUIRE(pgx.gdxSymbolSetDomain(domainIds.cptrs()));
            key = 17;
            REQUIRE(pgx.gdxDataWriteRaw(&key, vals.data()));
            REQUIRE(pgx.gdxDataWriteDone());
            if(pgx.gdxErrorCount() > 0) {
                int errNr = pgx.gdxGetLastError();
                char errMsg[GMS_SSSIZE];
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
            TgdxValues vals {};
            vals[GMS_VAL_LEVEL] = 1e+300;
            REQUIRE(pgx.gdxDataWriteRawStart("undef", "", 0, dt_par, 0));
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
            TgdxValues vals {};
            pgx.gdxDataReadRaw(nullptr, vals.data(), dimFirst);
            REQUIRE(pgx.gdxDataReadDone());
            double undef = vals[GMS_VAL_LEVEL];
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
            pgx.gdxUELRegisterRaw("a");
            pgx.gdxUELRegisterDone();
            pgx.gdxClose();
            pgx.gdxOpenAppend(getfn(cnt), prod, errNr);
            pgx.gdxRenameUEL("a", "b");
            pgx.gdxClose();
            cnt++;
        });
        cnt = 0;
        basicTest([&](GDXInterface &pgx) {
            pgx.gdxOpenRead(getfn(cnt), errNr);
            int uelMap;
            char uelStr[GMS_SSSIZE];
            pgx.gdxUMUelGet(1, uelStr, uelMap);
            REQUIRE_EQ("b"s, uelStr);
            pgx.gdxClose();
            std::filesystem::remove(getfn(cnt));
            cnt++;
        });
    }

    void testWithCompressConvert(bool compress, const std::string &convert) {
        const std::string f1 {"conv_compr_wrap.gdx"}, f2 {"conv_compr_port.gdx"};
        rtl::p3utils::P3SetEnv("GDXCOMPRESS", compress ? "1"s : "0"s);
        rtl::p3utils::P3SetEnv("GDXCONVERT", convert);
        testMatchingWrites(f1, f2, [&](GDXInterface &pgx) {
            TgdxValues vals {};
            REQUIRE(pgx.gdxDataWriteRawStart("undef", "", 0, dt_par, 0));
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
#ifndef __APPLE__
        testWithCompressConvert(false, "v5");
        testWithCompressConvert(true,  "v5");
#endif
        testWithCompressConvert(false, "v7");
        testWithCompressConvert(true,  "v7");
    }

    TEST_CASE("Test symbol index max UEL length") {
        testReadModelGDX("trnsport"s, [&](GDXInterface &pgx) {
            std::array<int, GLOBAL_MAX_INDEX_DIM> lengthInfo {};
            int maxUelLen = pgx.gdxSymbIndxMaxLength(7, lengthInfo.data()); // c
            REQUIRE_EQ(9, maxUelLen); // len(san-diego)=9
            REQUIRE_EQ(9, lengthInfo[0]); // san-diego
            REQUIRE_EQ(8, lengthInfo[1]); // new-york
        });
    }

    TEST_CASE("Test UEL table get max uel length") {
        testReadModelGDX("trnsport"s, [&](GDXInterface &pgx) {
            REQUIRE_EQ(9, pgx.gdxUELMaxLength());
        });
    }

    TEST_CASE("Test symbol info out of range") {
        testReadModelGDX("trnsport"s, [&](GDXInterface &pgx) {
            char symId[GMS_SSSIZE];
            int dim, typ;
            REQUIRE_FALSE(pgx.gdxSymbolInfo(99, symId, dim, typ));
            REQUIRE(symId[0] == '\0');
            REQUIRE_EQ(-1, dim);
            REQUIRE_EQ(dt_set, typ);
        });
    }

    /*TEST_CASE("Test reading old GDX file") {
        const std::string tmp_fn {R"(C:\Users\aschn\Desktop\leg_qa_GDX2_gams\gdx\20.6\taix.gdx)"};
        testReads(tmp_fn, tmp_fn, [&](GDXInterface &pgx) {
            int symbolCount, uelCount;
            REQUIRE(pgx.gdxSystemInfo(symbolCount, uelCount));
            REQUIRE_EQ(5, uelCount);
            REQUIRE_EQ(12, symbolCount);
        });
    }*/

    TEST_CASE("Test filter") {
        testReadModelGDX("trnsport"s, [](GDXInterface &pgx) {
            int nrRecs, dimFirst;
            // uels: seattle 1, san-diego 2, new-york 3, chicago 4, topeka 5
            std::array<int, 2> filterAction { gxfile::DOMC_EXPAND, gxfile::DOMC_EXPAND },
                               keys { 3, 1 }; // new-york, seattle
            std::array<double, GLOBAL_MAX_INDEX_DIM> values {};

            REQUIRE(pgx.gdxDataReadFilteredStart(5, filterAction.data(), nrRecs)); // symbol 'd'
            REQUIRE_EQ(6, nrRecs);
            REQUIRE(pgx.gdxDataReadMap(1, keys.data(), values.data(), dimFirst));
            REQUIRE(pgx.gdxDataReadDone());

            REQUIRE_FALSE(pgx.gdxFilterExists(1));
            REQUIRE(pgx.gdxFilterRegisterStart(1));
            REQUIRE(pgx.gdxFilterRegister(1)); // seattle
            REQUIRE(pgx.gdxFilterRegister(3)); // new-york
            REQUIRE(pgx.gdxFilterRegisterDone());
            REQUIRE(pgx.gdxFilterExists(1));

            filterAction[0] = filterAction[1] = 1;
            REQUIRE(pgx.gdxDataReadFilteredStart(5, filterAction.data(), nrRecs)); // symbol 'd'
            REQUIRE_EQ(6, nrRecs);
            REQUIRE(pgx.gdxDataReadMap(1, keys.data(), values.data(), dimFirst));
            REQUIRE(pgx.gdxDataReadDone());
        });
    }

    class AbstractWriteReadPair {
    protected:
        std::array<int, GLOBAL_MAX_INDEX_DIM> keys{};
        std::array<double, GMS_VAL_SCALE + 1> values{};
    public:
        virtual void write(GDXInterface &pgx, int count, const int *nums) = 0;
        virtual void read(GDXInterface &pgx, int count, const int *nums) = 0;
        virtual std::string getName() const = 0;

        virtual void reset() {
            keys.fill(0);
            values.fill(0.0);
        }
    };

    double perfBenchmarkCppVsDelphi(AbstractWriteReadPair &pair, bool randomOrderInsert = true);
    void enforceSlowdownLimit(AbstractWriteReadPair &pair, double limit);

    double perfBenchmarkCppVsDelphi(AbstractWriteReadPair &pair, bool randomOrderInsert) {
        const int upto {10000};
        auto nums = std::make_unique<std::array<int, upto>>();
        std::iota(nums->begin(), nums->end(), 1);
        if(randomOrderInsert)
            std::shuffle(nums->begin(), nums->end(), std::default_random_engine(23));
        const std::array<std::string, 2> methodNames {
                "xp-level wrap"s,
                "C++ port"s
        };
        std::array<double, 2> elapsedTimes {};
        const int ntries = 10;
        std::array<double, ntries> slowdowns{};
        std::map<std::string, std::string> gdxFns {
                {"xpwrap", "speed_test_wrapped.gdx"},
                {"tgxfileobj", "speed_test_ported.gdx"}
        };
        std::vector<std::string> tmpFiles;
        for (int n{}; n < ntries; n++) {
            int methodIx{};
            basicTest([&](GDXInterface& pgx) {
                const std::string implName = pgx.getImplName();
                int errNr;

                std::chrono::time_point startWrite = std::chrono::high_resolution_clock::now();
                REQUIRE(pgx.gdxOpenWrite(gdxFns[implName], "gdxinterfacetest", errNr));
                pair.reset();
                pair.write(pgx, upto, nums->data());
                pgx.gdxClose();
                std::chrono::time_point endWrite = std::chrono::high_resolution_clock::now();
                double timeWrite = std::chrono::duration<double, std::milli>(endWrite - startWrite).count();

                tmpFiles.push_back(gdxFns[implName]);
                if(tmpFiles.size() >= 2)
                    checkForMismatches(tmpFiles.front(), tmpFiles[1], false);

                std::chrono::time_point startRead = std::chrono::high_resolution_clock::now();
                REQUIRE(pgx.gdxOpenRead(gdxFns[implName], errNr));
                pair.reset();
                pair.read(pgx, upto, nums->data());
                pgx.gdxClose();
                std::chrono::time_point endRead = std::chrono::high_resolution_clock::now();
                double timeRead = std::chrono::duration<double, std::milli>(endRead - startRead).count();
                elapsedTimes[methodIx] = timeWrite + timeRead;

                /*std::cout << "Method " << std::to_string(methodIx + 1) << " ("s << methodNames[methodIx] <<
                    "): Time elapsed for entering set with "s << std::to_string(upto) << " elements: "s <<
                    std::to_string(elapsedTimes[methodIx]) << " ms" << std::endl;*/

                methodIx++;
            });
            for(const auto &s : tmpFiles)
                std::filesystem::remove(s);
            tmpFiles.clear();
            slowdowns[n] = elapsedTimes[1] / elapsedTimes[0];
        }
        return std::accumulate(slowdowns.begin(), slowdowns.end(), 0.0) / ntries;
    }

    class WriteReadRawPair : public AbstractWriteReadPair {
        void write(GDXInterface &pgx, int count, const int *nums) override {
            // Register many UELs
            REQUIRE(pgx.gdxUELRegisterRawStart());
            for (int i{}; i<count; i++)
                REQUIRE(pgx.gdxUELRegisterRaw(("i"s + std::to_string(nums[i])).c_str()));
            REQUIRE(pgx.gdxUELRegisterDone());
            // Write set symbol "i" with many records referencing the large number of UELs for its elements
            REQUIRE(pgx.gdxDataWriteRawStart("i"s, "a set"s, 1, dt_set, 0));
            for (int i{}; i<count; i++) {
                keys.front() = i+1;
                REQUIRE(pgx.gdxDataWriteRaw(keys.data(), values.data()));
            }
            REQUIRE(pgx.gdxDataWriteDone());
            // Write a many dimensional parameter symbol "d(i,...,i)" with many records
            const int paramDim {16};
            REQUIRE(pgx.gdxDataWriteRawStart("d"s, "a parameter"s, paramDim, dt_par, 0));
            for(int i{}; i<count; i++) {
                for(int d{}; d<paramDim; d++)
                    keys[d] = i+1;
                values[GMS_VAL_LEVEL] = i+1;
                REQUIRE(pgx.gdxDataWriteRaw(keys.data(), values.data()));
            }
            REQUIRE(pgx.gdxDataWriteDone());
            std::array<std::string, 20> domainNames;
            domainNames.fill("i"s);
            StrIndexBuffers sib {&domainNames};
            REQUIRE(pgx.gdxSymbolSetDomainX(2, sib.cptrs()));
        }

        void read(GDXInterface &pgx, int count, const int *nums) override {
            int numRecs;
            REQUIRE(pgx.gdxDataReadRawStart(1, numRecs));
            int dimFirst;
            for (int i{}; i < count; i++)
                REQUIRE(pgx.gdxDataReadRaw(keys.data(), values.data(), dimFirst));
            REQUIRE(pgx.gdxDataReadDone());
            REQUIRE(pgx.gdxDataReadRawStart(2, numRecs));
            for(int i{}; i< count; i++)
                REQUIRE(pgx.gdxDataReadRaw(keys.data(), values.data(), dimFirst));
            REQUIRE(pgx.gdxDataReadDone());
        }

        std::string getName() const override {
            return "raw"s;
        }
    };

    class WriteReadStrPair : public AbstractWriteReadPair {
        void write(GDXInterface &pgx, int count, const int *nums) override {
            REQUIRE(pgx.gdxDataWriteStrStart("i"s, "a set"s, 1, dt_set, 0));
            StrIndexBuffers sib;
            for (int i{}; i<count; i++) {
                sib[0] = "i"s + std::to_string(nums[i]);
                REQUIRE(pgx.gdxDataWriteStr(sib.cptrs(), values.data()));
            }
            REQUIRE(pgx.gdxDataWriteDone());
        }

        void read(GDXInterface &pgx, int count, const int *nums) override {
            int numRecs;
            REQUIRE(pgx.gdxDataReadStrStart(1, numRecs));
            int dimFirst;
            StrIndexBuffers sib;
            for (int i{}; i < count; i++)
                REQUIRE(pgx.gdxDataReadStr(sib.ptrs(), values.data(), dimFirst));
            REQUIRE(pgx.gdxDataReadDone());
        }

        std::string getName() const override {
            return "str"s;
        }
    };

    class WriteReadMappedPair : public AbstractWriteReadPair {
        static void registerMappedUels(GDXInterface &pgx, int count, const int *nums) {
            std::vector<std::string> uelIds(count);
            for(int i{}; i<(int)uelIds.size(); i++)
                uelIds[i] = "i"+std::to_string(i+1);
            std::shuffle(uelIds.begin(), uelIds.end(), std::default_random_engine(42));
            REQUIRE(pgx.gdxUELRegisterMapStart());
            for(int i{}; i<count; i++)
                REQUIRE(pgx.gdxUELRegisterMap(nums[i], uelIds[i].c_str()));
            REQUIRE(pgx.gdxUELRegisterDone());
        }

        void write(GDXInterface &pgx, int count, const int *nums) override {
            registerMappedUels(pgx, count, nums);
            REQUIRE(pgx.gdxDataWriteMapStart("i"s, "a set"s, 1, dt_set, 0));
            for (int i{}; i<count; i++) {
                keys.front() = nums[i];
                REQUIRE(pgx.gdxDataWriteMap(keys.data(), values.data()));
            }
            REQUIRE(pgx.gdxDataWriteDone());
        }

        void read(GDXInterface &pgx, int count, const int *nums) override {
            registerMappedUels(pgx, count, nums);
            int numRecs;
            REQUIRE(pgx.gdxDataReadMapStart(1, numRecs));
            REQUIRE_EQ(count, numRecs);
            int dimFirst;
            for (int i{}; i < count; i++)
                REQUIRE(pgx.gdxDataReadMap(i + 1, keys.data(), values.data(), dimFirst));
            REQUIRE(pgx.gdxDataReadDone());
        }

        std::string getName() const override {
            return "mapped";
        }
    };

    void enforceSlowdownLimit(AbstractWriteReadPair &pair, double limit) {
        const double avgSlowdown = perfBenchmarkCppVsDelphi(pair, true);
        //std::cout << "slowdown = " << avgSlowdown << " for " << pair.getName() << std::endl;
        slowdownReport << pair.getName() << ";"s << avgSlowdown << std::endl;
#ifdef NDEBUG
        REQUIRE(avgSlowdown <= limit);
#endif
    }

    TEST_CASE("Test performance of legacy vs. new GDX object for writing and reading records") {
        {
            WriteReadRawPair wrrp;
            enforceSlowdownLimit(wrrp, 1.0);
        }
        {
            WriteReadStrPair wrsp;
            enforceSlowdownLimit(wrsp, 1.15);
        }
        {
            WriteReadMappedPair wrmp;
            enforceSlowdownLimit(wrmp, 1.2);
        }
    }

    std::string makeCorporateGDXAvailable() {
        const std::string model_fn = "Corporate.gms"s,
                log_fn = "corporateLog.txt"s,
                fnpf = "corporate"s;
        std::string gdxfn = fnpf+".gdx"s; // non-const so we get automatic move
        int rc = std::system(("finlib Corporate > gamslibLog.txt"s).c_str());
        REQUIRE_FALSE(rc);
        std::filesystem::remove("gamslibLog.txt");
        REQUIRE(std::filesystem::exists(model_fn));
        rc = std::system(("gams " + model_fn + " gdx="s + fnpf + " lo=0 o=lf > " + log_fn).c_str());
        REQUIRE_FALSE(rc);
        std::filesystem::remove(log_fn);
        std::filesystem::remove(model_fn);
        std::filesystem::remove("CorporateCommonInclude.inc");
        std::filesystem::remove("CorporateScenarios.inc");
        std::filesystem::remove("lf");
        REQUIRE(std::filesystem::exists(gdxfn));
        return gdxfn;
    }

    TEST_CASE("Test performance of legacy vs. new GDX object for reading a big reference GDX file") {
#ifdef NO_SLOW_TESTS
        return;
#endif
        const std::string gdxfn = makeCorporateGDXAvailable();
        const int ntries {10};
        std::map<std::string, std::list<double>> elapsedTimes {};

        auto addEntry = [&](const std::string &implName, double elapsed) {
            auto [it,wasNew] = elapsedTimes.insert({implName, {}});
            it->second.push_back(elapsed);
        };

        std::array<int, GLOBAL_MAX_INDEX_DIM> keys{};
        std::array<double, GMS_VAL_SCALE + 1> values{};

        for(int n{}; n<ntries; n++) {
            testReads(gdxfn, gdxfn, [&](GDXInterface &pgx) {
                std::chrono::time_point startRead = std::chrono::high_resolution_clock::now();
                int symCnt, uelCnt;
                REQUIRE(pgx.gdxSystemInfo(symCnt, uelCnt));
                REQUIRE_EQ(34, symCnt);
                REQUIRE_EQ(10021, uelCnt);
                for(int syNr{1}; syNr <= symCnt; syNr++) {
                    int recCnt, userInfo;
                    char explTxt[GMS_SSSIZE];
                    REQUIRE(pgx.gdxSymbolInfoX(syNr, recCnt, userInfo, explTxt));
                    int nrRecs;
                    REQUIRE(pgx.gdxDataReadRawStart(syNr, nrRecs));
                    //REQUIRE_EQ(nrRecs, recCnt);
                    int dimFrst;
                    for(int i{}; i<nrRecs; i++)
                        REQUIRE(pgx.gdxDataReadRaw(keys.data(), values.data(), dimFrst));
                    REQUIRE(pgx.gdxDataReadDone());
                }
                double elapsed = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - startRead).count();
                addEntry(pgx.getImplName(), elapsed);
            });
        }
        std::filesystem::remove(gdxfn);

        std::map<std::string, double> averageElapsedForImpl;
        for(auto &[implName, elapsedSamples] : elapsedTimes) {
            double averageElapsed {};
            for(double elapsed : elapsedSamples)
                averageElapsed += elapsed;
            averageElapsed /= static_cast<double>(elapsedSamples.size());
            //std::cout << "Average elapsed time for method \"" << implName << "\" is: " << averageElapsed << std::endl;
            averageElapsedForImpl[implName] = averageElapsed;
        }

        double avgSlowdown = averageElapsedForImpl["tgxfileobj"] / averageElapsedForImpl["xpwrap"];
        slowdownReport << "corporate;" << avgSlowdown << std::endl;
#ifdef NDEBUG
        REQUIRE(avgSlowdown <= 1.2);
#endif
    }

    TEST_CASE("Correct set text numbers and element texts for agg.gdx") {
        if(!std::filesystem::exists("agg.gdx")) return;
        std::array<int, GLOBAL_MAX_INDEX_DIM> keys{};
        std::array<double, GMS_VAL_SCALE + 1> values{};
        std::string ErrMsg;
        int ErrNr, nrRecs, dimFrst;
        gdxinterface::GDXInterface *pgdx{};

        // Check set text numbers
        std::vector<int> memorizedSetTextNumbers{};
        for(int k{}; k<2; k++) {
            bool legacyRun = !k;
            pgdx = legacyRun ? (gdxinterface::GDXInterface *)(new xpwrap::GDXFile{ErrMsg}) : (gdxinterface::GDXInterface *)(new gxfile::TGXFileObj {ErrMsg});
            pgdx->gdxOpenRead("agg.gdx", ErrNr);
            pgdx->gdxDataReadRawStart(11, nrRecs);
            if(legacyRun) memorizedSetTextNumbers.resize(nrRecs);
            for (int i{}; i < nrRecs; i++) {
                pgdx->gdxDataReadRaw(keys.data(), values.data(), dimFrst);
                if(legacyRun) memorizedSetTextNumbers[i] = static_cast<int>(values[GMS_VAL_LEVEL]);
                REQUIRE_EQ(memorizedSetTextNumbers[i], values[GMS_VAL_LEVEL]);
                if(!legacyRun && memorizedSetTextNumbers[i] != values[GMS_VAL_LEVEL]) {
                    std::cout << "Mismatch at index " << i << std::endl;
                }
            }
            pgdx->gdxDataReadDone();
            pgdx->gdxClose();
            delete pgdx;
        }

        // Check set element texts too
        std::vector<std::string> setElemTxts{};
        int refCount{};
        for(int k{}; k<2; k++) {
            bool legacyRun = !k;
            pgdx = legacyRun ? (gdxinterface::GDXInterface *) (new xpwrap::GDXFile{ErrMsg})
                             : (gdxinterface::GDXInterface *) (new gxfile::TGXFileObj{ErrMsg});
            pgdx->gdxOpenRead("agg.gdx", ErrNr);
            int node{}, n{1};
            std::array<char, GMS_SSSIZE> text {};
            while(pgdx->gdxGetElemText(n, text.data(), node)) {
                if(legacyRun) setElemTxts.emplace_back(text.data());
                // Set texts must match
                std::string &s1 {setElemTxts[n-1]};
                std::string s2 {text.data()};
                if (!legacyRun && s1 != s2)
                    std::cout << "Mismatch \"" << s1 << "\" vs \"" << s2 << "\" at index " << n << std::endl;
                REQUIRE_EQ(s1, s2);
                n++;
            }
            // Cardinality must match!
            if(legacyRun) refCount = n-1;
            else REQUIRE_EQ(n-1, refCount);
            pgdx->gdxDataReadDone();
            pgdx->gdxClose();
            delete pgdx;
        }
    }

    // For BondIndexData.gdx
    double extractValueForDemExchangeRate(GDXInterface &pgdx, const std::string &fn);
    double extractValueForDemExchangeRate(GDXInterface &pgdx, const std::string &fn) {
        int ErrNr{};
        pgdx.gdxOpenRead(fn, ErrNr);
        int nrecs{};
        pgdx.gdxDataReadMapStart(38, nrecs);
        REQUIRE_EQ(3, nrecs);
        int dimFrst{}, key{};
        std::array<double, GMS_VAL_MAX> vals{};
        pgdx.gdxDataReadMap(0, &key, vals.data(), dimFrst);
        pgdx.gdxDataReadDone();
        pgdx.gdxClose();
        return vals.front();
    }

    TEST_CASE("Correct parameter values for BondIndexData") {
        std::string bidFn {"BondIndexData.gdx"s};
        if(!std::filesystem::exists(bidFn)) return;
        double expectedValue;
        std::string ErrMsg;
        {
            xpwrap::GDXFile pgdx {ErrMsg};
            expectedValue = extractValueForDemExchangeRate(pgdx, bidFn);
        }
        {
            gxfile::TGXFileObj pgdx {ErrMsg};
            double actualValue = extractValueForDemExchangeRate(pgdx, bidFn);
            REQUIRE_EQ(expectedValue, actualValue);
        }
    }

    TEST_CASE("Test setting trace level") {
        std::string f1 {"trace_wrapper.gdx"s},
                    f2 {"trace_port.gdx"s};
        testMatchingWrites(f1, f2, [&](GDXInterface &pgx) {
            REQUIRE(pgx.gdxSetTraceLevel((int)gxfile::TGXFileObj::TraceLevels::trl_all, "tracestr"s));
            REQUIRE(pgx.gdxUELRegisterRawStart());
            REQUIRE(pgx.gdxUELRegisterRaw("TheOnlyUEL"));
            REQUIRE(pgx.gdxUELRegisterDone());
            REQUIRE(pgx.gdxDataWriteRawStart("mysym", "This is my symbol!", 1, dt_par, 0));
            int key {1};
            TgdxValues values{};
            values[GMS_VAL_LEVEL] = 3.141;
            REQUIRE(pgx.gdxDataWriteRaw(&key, values.data()));
            REQUIRE(pgx.gdxDataWriteDone());
            REQUIRE(pgx.gdxDataWriteRawStart("myscalar", "This is a scalar!", 0, dt_par, 0));
            REQUIRE(pgx.gdxDataWriteDone());
        });
        std::filesystem::remove(f1);
        std::filesystem::remove(f2);
    }

    TEST_SUITE_END();

}