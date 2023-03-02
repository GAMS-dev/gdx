#include "doctest.h"
#include "../gxfile.h"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <random>

#include "pfgdx.hpp"

using namespace gxfile;
using namespace std::literals::string_literals;

namespace tests::gxfiletests {
    TEST_SUITE_BEGIN("gxfile");

    void writeFile();
    void readFile();

    static std::ostream my_cout {&gxfile::null_buffer};

    const std::string tmp_fn {"mytest.gdx"};

    void writeFile() {
        std::string msg;
        TGXFileObj pgx{ msg };
        int ErrNr;
        pgx.gdxOpenWrite(tmp_fn.c_str(), "TGXFileObj", ErrNr);
        pgx.gdxDataWriteStrStart("Demand", "Demand data", 1, 1, 0);
        auto writeRec = [&pgx](const std::string& s, double v) {
            static std::array<std::string, 20> keys{};
            static std::array<double, 5> values{};
            keys.front() = s;
            values.front() = v;
            const char *keyptrs[] = { s.c_str() };
            pgx.gdxDataWriteStr(keyptrs, values.data());
        };
        writeRec("New-York"s, 324.0);
        writeRec("Chicago"s, 299.0);
        writeRec("Topeka"s, 274.0);
        pgx.gdxDataWriteDone();
        pgx.gdxClose();
    }

    void readFile() {
        std::string msg;
        TGXFileObj pgx{ msg };

        int ErrNr{};
        REQUIRE(pgx.gdxOpenRead(tmp_fn.c_str(), ErrNr));
        REQUIRE_EQ(0, ErrNr);
        //if (ErrNr) ReportIOError(ErrNr, "gdxOpenRead");
        char Producer[GMS_SSSIZE], FileVersion[GMS_SSSIZE];
        REQUIRE(pgx.gdxFileVersion(FileVersion, Producer));

        my_cout << "GDX file written using version: " << FileVersion << '\n';
        my_cout << "GDX file written by: " << Producer << '\n';

        int SyNr{};
        REQUIRE(pgx.gdxFindSymbol("demand", SyNr));

        int Dim{}, VarTyp{};
        char VarName[GMS_SSSIZE];
        REQUIRE(pgx.gdxSymbolInfo(SyNr, VarName, Dim, VarTyp));
        REQUIRE(Dim == 1);
        REQUIRE(dt_par == VarTyp);

        int NrRecs;
        REQUIRE(pgx.gdxDataReadStrStart(SyNr, NrRecs));
        //if (!pgx.gdxDataReadStrStart(SyNr,NrRecs)) ReportGDXError(PGX);

        my_cout << "Parameter demand has " << std::to_string(NrRecs) << " records\n";

        std::array<std::array<char, 256>, GLOBAL_MAX_INDEX_DIM> bufs {};
        std::array<char *, GLOBAL_MAX_INDEX_DIM> Indx {};

        for(int i=0; i<(int)Indx.size(); i++) {
            Indx[i] = bufs[i].data();
        }
        gdxinterface::TgdxValues Values;
        for (int N{}; pgx.gdxDataReadStr(Indx.data(), Values.data(), N);) {
            /*if (0 == Values[global::gmsspecs::vallevel]) continue;
            for (int D{}; D < Dim; D++)
                my_cout << (D ? '.' : ' ') << Indx[D];
            my_cout << " = %7.2f\n" << Values[global::gmsspecs::vallevel] << '\n';*/
            my_cout << "Key=" << Indx.front() << ", Value=" << Values[GMS_VAL_LEVEL] << "\n";
        }

        REQUIRE(pgx.gdxDataReadDone());
        REQUIRE_FALSE(pgx.gdxClose());
    }

    TEST_CASE("Test creating and reading a simple gdx file with gxfile port") {
        writeFile();
        readFile();
        std::filesystem::remove(tmp_fn);
    }

    TEST_CASE("Test integer mapping") {
        TIntegerMappingImpl mapping;
        REQUIRE_FALSE(mapping.GetHighestIndex());
        mapping.SetMapping(3, 5);
        REQUIRE_EQ(5, mapping.GetMapping(3));
        const int memused = mapping.MemoryUsed();
        REQUIRE(memused > 0);
        /*mapping.clear();
        REQUIRE(mapping.MemoryUsed() < memused);
        mapping.SetMapping(3, 5);
        REQUIRE_EQ(5, mapping[3]);*/
    }

