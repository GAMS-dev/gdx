#include <string>
#include "../doctest.h"
#include "../../global/delphitypes.h"

using namespace std::literals::string_literals;
using namespace global::delphitypes;

namespace tests::shortstringtests {

    TEST_SUITE_BEGIN("global::delphitypes");

    TEST_CASE("Simple usage of stack short strings") {
        ShortString s{"test"};
        REQUIRE_EQ(4, s.size());
        REQUIRE_EQ(s, "test"s);
        REQUIRE(!strcmp(s.c_str(), "test"));
        char *dstr {s.d_str()};
        REQUIRE_EQ(4, dstr[0]);
        REQUIRE(!strcmp(&dstr[1], "test"));
    }

    TEST_CASE("Simple usage of heap short strings") {
        ShortStringHeap s{"test"};
        REQUIRE_EQ(4, s.size());
        REQUIRE_EQ(s, "test"s);
        REQUIRE(!strcmp(s.c_str(), "test"));
        char *dstr {s.d_str()};
        REQUIRE_EQ(4, dstr[0]);
        REQUIRE(!strcmp(&dstr[1], "test"));
    }

    TEST_SUITE_END();

}