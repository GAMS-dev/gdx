#include <string>
#include "../doctest.h"
#include "../../gdlib/gmsheapnew.h"
#include <numeric>

using namespace std::literals::string_literals;
using namespace gdlib::gmsheapnew;

namespace tests::gmsheapnew {

    TEST_SUITE_BEGIN("gdlib::gmsheapnew");

    TEST_CASE("Test simple usage of the custom heap") {
        gdlib::gmsheapnew::THeapMgr thm {"heapmgr-test"s};
        const char *data {"hej"};
        const int l {4};
        void *buf = thm.XGetMem(l);
        std::memcpy(buf, data, l);
        thm.XFreeMem(buf, l);
        const int n {10000};
        std::array<void *, n> ptrs {};
        std::array<uint8_t, 20> buf2{};
        int64_t *ref {(int64_t*)buf2.data()};
        for(int i{}; i<n; i++) {
            ptrs[i] = thm.XGetMem((int)buf2.size());
            *ref = (int64_t)i;
            std::memcpy(ptrs[i], buf2.data(), buf2.size());
        }
        for(int i{}; i<n; i++) {
            REQUIRE_EQ(*((int64_t*)ptrs[i]), (int64_t)i);
            thm.XFreeMem(ptrs[i], (int)buf2.size());
        }
    }

    TEST_SUITE_END();

}
