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

        void operator/=(int scalar) {
            time /= scalar;
            peakRSS /= scalar;
        }

        void operator+=(const BenchResult& other) {
            time += other.time;
            peakRSS += other.peakRSS;
        }
    };

    // Just to silence no previous declaration warning from GCC
    BenchResult benchmarkFrame(const std::function<void(void)> &op, int ntries = 10);
    BenchResult benchmarkFrame(const std::function<void(void)> &op, int ntries) {
        std::vector<BenchResult> results(ntries);
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

    static std::string fbl(const std::string &s, int targetLen = 20) {
        return utils::blanks(targetLen - (int)s.length()) + s;
    }

    // Just to silence no previous declaration warning from GCC
    struct BenchClass {
        std::string name;
        std::function<void(void)> op;
    };

    void benchmarkNClasses(int ntries, const std::initializer_list<BenchClass> &classes);
    void benchmarkNClasses(int ntries, const std::initializer_list<BenchClass> &classes) {
        const bool quiet{false};
        std::vector<BenchResult> aggrRes(classes.size());
        std::vector<std::string> names(classes.size());
        int i{};
        for(const auto &clz : classes) {
            aggrRes[i] = benchmarkFrame(clz.op, ntries);
            if(!quiet)
                bres << fbl(clz.name) << "\t\t" << aggrRes[i].time << "s\t\t" << aggrRes[i].peakRSS << " bytes" << std::endl;
            names[i++] = clz.name;
        }
        double minTime {std::numeric_limits<double>::max()};
        int winnerIx {-1};
        for(int j{}; j<(int)aggrRes.size(); j++) {
            const BenchResult &br = aggrRes[j];
            if (br.time < minTime) {
                minTime = br.time;
                winnerIx = j;
            }
        }
        if(!quiet)
            bres << "Winner: " << names[winnerIx] << std::endl << std::endl;
    }

    void benchmarkTwoClasses(const std::string &name1, const std::function<void(void)> &op1,
                             const std::string &name2, const std::function<void(void)> &op2,
                             int ntries = 10);
    void benchmarkTwoClasses(const std::string &name1, const std::function<void(void)> &op1,
                             const std::string &name2, const std::function<void(void)> &op2,
                             int ntries) {
        benchmarkNClasses(ntries, {BenchClass{name1, op1}, BenchClass{name2, op2}});
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

    // TAcronymList (vec<TAcronym>) vs. TAcronymListLegacy (TXList<TAcronym>)
    template<typename T>
    void acronymListTest() {
        const int n{ 100 };
        T obj;
        for (int i{}; i<n; i++)
            obj.AddEntry("acr" + std::to_string(i), "AcronymText" + std::to_string(i), i+1);
        int cnt{};
        for (int i{n-1}; i>=0; i--)
            cnt += obj.FindName("acr" + std::to_string(i));
    }

    TEST_CASE("Benchmark variants of TAcronymList") {
        benchmarkTwoClasses("cxx-acro"s, acronymListTest<gxfile::TAcronymList>,
                "gdlib-acro"s, acronymListTest<gxfile::TAcronymListLegacy>);
    }

    // TFilterList (vec<TDFilter*>) vs. TFilterListLegacy (TXList<TDFilter>)
    template<typename T>
    void filterListTest() {
        const int n{ 1000 };
        T obj;
        for (int i{}; i < n; i++)
            obj.AddFilter(new gxfile::TDFilter{ i, 0 });
        int cnt{};
        for (int i{ n - 1 }; i >= 0; i--)
            cnt += obj.FindFilter(i)->FiltNumber;
    }

    TEST_CASE("Benchmark variants of TAcronymList") {
        benchmarkTwoClasses("cxx-filterlist"s, filterListTest<gxfile::TFilterList>,
            "gdlib-filterlist"s, filterListTest<gxfile::TFilterListLegacy>);
    }

    // Name list variants: WrapCxxUnorderedMap<PgdxSymbRecord> vs. TXStrHashListImpl<PgdxSymbRecord>
    template<typename T>
    void nameListTest() {
        const int n{ 1000 };
        T obj;
        for (int i{}; i < n; i++)
            obj.AddObject("sym" + std::to_string(i), new gxfile::TgdxSymbRecord{});
        int cnt{};
        for (int i{ n - 1 }; i >= 0; i--)
            cnt += (*obj.GetObject(i))->SSyNr;
    }

    TEST_CASE("Benchmark variants of TAcronymList") {
        benchmarkTwoClasses("cxx-namelist"s, nameListTest<gxfile::WrapCxxUnorderedMap<gxfile::PgdxSymbRecord>>,
            "gdlib-namelist"s, nameListTest<gxfile::TXStrHashListImpl<gxfile::PgdxSymbRecord>>);
    }

    // TUELTable (C++ hashmap) vs. TUELTableLegacy (TXStrHashList)
    template<typename T>
    void uelTableTest() {
        const int n{ 1000 };
        T obj;
        for (int i{}; i < n; i++)
            obj.AddObject("uel" + std::to_string(i), i);
        int cnt{};
        for (int i{ n - 1 }; i >= 0; i--)
            cnt += obj.IndexOf("uel" + std::to_string(i));
    }

    TEST_CASE("Benchmark variants of TAcronymList") {
        benchmarkTwoClasses("cxx-ueltbl"s, uelTableTest<gxfile::TUELTable>, "gdlib-ueltbl"s, uelTableTest<gxfile::TUELTableLegacy>);
    }

    // TLinkedData vs. TLinkedDatLegacy
    template<typename T>
    class LinkedDataTest {
        static const int nkeys{ 3 }, nvals{ 2 }, n{ 100000 };
        std::array<int, nkeys> keys{};
        std::array<double, nvals> values{};
        void linkedDataIterate(T& obj) {}
    public:
        void linkedDataTest() {
            T obj{ nkeys, sizeof(double) * nvals };
            for (int i{}; i < n; i++) {
                keys.front() = i;
                values.front() = 3.142;
                obj.AddItem(keys.data(), values.data());
            }
            linkedDataIterate(obj);
        }
    };

    template<>
    void LinkedDataTest<gdlib::datastorage::TLinkedData<int, double>>::linkedDataIterate(gdlib::datastorage::TLinkedData<int, double>& obj) {
        auto it = obj.StartRead();
        for (int i{}; i < n; i++)
            obj.GetNextRecord(it, keys.data(), values.data());
    }

    template<>
    void LinkedDataTest<gdlib::datastorage::TLinkedDataLegacy<int, double>>::linkedDataIterate(gdlib::datastorage::TLinkedDataLegacy<int, double>& obj) {
        auto it = obj.StartRead();
        for (int i{}; i < n; i++)
            obj.GetNextRecord(&*it, keys.data(), values.data());
    }

    template<typename T>
    void linkedDataTest() {
        LinkedDataTest<T> ldt;
        ldt.linkedDataTest();
    }

    TEST_CASE("Benchmark variants of linked data") {
        benchmarkTwoClasses("cxx-tld"s, linkedDataTest<gdlib::datastorage::TLinkedData<int,double>>,
                            "gdlib-tld"s, linkedDataTest<gdlib::datastorage::TLinkedDataLegacy<int,double>>);
    }

    template<typename T>
    void testSetTextList() {
        T obj;
        const int n{10000};
        obj.SetCapacity(n);
        for(int i{}; i<n; i++)
            obj.Add("set_text_"+std::to_string(i+1));
        std::string s;
        for(int i{}; i<n; i++) {
            const char *cs = obj.GetString(i);
        }
    }

    TEST_CASE("Benchmark various set text list implementations") {
        benchmarkNClasses(10, {
            BenchClass {"VecSetList"s, testSetTextList<gxfile::VecSetTextList>},
            BenchClass {"WrapCxxUnorderedMap", testSetTextList<gxfile::WrapCxxUnorderedMap<int>>},
            BenchClass {"TXStrPool", testSetTextList<gdlib::gmsobj::TXStrPool<int>>},
            BenchClass {"TXCSStrHashListImpl", testSetTextList<gxfile::TXStrHashListImpl<int*>>}
        });
    }

    TEST_SUITE_END();
}
