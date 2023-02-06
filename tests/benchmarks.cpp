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

    template<class T>
    BenchResult benchmarkFilterClass() {
        const int ntries{100};
        std::array<BenchResult, ntries> results{};
        double avgTime{}, avgPeakRSS{};
        for (int k{}; k < ntries; k++) {
            std::mt19937 sgen{23};
            const int n{100000};
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
            results[k] = BenchResult{std::chrono::duration<double>(tend - tstart).count(), utils::queryPeakRSS()};
            avgTime += results[k].time;
            avgPeakRSS += (double) results[k].peakRSS;
        }
        avgTime /= ntries;
        avgPeakRSS /= ntries;
        return BenchResult{avgTime, (int) std::round(avgPeakRSS)};
    };

    TEST_CASE("Benchmark filter performance (set, lookup) for boolean bit array vs. std::vector<bool> internally") {
        const bool quiet{true};
        auto resBBA = benchmarkFilterClass<gxfile::TDFilterLegacy>();
        auto resBoolVec = benchmarkFilterClass<gxfile::TDFilterBoolVec>();
        if (!quiet) {
            std::cout << "BBA:\nTime = " << resBBA.time << " peak RSS = " << resBBA.peakRSS << std::endl;
            std::cout << "Bool vec:\nTime = " << resBoolVec.time << " peak RSS = " << resBoolVec.peakRSS << std::endl;
        }
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

    TEST_SUITE_END();
}
