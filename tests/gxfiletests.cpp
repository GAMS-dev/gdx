#include "doctest.h"
#include "../gxfile.h"
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <random>

#include "pfgdx.hpp"

// Needed for peak working set size query
#if defined(_WIN32)
#include <windows.h>
#include <Psapi.h>
#include <processthreadsapi.h>
#endif

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
        pgx.gdxOpenWrite(tmp_fn, "TGXFileObj", ErrNr);
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
        REQUIRE(pgx.gdxOpenRead(tmp_fn, ErrNr));
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
        REQUIRE_EQ("x", MakeGoodExplText("x"));
        REQUIRE_EQ("Das hier ist ein Test?", MakeGoodExplText("Das hier ist ein Test\r"));
        REQUIRE_EQ("Ein \"gemischter\" \"Text\"!", MakeGoodExplText("Ein \"gemischter\" 'Text'!"));
    }

    TEST_CASE("Test checking whether an identifier is well-formed (good)") {
        REQUIRE_FALSE(IsGoodIdent(""));
        REQUIRE_FALSE(IsGoodIdent(std::string(100, ' ')));
        REQUIRE_FALSE(IsGoodIdent(" abc"));
        REQUIRE_FALSE(IsGoodIdent("A!"));

        REQUIRE(IsGoodIdent("x"));
        REQUIRE(IsGoodIdent("abc_123"));
        REQUIRE(IsGoodIdent("a1"));
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

    static int64_t queryPeakRSS() {
#if defined(_WIN32)
        PROCESS_MEMORY_COUNTERS info;
        if(!GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info)))
            return 0;
        return (int64_t)info.PeakWorkingSetSize;
#elif defined(__linux)
        std::ifstream ifs {"/proc/self/status"};
        if(!ifs.is_open()) return 0;
        std::string line;
        while(!ifs.eof()) {
            std::getline(ifs, line);
            if(utils::starts_with(line, "VmHWM")) {
                auto parts = utils::split(line);
                return std::stoi(utils::nth(parts, 1));
            }
        }
        return 0;
#elif defined(__APPLE__)
        return 0;
#endif
    }

    void runBenchmarkTimeMemForGDXFile(const std::string &fn);

    TEST_CASE("Run pfgdx for suiteName/modelName.gdx in order to debug memory issues (and test pfgdx port)") {
#ifdef NO_SLOW_TESTS
        return;
#endif
        std::vector<std::pair<std::string, std::string>> suiteModelPairs {
            /*{"lwsup","modBig"},
            {"lwsup","modBigx"},
            {"lwsup","t4716"},*/
            {"sqagams","dummy42"},
            /*{"lwsup","10rr"},
            {"sqagams","t3010"},
            {"mrb","WSC Fixing"},
            {"lwsup","modBig8788"},
            {"lwsup","SCUCinput_new"},
            {"src","Estimate"},
            {"lwsup","pShiftFactor"},
            {"lwsup","test1"},
            {"src","fnpower"},
            {"mrb","new6"},
            {"lwsup","daod4gams"},
            {"sqagams","times_psi"},
            {"sqagams","GuaranteeData"},
            {"sqagams","bearing"}*/
        };

        auto gdxFilePathCandidates = [](const std::string &suiteName, const std::string &modelName) {
#if defined(_WIN32)
            return std::array {
                modelName + ".gdx"s,
                suiteName + "\\"s + modelName + ".gdx"s,
                R"(C:\dockerhome\)"+suiteName+"\\"s+modelName+".gdx"s
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

        for(const auto &[suiteName, modelName] : suiteModelPairs)
            for(const auto &fn : gdxFilePathCandidates(suiteName, modelName))
                if (std::filesystem::exists(fn))
                    runBenchmarkTimeMemForGDXFile(fn);
    }

    void runBenchmarkTimeMemForGDXFile(const std::string &fn) {
        const int ntries = 1;
        const bool  quiet = false,
                onlyPorted = false,
                onlyWrapped = false;

        pfgdx::TimeTriple tWrap, tPort;
        std::array<double, ntries> slowdowns {};

        for (int i = 0; i < ntries; i++) {
            int64_t peakRSS;
            if (!onlyPorted) {
                tWrap = pfgdx::runWithTiming(fn, true, quiet);
                peakRSS = queryPeakRSS();
                if(!quiet) std::cout << "Peak RSS after wrapped GDX (P3/Delphi): " << peakRSS << std::endl;
            }
            if (!onlyWrapped) {
                tPort = pfgdx::runWithTiming(fn, false, quiet);
                if(!onlyPorted) {
                    auto newPeakRSS = queryPeakRSS();
                    if(!quiet) std::cout << "Peak RSS after both wrapped and ported GDX: " << peakRSS << std::endl;
                    if (newPeakRSS > peakRSS) {
                        if(!quiet)
                            std::cout << "Warning: Peak RSS increase by " << newPeakRSS - peakRSS << " bytes ("
                                      << ((double)newPeakRSS / (double)peakRSS - 1.0) * 100.0 << "%)" << std::endl;
                    }
                    REQUIRE_LE(newPeakRSS, peakRSS); // peak rss should not increase after running C++ GDX
                }
            }

            slowdowns[i] = tWrap.total_t > 0 ? tPort.total_t / tWrap.total_t : 0;
            if (!quiet && !onlyPorted && !onlyWrapped)
                std::cout << "Slowdown for " << fn << " = " << slowdowns[i] << std::endl;
        }
        if(!quiet)
            std::cout << "Average slowdown = " << std::accumulate(slowdowns.begin(), slowdowns.end(), 0.0) / (double)ntries << std::endl;;
    }

    struct BenchResult {
        double time;
        int64_t peakRSS;
        BenchResult(double time, int64_t peakRss) : time(time), peakRSS(peakRss) {}
        BenchResult() : time {}, peakRSS {} {}
    };

    template<class T>
    BenchResult benchmarkFilterClass() {
        const int ntries {100};
        std::array<BenchResult, ntries> results{};
        double avgTime {}, avgPeakRSS {};
        for(int k{}; k<ntries; k++) {
            std::mt19937 sgen{23};
            const int n {100000};
            std::uniform_int_distribution<int> dist(0, n - 1);
            auto tstart = std::chrono::high_resolution_clock::now();
            T obj{1, 1};
            for (int i{}; i < n; i++)
                obj.SetFilter(dist(sgen), 1);
            int cnt{};
            sgen.seed(23);
            for (int i{}; i < n; i++)
                cnt += obj.InFilter(dist(sgen)) ? 1 : 0;
            auto tend = std::chrono::high_resolution_clock::now();
            REQUIRE(cnt > 0);
            results[k] = BenchResult {std::chrono::duration<double>(tend - tstart).count(), queryPeakRSS()};
            avgTime += results[k].time;
            avgPeakRSS += (double)results[k].peakRSS;
        }
        avgTime /= ntries;
        avgPeakRSS /= ntries;
        return BenchResult {avgTime, (int)std::round(avgPeakRSS)};
    };

    TEST_CASE("Benchmark filter performance (set, lookup) for boolean bit array vs. std::vector<bool> internally") {
        const bool quiet {true};
        auto resBBA = benchmarkFilterClass<TDFilterLegacy>();
        auto resBoolVec = benchmarkFilterClass<TDFilterBoolVec>();
        if(!quiet) {
            std::cout << "BBA:\nTime = " << resBBA.time << " peak RSS = " << resBBA.peakRSS << std::endl;
            std::cout << "Bool vec:\nTime = " << resBoolVec.time << " peak RSS = " << resBoolVec.peakRSS << std::endl;
        }
    }

    TEST_SUITE_END();
}