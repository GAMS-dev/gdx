#include <string>
#include <numeric>
#include "../doctest.h"
#include "../../gdlib/gmsdata.h"

using namespace std::literals::string_literals;
using namespace gdlib::gmsdata;

namespace tests::gmsdatatests {
    TEST_SUITE_BEGIN("gdlib::gmsdata");

    TEST_CASE("Test basic usage of TGrowArrayFxd") {
        TGrowArrayFxd<int> gaf;
        REQUIRE_EQ(0, gaf.GetCount());
        REQUIRE_EQ(0, gaf.MemoryUsed());
        // make sure we need at least three fixed size buffers as storage
        const int ub {BufSize/sizeof(int)*3+1};
        for(int n{}; n<ub; n++)
            gaf.AddItem(&n); // add item actually copies contents
        REQUIRE_EQ(ub, gaf.GetCount());
        REQUIRE(gaf.MemoryUsed() > 0);
        int k;
        for(int i{}; i<ub; i++) {
            gaf.GetItem(i, (int **)&k);
            REQUIRE_EQ(i, k);
            REQUIRE_EQ(i, *gaf.GetItemPtrIndex(i));
        }
        gaf.Clear();
        REQUIRE_EQ(0, gaf.GetCount());
        REQUIRE_EQ(0, gaf.MemoryUsed());
    }

    TEST_SUITE_END();
}