    TEST_CASE("Test function for making a good explanatory text") {
        REQUIRE_EQ("x", MakeGoodExplText("x"s));
        REQUIRE_EQ("Das hier ist ein Test?", MakeGoodExplText("Das hier ist ein Test\r"s));
        REQUIRE_EQ("Ein \"gemischter\" \"Text\"!", MakeGoodExplText("Ein \"gemischter\" 'Text'!"s));
    }

    TEST_CASE("Test checking whether an identifier is well-formed (good)") {
        REQUIRE_FALSE(IsGoodIdent(""s));
        REQUIRE_FALSE(IsGoodIdent(std::string(100, ' ')));
        REQUIRE_FALSE(IsGoodIdent(" abc"s));
        REQUIRE_FALSE(IsGoodIdent("A!"s));

        REQUIRE(IsGoodIdent("x"s));
        REQUIRE(IsGoodIdent("abc_123"s));
        REQUIRE(IsGoodIdent("a1"s));
    }

    // Temporarily disable to prevent stdout
    /*TEST_CASE("Test check mode (indirectly)") {
        std::string errMsg;
        TGXFileObj gdx{errMsg};
        int errNr;
        const std::string tmpfn {"tmp.gdx"};
        REQUIRE(gdx.gdxOpenWrite(tmpfn, "gxfiletest", errNr));
        REQUIRE_FALSE(gdx.gdxDataWriteRaw(nullptr, nullptr));
        gdx.gdxClose();
        std::filesystem::remove(tmpfn);
    }*/

    TEST_CASE("Test conversion of GDX files using gdxcopy utility") {
        auto rmfiles = [](const std::initializer_list<std::string> &fns) {
            for(auto &fn : fns)
                std::filesystem::remove(fn);
        };
        int rc{};
        rc = system("gamslib trnsport > gamslibLog.txt");
        std::filesystem::remove("gamslibLog.txt");
        REQUIRE_EQ(0, rc);
        rc = system("gams trnsport a=c gdx=trnsport lo=0 o=lf > log.txt");
        REQUIRE_EQ(0, rc);
        rtl::p3utils::P3SetEnv("GDXCOMPRESS", "1"s);
        rtl::p3utils::P3SetEnv("GDXCONVERT", "V5");
        REQUIRE_EQ(0, gxfile::ConvertGDXFile("trnsport.gdx"s, "U"s));
        rtl::p3utils::P3UnSetEnv("GDXCOMPRESS"s);
        rtl::p3utils::P3UnSetEnv("GDXCONVERT"s);
        rmfiles({"trnsport.gms", "trnsport.gdx", "log.txt", "lf.txt"});
    }

    void runBenchmarkTimeMemForGDXFile(const std::string& suiteName, const std::string& modelName, const std::string& fn);

    enum class BatchBenchModes {
        SUITE_MODEL_PAIRS,
        ALL_SUITES,
        SINGLE_SUITE
    };

    void batchRunBenchmarks(BatchBenchModes mode);

    TEST_CASE("Run pfgdx for suiteName/modelName.gdx in order to debug memory issues (and test pfgdx port)") {
#ifdef NO_SLOW_TESTS
        return;
#endif
        batchRunBenchmarks(BatchBenchModes::SUITE_MODEL_PAIRS);
    }

    static const std::string benchOutFileName{ "gdxbench_collected.txt"s };

    std::list<std::string> gdxFilesInPath(const std::string& path);
    std::list<std::string> gdxFilesInPath(const std::string& path) {
#ifdef __INTEL_COMPILER
        return {};
#else
        std::list<std::string> res{};
        std::map<std::string, uintmax_t> cachedFileSizes {};
        for (const auto& entry : std::filesystem::directory_iterator{ path }) {
            const auto fn = entry.path().filename().string();
            if (utils::ends_with(fn, ".gdx")) {
                auto name = fn.substr(0, fn.size() - 4);
                res.push_back(name);
                cachedFileSizes[name] = std::filesystem::file_size(std::filesystem::path{path+"/"s+name+".gdx"s});
            }
        }
        res.sort([&](const std::string &a, const std::string &b) {
            return cachedFileSizes[a] < cachedFileSizes[b];
        });
        return res;
#endif
    }

