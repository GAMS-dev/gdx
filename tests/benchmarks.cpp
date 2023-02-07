#include <string>
#include <array>
#include <random>
#include <chrono>
#include <memory>
#include "doctest.h"
#include "../gxfile.h"

using namespace std::literals::string_literals;

namespace tests::benchmarks {

    TEST_SUITE_BEGIN("benchmarks");

    static std::ofstream bres {"class_benchmarks.txt"s};

    struct BenchResult {
        double time;
        int64_t peakRSS;
        BenchResult(double time, int64_t peakRss) : time(time), peakRSS(peakRss) {}
        BenchResult() : time{}, peakRSS{} {}
    };

    BenchResult benchmarkFrame(const std::function<void(void)> &op) {
        const int ntries{100};
        std::array<BenchResult, ntries> results{};
        double avgTime{}, avgPeakRSS{};
        for (int k{}; k < ntries; k++) {
            auto tstart = std::chrono::high_resolution_clock::now();
            op();
            auto tend = std::chrono::high_resolution_clock::now();
            results[k] = BenchResult{std::chrono::duration<double>(tend - tstart).count(), utils::queryPeakRSS()};
            avgTime += results[k].time;
            avgPeakRSS += (double) results[k].peakRSS;
        }
        avgTime /= ntries;
        avgPeakRSS /= ntries;
        return BenchResult{avgTime, (int) std::round(avgPeakRSS)};
    };

    static std::string fbl(const std::string &s, int targetLen = 15) {
        return utils::blanks(targetLen - (int)s.length()) + s;
    }

    void benchmarkTwoClasses(const std::string &name1, const std::function<void(void)> &op1,
                             const std::string &name2, const std::function<void(void)> &op2) {
        const bool quiet{false};
        BenchResult res1 = benchmarkFrame(op1), res2 = benchmarkFrame(op2);
        if (!quiet) {
            bres << fbl(name1) << "\t\t" << res1.time << "s\t\t" << res1.peakRSS << " bytes" << std::endl;
            bres << fbl(name2) << "\t\t" << res2.time << "s\t\t" << res2.peakRSS << " bytes" << std::endl;
            bres << "Winner: " << (res1.time < res2.time ? name1 : name2) << std::endl << std::endl;
        }
    };

    // Tiny helper for random sampling
    const int defaultSeed = 23;
    template<typename T>
    class RanGen {
        std::mt19937 sgen{defaultSeed};
        std::uniform_int_distribution<T> dist;
    public:
        RanGen(int lbIncl, int ubIncl) :
            dist(lbIncl, ubIncl+1) {}
        void Reset(int seed = defaultSeed) {
            sgen.seed(seed);
        }
        T Sample() {
            return dist(sgen);
        }
    };

    /*
     * TDFilter based on boolean bit array vs. std::vector<bool>
     * -> TDFilterBoolVec vs. TDFilterLegacy
     */
    template<typename T>
    void filterTest() {
        const int n{100000};
        RanGen<int> rg{0,n};
        T obj{1, 1};
        for (int i{}; i < n; i++)
            obj.SetFilter(rg.Sample(), 1);
        int cnt{};
        rg.Reset();
        for (int i{}; i < n; i++)
            cnt += obj.InFilter(rg.Sample()) ? 1 : 0;
        REQUIRE(cnt > 0);
    }
    TEST_CASE("Benchmark filter performance (set, lookup) for boolean bit array vs. std::vector<bool> internally") {
        benchmarkTwoClasses("gdlib-filter"s, filterTest<gxfile::TDFilterLegacy>,
                "cxx-filter"s, filterTest<gxfile::TDFilterBoolVec>);
    }

    // Futher Benchmarks to conduct:
    // TODO: TUELTable (C++ hashmap) vs. TUELTableLegacy (TXStrHashList)
    // TODO: TAcronymList (vec<TAcronym>) vs. TAcronymListLegacy (TXList<TAcronym>)
    // TODO: TFilterList (vec<TDFilter*>) vs. TFilterListLegacy (TXList<TDFilter>)
    // TODO: TLinkedData vs. TLinkedDatLegacy
    // TODO: Multiple set text list variants:
    // - WrapCxxUnorderedMap<int>
    // - VecSetTextList
    // - TXStrPool<int>
    // - TXCSStrHashListImpl<int>
    // TODO: Name list variants: WrapCxxUnorderedMap<PgdxSymbRecord> vs. TXStrHashListImpl<PgdxSymbRecord>

    /*
     * TIntegerMapping (vec<int>) vs. TIntegerMappingLegacy (heap int *)
     */
    template<typename T>
    void integerMappingTest() {
        const int n{100000};
        RanGen<int> rg{0, n};
        T obj;
        for (int i{}; i < n; i++)
            obj.SetMapping(rg.Sample(), 1);
        int cnt{};
        rg.Reset();
        for (int i{}; i < n; i++)
            cnt += obj.GetMapping(rg.Sample()) ? 1 : 0;
        REQUIRE(cnt > 0);
    }
    TEST_CASE("Benchmark variants of TIntegerMapping") {
        benchmarkTwoClasses("cxx-imap"s, integerMappingTest<gxfile::TIntegerMapping>,
                "gdlib-imap"s, integerMappingTest<gxfile::TIntegerMappingLegacy>);
    }

    TEST_SUITE_END();
}
