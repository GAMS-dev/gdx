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
            REQUIRE(pgx.gdxOpenWrite(fn, "xpwraptest", ErrNr));
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
                std::cout << "Found " << maybeMismatches->size() << " mismatches between " << filename1 << " and " << filename2 << "\n";
                const int truncCnt{ 10 };
                int cnt{};
                for (const auto& mm : *maybeMismatches) {
                    std::cout << "#" << mm.offset << ": " << (int)mm.lhs << " != " << (int)mm.rhs << "\n";
                    if (cnt++ >= truncCnt) {
                        std::cout << "Truncating results after " << truncCnt << " entries." << "\n";
                        break;
                    }
                }
                std::cout << std::endl;
#ifdef __APPLE__
                const std::string pythonInterpreter {"python3"};
#else
                const std::string pythonInterpreter {"python"};
#endif
                system((pythonInterpreter + " gdxtoyaml/main.py "s + filename1 + " "s + filename2).c_str());
            } else std::cout << "No mismatches found between " << filename1 << " and " << filename2 << std::endl;
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
        std::system(("gamslib "s+model).c_str());
        REQUIRE(std::filesystem::exists(model_fn));
        std::system(("gams trnsport gdx="s + fnpf + " lo=0 o=lf").c_str());
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
        });
        std::filesystem::remove(gdxfn);
    }

    TEST_SUITE_END();

}