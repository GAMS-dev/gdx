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
        REQUIRE_EQ(0, bba.MemoryUsed());
        bba.SetBit(3, true);
        REQUIRE(bba.MemoryUsed() > 0);
        REQUIRE(bba.GetBit(3));
        REQUIRE_EQ(3, bba.GetHighIndex());
        for(int i{-2}; i<3; i++)
            REQUIRE_FALSE(bba.GetBit(i));
        for(int i{4}; i<6; i++)
            REQUIRE_FALSE(bba.GetBit(i));
        bba.SetHighIndex(4);
        REQUIRE_FALSE(bba.GetBit(4));
        auto oldMem = bba.MemoryUsed();
        bba.SetBit(4, true);
        REQUIRE_EQ(bba.MemoryUsed(), oldMem);
        auto vec = asBoolVec(bba);
        REQUIRE_EQ(5, vec.size());
        std::vector<bool> expectedVec {false, false, false, true, true};
        for(int i{}; i<5; i++)
            REQUIRE_EQ(expectedVec[i], vec[i]);
        // Make sure we need a second alloc
        oldMem = bba.MemoryUsed();
        int highIndex = bba.MemoryUsed()*8+10;
        bba.SetBit(highIndex, true);
        REQUIRE(bba.GetBit(3));
        REQUIRE(bba.GetBit(4));
        REQUIRE(bba.GetBit(highIndex));
        REQUIRE(bba.MemoryUsed() > oldMem);
    }

    TEST_CASE("Simple use of TXList") {
        TXList<int> lst;
        std::array<int, 23> nums {};
        for(int i=0; i<(int)nums.size(); i++) {
            nums[i] = i + 1;
            REQUIRE_EQ(i, lst.Add(&nums[i]));
        }
        REQUIRE_EQ(nums.size(), lst.GetCount());
        REQUIRE(lst.MemoryUsed() > 0);
        for(int i=0; i<lst.GetCount(); i++)
            REQUIRE_EQ(&nums[i], lst[i]);
        REQUIRE_EQ(&nums.back(), lst.GetLast());

        lst.Delete(0);
        REQUIRE_EQ(22, lst.GetCount());
        REQUIRE_EQ(2, *lst[0]);
        lst.Insert(0, &nums.front());
        REQUIRE_EQ(23, lst.GetCount());
        REQUIRE_EQ(1, *lst[0]);

        lst.Insert(10, &nums.front());
        REQUIRE_EQ(24, lst.GetCount());
        REQUIRE_EQ(1, *lst[10]);
        REQUIRE_EQ(lst[0], lst[10]);
        REQUIRE_EQ(10, lst.Remove(&nums.front()));
        REQUIRE_EQ(23, lst.GetCount());
        REQUIRE_EQ(0, lst.Remove(&nums.front()));
        REQUIRE_EQ(22, lst.GetCount());
    }

    TEST_CASE("Simple use of TXStrings") {
        TXStrings lst;
        lst.Add("First");
        lst.Add("Second");
        lst.Add("Third");
        REQUIRE_EQ(1, lst.IndexOf("Second"));
        REQUIRE_EQ("First", lst[0]);
    }

    TEST_SUITE_END();

}
