#include <string>
#include "../doctest.h"
#include "../../gdlib/gmsobj.h"

using namespace std::literals::string_literals;
using namespace gdlib::gmsobj;

namespace tests::gmsobjtests {

    TEST_SUITE_BEGIN("gdlib::gmsobj");

    static std::vector<bool> asBoolVec(const TBooleanBitArray &bba) {
        std::vector<bool> res(bba.GetHighIndex()+1);
        for(int i{}; i<static_cast<int>(res.size()); i++)
            res[i] = bba.GetBit(i);
        return res;
    }

    TEST_CASE("Simple use TBooleanBitArray") {
        TBooleanBitArray bba;
        for(int i{-2}; i<4; i++)
            REQUIRE_FALSE(bba.GetBit(i));
        REQUIRE_EQ(0, bba.GetMemoryUsed());
        bba.SetBit(3, true);
        REQUIRE(bba.GetMemoryUsed() > 0);
        REQUIRE(bba.GetBit(3));
        REQUIRE_EQ(3, bba.GetHighIndex());
        for(int i{-2}; i<3; i++)
            REQUIRE_FALSE(bba.GetBit(i));
        for(int i{4}; i<6; i++)
            REQUIRE_FALSE(bba.GetBit(i));
        bba.SetHighIndex(4);
        REQUIRE_FALSE(bba.GetBit(4));
        auto oldMem = bba.GetMemoryUsed();
        bba.SetBit(4, true);
        REQUIRE_EQ(bba.GetMemoryUsed(), oldMem);
        auto vec = asBoolVec(bba);
        REQUIRE_EQ(5, vec.size());
        std::vector<bool> expectedVec {false, false, false, true, true};
        for(int i{}; i<5; i++)
            REQUIRE_EQ(expectedVec[i], vec[i]);
        // Make sure we need a second alloc
        oldMem = bba.GetMemoryUsed();
        int highIndex = bba.GetMemoryUsed()*8+10;
        bba.SetBit(highIndex, true);
        REQUIRE(bba.GetBit(3));
        REQUIRE(bba.GetBit(4));
        REQUIRE(bba.GetBit(highIndex));
        REQUIRE(bba.GetMemoryUsed() > oldMem);
    }

    TEST_CASE("Simple use of TXList") {
        TXList<int> lst;
        std::array<int, 23> nums {};
        for(int i=0; i<nums.size(); i++) {
            nums[i] = i + 1;
            REQUIRE_EQ(i, lst.Add(&nums[i]));
        }
        REQUIRE_EQ(nums.size(), lst.GetCount());
        REQUIRE(lst.GetMemoryUsed() > 0);
        for(int i=0; i<lst.GetCount(); i++)
            REQUIRE_EQ(&nums[i], lst[i]);
        REQUIRE_EQ(&nums.back(), lst.GetLast());
    }

    TEST_SUITE_END();

}