    std::list<std::string> extractModelNames(const std::string &textFilename);
    std::list<std::string> extractModelNames(const std::string &textFilename) {
        std::list<std::string> res{};
        std::ifstream ifs{textFilename};
        std::string line;
        while(!ifs.eof()) {
            std::getline(ifs, line);
            if(line.length() > 0 && line.front() != ' ')
                res.push_back(line);
        }
        return res;
    }

    void batchRunBenchmarks(BatchBenchModes mode) {
        std::list<std::string> alreadyKnownResults;
        if(std::filesystem::exists(benchOutFileName)) {
            alreadyKnownResults = extractModelNames(benchOutFileName);
        }

        std::array suites{
            "sqagams"s, "lwsup"s, "src"s, "mrb"s, "apc"s
        };

        std::vector<std::pair<std::string, std::string>> suiteModelPairs{
            //{"sqagams","PERSEUS"}
            {"src", "ex3"}
            //{"lwsup", "test1"}
            //{"src", "2"}
            //{"lwsup","agg"}
            //{"src", "gdxfromutlity"},
            //{"sqagams","mpstrans"},
            //{"sqagams","catmix"}
        };

        std::string singleSuiteName{ "apc"s };

        auto gdxFilePathCandidates = [](const std::string& suiteName, const std::string& modelName) {
#if defined(_WIN32)
            return std::array{
                modelName + ".gdx"s,
                suiteName + "\\"s + modelName + ".gdx"s,
                R"(C:\dockerhome\)" + suiteName + "\\"s + modelName + ".gdx"s
            };
#else
            return std::array{
                modelName + ".gdx"s,
                suiteName + "\\"s + modelName + ".gdx"s,
                "/mnt/c/dockerhome/"s + suiteName + "/"s + modelName + ".gdx"s,
                "/home/andre/dockerhome/distrib/"s + suiteName + "/"s + modelName + ".gdx"s
            };
#endif
        };

        auto runBenchmarkForSuiteModel = [&](const std::string& suiteName, const std::string& modelName) {
            for (const auto& fn : gdxFilePathCandidates(suiteName, modelName))
                if (std::filesystem::exists(fn) && std::find(alreadyKnownResults.begin(), alreadyKnownResults.end(), modelName) == alreadyKnownResults.end())
                    runBenchmarkTimeMemForGDXFile(suiteName, modelName, fn);
        };

        std::string suitePrefixPath;
#if defined(_WIN32)
        suitePrefixPath = R"(C:\dockerhome\)";
#else
        //suitePrefixPath = "/mnt/c/dockerhome/";
        suitePrefixPath = "/home/andre/dockerhome/distrib/"s;
#endif

        switch (mode) {
        case BatchBenchModes::ALL_SUITES:
            for (const auto& suite : suites)
                for (const auto& modelName : gdxFilesInPath(suitePrefixPath + suite))
                    runBenchmarkForSuiteModel(suite, modelName);
            break;
        case BatchBenchModes::SINGLE_SUITE:
            for (const auto& modelName : gdxFilesInPath(suitePrefixPath + singleSuiteName))
                runBenchmarkForSuiteModel(singleSuiteName, modelName);
            break;
        case BatchBenchModes::SUITE_MODEL_PAIRS:
            for (const auto& [suiteName, modelName] : suiteModelPairs)
                runBenchmarkForSuiteModel(suiteName, modelName);
            break;
        }
    }

