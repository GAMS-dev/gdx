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
    }

    TEST_SUITE_END();

}
