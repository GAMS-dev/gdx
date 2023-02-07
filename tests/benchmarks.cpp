#include <string>
#include <array>
#include <random>
#include <chrono>
#include "doctest.h"
#include "../gxfile.h"

using namespace std::literals::string_literals;

namespace tests::benchmarks {

    TEST_SUITE_BEGIN("benchmarks");

    struct BenchResult {
        double time;
        int64_t peakRSS;
        BenchResult(double time, int64_t peakRss) : time(time), peakRSS(peakRss) {}
        BenchResult() : time{}, peakRSS{} {}
    };

    template<typename T>
    void filterTest() {
        std::mt19937 sgen{23};
        const int n{100000};
        std::uniform_int_distribution<int> dist(0, n - 1);
        T obj{1, 1};
        for (int i{}; i < n; i++)
            obj.SetFilter(dist(sgen), 1);
        int cnt{};
        sgen.seed(23);
        for (int i{}; i < n; i++)
            cnt += obj.InFilter(dist(sgen)) ? 1 : 0;
        REQUIRE(cnt > 0);
    }

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

    void benchmarkTwoClasses(const std::string &name1, const std::function<void(void)> &op1,
                             const std::string &name2, const std::function<void(void)> &op2) {
        const bool quiet{true};
        BenchResult res1 = benchmarkFrame(op1), res2 = benchmarkFrame(op2);
        if (!quiet) {
            std::cout << name1 << ":\nTime = " << res1.time << " peak RSS = " << res1.peakRSS << std::endl;
            std::cout << name2 << ":\nTime = " << res2.time << " peak RSS = " << res2.peakRSS << std::endl;
        }
    };

    TEST_CASE("Benchmark filter performance (set, lookup) for boolean bit array vs. std::vector<bool> internally") {
        benchmarkTwoClasses("BBA"s, filterTest<gxfile::TDFilterLegacy>, "Bool vec"s, filterTest<gxfile::TDFilterBoolVec>);
    }

    // Benchmarks to conduct:
    // Done: TDFilter based on boolean bit array vs. std::vector<bool>
    //       -> TDFilterBoolVec vs. TDFilterLegacy
    // TODO: TIntegerMapping (vec<int>) vs. TIntegerMappingLegacy (heap int *)
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

    template<typename T>
    void integerMappingTest() {
        std::mt19937 sgen{23};
        const int n{100000};
        std::uniform_int_distribution<int> dist(0, n - 1);
        T obj;
        for (int i{}; i < n; i++)
            obj.SetMapping(dist(sgen), 1);
        int cnt{};
        sgen.seed(23);
        for (int i{}; i < n; i++)
            cnt += obj.GetMapping(dist(sgen)) ? 1 : 0;
        REQUIRE(cnt > 0);
    }

    TEST_CASE("Benchmark variants of TIntegerMapping") {
        benchmarkTwoClasses("TIM"s, integerMappingTest<gxfile::TIntegerMapping>,
                "TIM Legacy"s, integerMappingTest<gxfile::TIntegerMappingLegacy>);
    }

    TEST_SUITE_END();
}
