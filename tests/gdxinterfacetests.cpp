#include <filesystem>
#include "doctest.h"
#include "../xpwrap.h"
#include "../gxfile.h"
#include "../utils.h"

using namespace std::literals::string_literals;

namespace tests::gdxinterfacetests {
    TEST_SUITE_BEGIN("GDX interface tests");

    //static std::ostream &mycout = std::cout;
    static std::ostream mycout {&gxfile::null_buffer};

    auto getBuilders() {
        static std::string ErrMsg;
        std::list<std::function<gdxinterface::GDXInterface*()>> builders {
                [&]() { return new xpwrap::GDXFile{ErrMsg}; },
                [&]() { return new gxfile::TGXFileObj{ErrMsg}; }
        };
        return builders;
    }

    void basicTest(const std::function<void(gdxinterface::GDXInterface&)> &cb) {
        for(const auto &builder : getBuilders()) {
            gdxinterface::GDXInterface *pgx = builder();
            cb(*pgx);
            delete pgx;
        }
    }

    TEST_CASE("Simple setup and teardown of a GDX object") {
        basicTest([](gdxinterface::GDXInterface &pgx) {
            REQUIRE_EQ(0, pgx.gdxErrorCount());
        });
    }

    TEST_CASE("Just create a file") {
        const std::string fn {"create.gdx"};
        basicTest([&](gdxinterface::GDXInterface &pgx) {
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
        basicTest([&](gdxinterface::GDXInterface &pgx) {
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
        basicTest([&](gdxinterface::GDXInterface &pgx) {
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
        basicTest([&](gdxinterface::GDXInterface &pgx) {
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
                     const std::function<void(gdxinterface::GDXInterface&)> &cb,
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
                            const std::function<void(gdxinterface::GDXInterface&)> &cb,
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
        testMatchingWrites(f1, f2, [](gdxinterface::GDXInterface &pgx) {
            REQUIRE(pgx.gdxUELRegisterRawStart());
            REQUIRE(pgx.gdxUELRegisterRaw("New-York"));
            REQUIRE(pgx.gdxUELRegisterDone());
        });
        testReads(f1, f2, [](gdxinterface::GDXInterface &pgx) {
            int uelCnt, highMap, uelMap;
            std::string uel;
            REQUIRE(pgx.gdxUMUelInfo(uelCnt, highMap));
            REQUIRE_EQ(0, highMap);
            REQUIRE_EQ(1, uelCnt);
            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));
            REQUIRE_EQ("New-York", uel);
            REQUIRE_EQ(-1, uelMap);
            REQUIRE_FALSE(pgx.gdxGetUEL(1, uel));
            REQUIRE_NE("New-York", uel);
        });
        for (const auto& fn : filenames)
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test adding uels (string mode)") {
        std::array filenames{ "uel_str_wrapper.gdx"s, "uel_str_port.gdx"s };
        auto [f1, f2] = filenames;
        testMatchingWrites(f1, f2, [](gdxinterface::GDXInterface& pgx) {
            REQUIRE(pgx.gdxUELRegisterStrStart());
            int uelNr;
            REQUIRE(pgx.gdxUELRegisterStr("TheOnlyUEL", uelNr));
            REQUIRE_EQ(1, uelNr);
            REQUIRE(pgx.gdxUELRegisterDone());
        });
        testReads(f1, f2, [](gdxinterface::GDXInterface& pgx) {
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
        testMatchingWrites(f1, f2, [](gdxinterface::GDXInterface& pgx) {
            REQUIRE(pgx.gdxUELRegisterMapStart());
            REQUIRE(pgx.gdxUELRegisterMap(3, "TheOnlyUEL"));
            REQUIRE(pgx.gdxUELRegisterDone());
        });
        testReads(f1, f2, [](gdxinterface::GDXInterface& pgx) {
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
        gxdefs::TgdxUELIndex keys{};
        gxdefs::TgdxValues values{};
        testMatchingWrites(f1, f2, [&](gdxinterface::GDXInterface &pgx) {
            REQUIRE(pgx.gdxUELRegisterRawStart());
            REQUIRE(pgx.gdxUELRegisterRaw("TheOnlyUEL"));
            REQUIRE(pgx.gdxUELRegisterDone());
            REQUIRE(pgx.gdxDataWriteRawStart("mysym", "This is my symbol!", 1, global::gmsspecs::gms_dt_par, 0));
            keys[0] = 1;
            values[global::gmsspecs::vallevel] = 3.141;
            REQUIRE(pgx.gdxDataWriteRaw(keys, values));
            REQUIRE(pgx.gdxDataWriteDone());
        });
        testReads(f1, f2, [&](gdxinterface::GDXInterface &pgx) {
            std::string uel;
            int uelMap;
            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));
            REQUIRE_EQ("TheOnlyUEL", uel);
            REQUIRE_EQ(-1, uelMap);
            int NrRecs;
            REQUIRE(pgx.gdxDataReadRawStart(1, NrRecs));
            REQUIRE_EQ(1, NrRecs);
            int dimFrst;
            REQUIRE(pgx.gdxDataReadRaw(keys, values, dimFrst));
            REQUIRE(pgx.gdxDataReadDone());
            REQUIRE_EQ(1, keys[0]);
            REQUIRE_EQ(3.141, values[global::gmsspecs::vallevel]);
        });
        for (const auto& fn : {f1, f2})
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test write and read record in string mode") {
        std::string f1{"rwrecordstr_wrapper.gdx"}, f2{"rwrecordstr_port.gdx"};
        gxdefs::TgdxStrIndex keyNames{};
        gxdefs::TgdxValues values{};
        testMatchingWrites(f1, f2, [&](gdxinterface::GDXInterface &pgx) {
            REQUIRE(pgx.gdxDataWriteStrStart("mysym", "This is my symbol!", 1, global::gmsspecs::gms_dt_par, 0));
            keyNames[0] = "TheOnlyUEL";
            values[global::gmsspecs::vallevel] = 3.141;
            REQUIRE(pgx.gdxDataWriteStr(keyNames, values));
            REQUIRE(pgx.gdxDataWriteDone());
        });
        testReads(f1, f2, [&](gdxinterface::GDXInterface &pgx) {
            int NrRecs;
            REQUIRE(pgx.gdxDataReadStrStart(1, NrRecs));
            REQUIRE_EQ(1, NrRecs);
            int dimFrst;
            REQUIRE(pgx.gdxDataReadStr(keyNames, values, dimFrst));
            REQUIRE(pgx.gdxDataReadDone());
            REQUIRE_EQ("TheOnlyUEL", keyNames[0]);
            REQUIRE_EQ(3.141, values[global::gmsspecs::vallevel]);
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
        basicTest([&](gdxinterface::GDXInterface &pgx) {
            gxdefs::TgdxSVals moddedSpecVals, queriedSpecVals;
            pgx.gdxGetSpecialValues(moddedSpecVals);
            moddedSpecVals[gxfile::TgdxIntlValTyp::vm_valpin] = 0.0;
            pgx.gdxSetSpecialValues(moddedSpecVals);
            pgx.gdxGetSpecialValues(queriedSpecVals);
            REQUIRE_EQ(0.0, queriedSpecVals[gxfile::TgdxIntlValTyp::vm_valpin]);
        });
    }

    TEST_CASE("Test writing mapped records out of order") {
        std::string f1{ "mapped_outoforder_wrapper.gdx" }, f2 {"mapped_outoforder_port.gdx"};
        gxdefs::TgdxUELIndex  keys{};
        gxdefs::TgdxValues values{};
        testMatchingWrites(f1, f2, [&](gdxinterface::GDXInterface &pgx) {
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

            REQUIRE(pgx.gdxDataWriteMapStart("mysym", "This is my symbol!", 1, global::gmsspecs::gms_dt_par, 0));
            for(const auto ix : randomOrder) {
                keys[0] = ix;
                values[global::gmsspecs::vallevel] = 3.141;
                REQUIRE(pgx.gdxDataWriteMap(keys, values));
            }
            REQUIRE(pgx.gdxDataWriteDone());
            REQUIRE_EQ(0, pgx.gdxErrorCount());
            REQUIRE_EQ(0, pgx.gdxDataErrorCount());
        });
        for (const auto& fn : { f1, f2 })
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test write and read record mapped") {
        std::string f1{ "rwrecordmapped_wrapper.gdx" }, f2 {"rwrecordmapped_port.gdx"};
        gxdefs::TgdxUELIndex  keys{};
        gxdefs::TgdxValues values{};
        testMatchingWrites(f1, f2, [&](gdxinterface::GDXInterface &pgx) {
            int uelCnt, highMap;
            REQUIRE(pgx.gdxUMUelInfo(uelCnt, highMap));
            REQUIRE_EQ(0, uelCnt);
            REQUIRE_EQ(0, highMap);
            REQUIRE(pgx.gdxUELRegisterMapStart());
            REQUIRE(pgx.gdxUELRegisterMap(3, "TheOnlyUEL"));
            REQUIRE(pgx.gdxUELRegisterDone());
            std::string uel;
            REQUIRE(pgx.gdxGetUEL(3, uel));
            REQUIRE_EQ("TheOnlyUEL", uel);
            REQUIRE(pgx.gdxDataWriteMapStart("mysym", "This is my symbol!", 1, global::gmsspecs::gms_dt_par, 0));
            keys[0] = 3;
            values[global::gmsspecs::vallevel] = 3.141;
            REQUIRE(pgx.gdxDataWriteMap(keys, values));
            REQUIRE(pgx.gdxDataWriteDone());
            REQUIRE_EQ(0, pgx.gdxErrorCount());
            REQUIRE_EQ(0, pgx.gdxDataErrorCount());
        });
        testReads(f1, f2, [&](gdxinterface::GDXInterface& pgx) {
            std::string uel;
            int uelMap;
            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));
            REQUIRE_EQ("TheOnlyUEL", uel);
            REQUIRE_EQ(-1, uelMap);

            REQUIRE(pgx.gdxUELRegisterMapStart());
            REQUIRE(pgx.gdxUELRegisterMap(3, "TheOnlyUEL"));
            REQUIRE(pgx.gdxUELRegisterDone());

            REQUIRE(pgx.gdxGetUEL(3, uel));
            REQUIRE_EQ("TheOnlyUEL", uel);

            REQUIRE(pgx.gdxUMUelGet(1, uel, uelMap));            
            REQUIRE_EQ("TheOnlyUEL", uel);
            REQUIRE_EQ(3, uelMap);

            int NrRecs;
            REQUIRE(pgx.gdxDataReadMapStart(1, NrRecs));
            REQUIRE_EQ(1, NrRecs);
            int dimFrst;

            REQUIRE(pgx.gdxDataReadMap(1, keys, values, dimFrst));
            REQUIRE_EQ(0, pgx.gdxDataErrorCount());

            REQUIRE_EQ(3, keys[0]);
            REQUIRE_EQ(3.141, values[global::gmsspecs::vallevel]);
            REQUIRE_EQ(1, dimFrst);
            REQUIRE(pgx.gdxDataReadDone());
        });
        for (const auto& fn : { f1, f2 })
            std::filesystem::remove(fn);
    }

    void domainSetGetTestSetupPrefix(gdxinterface::GDXInterface &pgx) {
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
        REQUIRE(pgx.gdxSystemInfo(numSyms, numUels));
        REQUIRE_EQ(2, numSyms);

        REQUIRE(pgx.gdxDataWriteStrStart("newd", "Same domain as d", 2, global::gmsspecs::dt_par, 0));
    }

    void domainSetGetTestSetupSuffix(gdxinterface::GDXInterface &pgx) {
        int numSyms, numUels;
        REQUIRE(pgx.gdxDataWriteDone());
        REQUIRE(pgx.gdxSystemInfo(numSyms, numUels));
        REQUIRE_EQ(3, numSyms);

        int newSymNr;
        REQUIRE(pgx.gdxFindSymbol("newd", newSymNr));
        REQUIRE_EQ(3, newSymNr);
    }

    void testSetGetDomainX(gdxinterface::GDXInterface &pgx) {
        gxdefs::TgdxStrIndex newSymDomainNames{"i", "j"}, domainIds;
        REQUIRE(pgx.gdxSymbolSetDomainX(3, newSymDomainNames));
        REQUIRE(pgx.gdxSymbolGetDomainX(3, domainIds));
        REQUIRE_EQ("i", domainIds[0]);
        REQUIRE_EQ("j", domainIds[1]);
    }

    TEST_CASE("Test writing with set/get domain normal and variant") {
        const std::array<std::string, 4> fns {
                "setgetdomainx_wrap.gdx"s,
                "setgetdomainx_port.gdx"s,
                "setgetdomain_wrap.gdx"s,
                "setgetdomain_port.gdx"s,
        };

        testMatchingWrites(fns[0], fns[1], [&](gdxinterface::GDXInterface &pgx) {
            domainSetGetTestSetupPrefix(pgx);
            domainSetGetTestSetupSuffix(pgx);
            testSetGetDomainX(pgx);
        });
        testMatchingWrites(fns[2], fns[3], [&](gdxinterface::GDXInterface &pgx) {
            domainSetGetTestSetupPrefix(pgx);
            gxdefs::TgdxStrIndex newSymDomainNames{"i", "j"};
            REQUIRE(pgx.gdxSymbolSetDomain(newSymDomainNames));
            domainSetGetTestSetupSuffix(pgx);
            gxdefs::TgdxUELIndex domainSyNrs;
            REQUIRE(pgx.gdxSymbolGetDomain(3, domainSyNrs));
            REQUIRE_EQ(1, domainSyNrs[0]);
            REQUIRE_EQ(2, domainSyNrs[1]);
        });

        for (const auto &fn: fns)
            std::filesystem::remove(fn);
    }

    TEST_CASE("Test adding a set alias") {
        const std::string f1 {"addalias_wrap.gdx"}, f2 {"addalias_port.gdx"};
        testMatchingWrites(f1, f2, [&](gdxinterface::GDXInterface &pgx) {
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
        testMatchingWrites(f1, f2, [&](gdxinterface::GDXInterface& pgx) {
            REQUIRE(pgx.gdxUELRegisterRawStart());
            REQUIRE(pgx.gdxUELRegisterRaw("onlyuel"));
            REQUIRE(pgx.gdxUELRegisterDone());
            REQUIRE(pgx.gdxDataWriteRawStart("i", "expl", 1, global::gmsspecs::gms_dt_set, 0));
            gxdefs::TgdxUELIndex keys;
            gxdefs::TgdxValues values;
            keys[0] = 1;
            values[global::gmsspecs::tvarvaltype::vallevel] = 1;
            REQUIRE(pgx.gdxDataWriteRaw(keys, values));
            REQUIRE(pgx.gdxDataWriteDone());
            int txtNr;
            REQUIRE(pgx.gdxAddSetText("set text", txtNr));
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
        gxdefs::TgdxUELIndex keys{};
        gxdefs::TgdxValues values{};
        basicTest([&](gdxinterface::GDXInterface& pgx) {
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
            keys[0] = 3;
            REQUIRE(pgx.gdxDataWriteRaw(keys, values));
            keys[0] = 1;
            REQUIRE_FALSE(pgx.gdxDataWriteRaw(keys, values));
            REQUIRE(pgx.gdxDataWriteDone());
            REQUIRE_EQ(1, pgx.gdxErrorCount());
            REQUIRE_EQ(1, pgx.gdxDataErrorCount());
            REQUIRE(pgx.gdxDataErrorRecord(1, keys, values));
            REQUIRE_EQ(1, keys.front());
            REQUIRE(pgx.gdxDataErrorRecordX(1, keys, values));
            REQUIRE_EQ(1, keys.front());
            int ec = pgx.gdxGetLastError();
            std::string errMsg;
            REQUIRE(pgx.gdxErrorStr(ec, errMsg));
            REQUIRE_EQ("Data not sorted when writing raw", errMsg);
            pgx.gdxClose();
        });
        std::filesystem::remove(fn);
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
        const std::string   model = "trnsport"s,
                            model_fn = model + ".gms"s,
                            fnpf = "trans_data"s,
                            gdxfn = fnpf+".gdx"s;
        std::system(("gamslib "s+model+" > gamslibLog.txt"s).c_str());
        std::filesystem::remove("gamslibLog.txt");
        REQUIRE(std::filesystem::exists(model_fn));
        std::system(("gams trnsport gdx="s + fnpf + " lo=0 o=lf > trnsportLog.txt").c_str());
        std::filesystem::remove("trnsportLog.txt");
        std::filesystem::remove(model_fn);
        std::filesystem::remove("lf");
        REQUIRE(std::filesystem::exists(gdxfn));
        testReads(gdxfn, gdxfn, [&](gdxinterface::GDXInterface &pgx) {
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
                    gxdefs::TgdxStrIndex keyNames;
                    gxdefs::TgdxValues values;
                    int dimFirst;
                    for(int j{}; j<numRecords; j++) {
                        REQUIRE(pgx.gdxDataReadStr(keyNames, values, dimFirst));
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
            gxdefs::TgdxUELIndex domainSyNrs;
            REQUIRE(pgx.gdxSymbolGetDomain(5, domainSyNrs));
            REQUIRE_EQ(2, pgx.gdxSymbolDim(5));
            // first domain of d(i,j) is i (symbol 1)
            REQUIRE_EQ(1, domainSyNrs[0]);
            // second domain of d(i,j) is j (symbol 2)
            REQUIRE_EQ(2, domainSyNrs[1]);

            gxdefs::TgdxStrIndex domainIds;
            REQUIRE(pgx.gdxSymbolGetDomainX(5, domainIds));
            REQUIRE_EQ("i", domainIds[0]);
            REQUIRE_EQ("j", domainIds[1]);
        });
        std::filesystem::remove(gdxfn);
    }

    TEST_CASE("Tests related to universe") {
        basicTest([](gdxinterface::GDXInterface &pgx) {
            int errNr;
            auto fn = "universe_tests.gdx"s;
            REQUIRE(pgx.gdxOpenWrite(fn, "gdxinterfacetests", errNr));
            REQUIRE(pgx.gdxDataWriteStrStart("i", "A set", 1, global::gmsspecs::dt_set, 0));
            gxdefs::TgdxStrIndex keys{};
            gxdefs::TgdxValues vals{};
            for(int i=1; i<=8; i++) {
                keys[0] = "uel_" + std::to_string(i);
                REQUIRE(pgx.gdxDataWriteStr(keys, vals));
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
                REQUIRE(pgx.gdxDataReadStr(keys, vals, dimFirst));
                REQUIRE_EQ("uel_"+std::to_string(i), keys.front());
            }
            REQUIRE(pgx.gdxDataReadDone());

            pgx.gdxClose();

            std::filesystem::remove(fn);
        });
    }

    TEST_CASE("Test domain checking for subset") {
        const std::string fn {"xyz.gdx"};
        basicTest([&](gdxinterface::GDXInterface &pgx) {
            int errnr;
            REQUIRE(pgx.gdxOpenWrite(fn, "gdxinterfacetests", errnr));
            REQUIRE(pgx.gdxDataWriteStrStart("i", "expl", 1, global::gmsspecs::gms_dt_set, 0));
            gxdefs::TgdxStrIndex keys{};
            gxdefs::TgdxValues vals{};
            for(int i{}; i<6; i++) {
                keys.front() = "i"s+std::to_string(i+1);
                REQUIRE(pgx.gdxDataWriteStr(keys, vals));
            }
            REQUIRE(pgx.gdxDataWriteDone());

            REQUIRE(pgx.gdxDataWriteStrStart("j", "subset of i", 1, global::gmsspecs::gms_dt_set, 0));
            keys.front() = "i";
            pgx.gdxSymbolSetDomain(keys);
            //REQUIRE(pgx.gdxSymbolSetDomain(keys));
            std::array<int, 2> subset = {2, 4};
            for(int i : subset) {
                keys.front() = "i"s+std::to_string(i);
                REQUIRE(pgx.gdxDataWriteStr(keys, vals));
            }

            // adding an uel not from superset should fail
            keys.front() = "not_in_i";
            REQUIRE(pgx.gdxDataWriteStr(keys, vals));

            REQUIRE(pgx.gdxDataWriteDone());

            REQUIRE_EQ(1, pgx.gdxErrorCount());
            std::string msg;
            REQUIRE(pgx.gdxErrorStr(pgx.gdxGetLastError(), msg));
            REQUIRE_EQ("Domain violation"s, msg);
            pgx.gdxClose();
        });
        std::filesystem::remove(fn);
    }

    TEST_CASE("Test writing a duplicate uel in string mode") {
        basicTest([](gdxinterface::GDXInterface &pgx) {
            int errNr;
            auto fn = "dup.gdx"s;
            REQUIRE(pgx.gdxOpenWrite(fn, "gdxinterfacetests", errNr));
            REQUIRE(pgx.gdxDataWriteStrStart("i", "A set", 1, global::gmsspecs::dt_set, 0));
            gxdefs::TgdxStrIndex keys{};
            gxdefs::TgdxValues vals{};
            for(int i=1; i<=8; i++) {
                keys[0] = "uel_" + std::to_string(i);
                REQUIRE(pgx.gdxDataWriteStr(keys, vals));
            }
            REQUIRE(pgx.gdxDataWriteStr(keys, vals));
            REQUIRE(pgx.gdxDataWriteDone());
            REQUIRE_EQ(1, pgx.gdxErrorCount());
            std::string msg;
            pgx.gdxErrorStr(pgx.gdxGetLastError(), msg);
            REQUIRE_EQ("Duplicate keys", msg);



            pgx.gdxClose();

            std::filesystem::remove(fn);
        });
    }

    TEST_SUITE_END();

}