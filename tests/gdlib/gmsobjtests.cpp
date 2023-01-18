#include <string>
#include "../doctest.h"
#include "../../gdlib/gmsobj.h"

using namespace std::literals::string_literals;
using namespace gdlib::gmsobj;

namespace tests::gmsobjtests {

    TEST_SUITE_BEGIN("gdlib::gmsobj");

    static std::vector<bool> asBoolVec(TBooleanBitArray &bba) {
        std::vector<bool> res(bba.GetHighIndex()+1);
        for(int i{}; i<res.size(); i++)
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
    }

    TEST_SUITE_END();

}
