#include "doctest.h"

namespace tests::gxfiletests {
    TEST_SUITE_BEGIN("gxfile");

    TEST_CASE("Dummy test") {
        REQUIRE_EQ(3.141, 3.141);
    }

    TEST_SUITE_END();
}