#include <string>
#include "../doctest.h"
#include "../../gdlib/strhash.h"
#include <numeric>

using namespace std::literals::string_literals;
using namespace gdlib::strhash;

namespace tests::strhashtests {

    TEST_SUITE_BEGIN("gdlib::strhash");

    template<typename T>
    void runTest(const std::array<int, 10> &nums) {
        T shlst;
        // 0-based
        for(const int &n : nums) {
            REQUIRE_EQ(n-1, shlst.AddObject("i" + std::to_string(n), n));
        }
        REQUIRE_EQ(2, shlst.IndexOf("i3"s));
    };

    TEST_CASE("Adding some set element names that are mapped to numbers") {
        std::array<int, 10> nums {};
        std::iota(nums.begin(), nums.end(), 1);
        runTest<TXStrHashList<int>>(nums);
        runTest<TXStrHashListLegacy<int>>(nums);
    }

    TEST_SUITE_END();

}