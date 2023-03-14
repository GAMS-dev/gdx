#include <string>
#include "doctest.h"
#include "../datastorage.h"
#include <chrono>
#include <iostream>
#include <random>
#include <numeric>

using namespace std::literals::string_literals;
using namespace gdx::collections::datastorage;

namespace tests::datastoragetests {

    TEST_SUITE_BEGIN("gdlib::datastorage");

    TEST_CASE("Simple use of linked data") {
        TLinkedData<int, double> ld {1, 1*(int)sizeof(double)};
        std::vector<int> keys(1);
        std::array<double, 1> vals {23.0};
        for(int i{}; i<4; i++) {
            keys.front() = 4-i; // insert in reverse order
            auto node = ld.AddItem(keys.data(), vals.data());
            REQUIRE_EQ(keys.front(), ((int *)node->RecData)[0]);
            REQUIRE_EQ(23.0, *((double *)&node->RecData[(int)sizeof(int)]));
        }
        REQUIRE_EQ(4, ld.Count());
        auto it = ld.StartRead(); // this calls sort!
        for(int i{}; i<4; i++) {
            bool res = ld.GetNextRecord(&it.value(), keys.data(), vals.data());
            REQUIRE((i == 3 || res));
            REQUIRE_EQ(i+1, keys.front());
        }
    }

    TEST_CASE("Stress test linked data") {
        auto t {std::chrono::high_resolution_clock::now()};
        constexpr int card {90000}, ntries { 40 }, dim{3}, valdim{5};
        std::random_device rd;
        std::array<int, dim> keys{};
        std::array<double, valdim> values{};
        for(int k{}; k<ntries; k++) {
            TLinkedData<int, double> lst {dim, sizeof(double)*valdim};
            std::mt19937 rng(rd());
            std::uniform_int_distribution<int> gen(1, 100);
            for(int j{}; j < card; j++) {
                for(int l{}; l < dim; l++)
                    keys[l] = gen(rng);
                for(int l{}; l < valdim; l++)
                    values[l] = gen(rng);
                lst.AddItem(keys.data(), values.data());
            }
            lst.Sort();
        }
        auto delta {std::chrono::high_resolution_clock::now() - t};
        std::cout << "Time in milliseconds for TLinkedData: "s << delta / std::chrono::milliseconds(1) << std::endl;
    }

    TEST_SUITE_END();

}