    std::vector<std::string> skiplist {
        "5062.gdx"s,
        "data1000.gdx"s,
        "data10000.gdx"s,
        "Modellendogen0004.gdx"s,
        "Modellendogen0005.gdx"s,
        "Modellendogen0725.gdx"s,
        "Modellendogen0726.gdx"s,
        "Modellendogen1093.gdx"s,
        "Modellendogen1407.gdx"s,
        "output2.gdx"s,
        "output_DisEmp.gdx"s,
        "preFix.gdx"s,
        "soln_Toroptimierung_2_p1.gdx"s,
        "Toroptimierung_2_p.gdx"s,
        "Toroptimierung_2_p - Copy.gdx"s,
        "z.gdx"s,
        "data_m5.gdx"s,
        "dbX2370.gdx"s, "dbX2372.gdx"s,
        "gmsgrid.gdx"s,
        "testInstance1000Ite.gdx"s,
        "testInstance100Ite.gdx"s,
        "testInstance200Ite.gdx"s,
        "x0.gdx"s,
        "x1.gdx"s,
        "x170.gdx"s,
        "x170postfix.gdx"s,
        "x171.gdx"s,
        "test.gdx"s,
        "gdxdump.gdx"s,
        "map.gdx"s,
        "new4.gdx"s,
        "food_man.gdx"s,
        "testExcel.gdx"s,
        "ELMOD.gdx"s,
        "JMM.gdx"s,
        "arauco.gdx"s,
        "modelo.gdx"s,
        "PERSEUS.gdx"
        "AssignmentBug.gdx"s,
        "globiom.gdx"s,
        "indata.gdx"s,
        "seders.gdx"s,
        "CHPSystemData2_Converted.gdx"s,
        "pegase.gdx"s,
        "getdata.gdx"s,
        "rank_out.gdx"s,
        "rank_output.gdx"s,
        "bau_p.gdx"
        "validnlpecopts.gdx"s,
        "xyz.gdx"
    };

    void runBenchmarkTimeMemForGDXFile(const std::string &suiteName, const std::string &modelName, const std::string &fn) {
#if defined(_WIN32)
        const char sep { '\\' };
#else
        const char sep{ '/' };
#endif
        auto fnParts = utils::split(fn, sep);
        auto filenamePart = fnParts.back();
        auto filenameStem = filenamePart.substr(0, filenamePart.size() - 4);

        if (std::any_of(skiplist.begin(), skiplist.end(),
            [&](const std::string& s) { return s == filenamePart; }))
            return;

        const int ntries = 4;
        const bool  quiet = false,
                onlyPorted = false,
                onlyWrapped = false;

        pfgdx::TimeTriple tWrap, tPort;
        std::vector<double> slowdowns {};
        double totWrap{}, totPort{};

        std::ofstream textout {benchOutFileName, std::ios_base::app};

        for (int i = 0; i < ntries; i++) {
            int64_t peakRSS;
            if (!onlyPorted) {
                tWrap = pfgdx::runWithTiming(fn, true, quiet);
                totWrap += tWrap.total_t;
                peakRSS = utils::queryPeakRSS();
                if(!quiet) std::cout << "Peak RSS after wrapped GDX (P3/Delphi): "s << peakRSS << '\n';
            }
            if (!onlyWrapped) {
                tPort = pfgdx::runWithTiming(fn, false, quiet);
                totPort += tPort.total_t;
                if(!onlyPorted) {
                    auto newPeakRSS = utils::queryPeakRSS();
                    if(!quiet) std::cout << "Peak RSS after both wrapped and ported GDX: "s << peakRSS << '\n';
                    if (newPeakRSS > peakRSS) {
                        if(!quiet)
                            std::cout << "Warning: Peak RSS increase by "s << newPeakRSS - peakRSS << " bytes ("s
                                      << ((double)newPeakRSS / (double)peakRSS - 1.0) * 100.0 << "%)\n"s;
                    }
                    REQUIRE_LE(newPeakRSS, peakRSS); // peak rss should not increase after running C++ GDX
                }
            }

            slowdowns.push_back(tWrap.total_t > 0 ? tPort.total_t / tWrap.total_t : 0);
            if (!quiet && !onlyPorted && !onlyWrapped)
                std::cout << "Slowdown for "s << fn << " = "s << slowdowns[i] << '\n';
            if(slowdowns[i] <= 1.05) break; // we are happy if we got at least one try with slowdown below threshold 5%
        }
        if (!quiet) {
            auto avgSlowdown = std::accumulate(slowdowns.begin(), slowdowns.end(), 0.0) / (double)slowdowns.size();
            auto minSlowdown = *std::min_element(slowdowns.begin(), slowdowns.end());
            std::cout << "Average slowdown = "s << avgSlowdown << '\n';
            std::cout << "Minimum slowdown = "s << minSlowdown << '\n';
            std::cout << "Total runtime wrapped = "s << totWrap << '\n';
            std::cout << "Total runtime ported = "s << totPort << '\n';
            textout << suiteName << "/"s << modelName << ";"s << minSlowdown << std::endl;
        }
    }

    TEST_SUITE_END();
}