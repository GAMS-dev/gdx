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

    TEST_CASE("Test basic usage of TXIntList") {
        TXIntList lst;
        REQUIRE_EQ(0, lst.GetCount());
        REQUIRE_EQ(0, lst.MemoryUsed());
        for(int i{}; i<100; i++)
            lst.Add(i);
        REQUIRE_EQ(100, lst.GetCount());
        REQUIRE(lst.MemoryUsed() > 0);
        for(int i{}; i<100; i++)
            REQUIRE_EQ(i, lst[i]);
        lst.Exchange(0, 1);
        REQUIRE_EQ(1, lst[0]);
        REQUIRE_EQ(0, lst[1]);
        lst[0] = 24;
        REQUIRE_EQ(24, lst[0]);
    }

    template<typename T>
    void commonTTblGamsDataTests(T &gdl) {
        REQUIRE_EQ(2, gdl.GetDimension());
        REQUIRE_EQ(0, gdl.GetCount());

        std::array<int, GLOBAL_MAX_INDEX_DIM> keys {};
        std::array<double, 2> vals {};

        vals.front() = 23.0;
        for (int i{}; i < 10; i++) {
            keys.front() = i + 1;
            gdl.AddRecord(keys.data(), vals.data());
        }
        REQUIRE_EQ(10, gdl.GetCount());
        REQUIRE_GT(gdl.MemoryUsed(), 0);

        std::fill(keys.begin(), keys.end(), 0);
        std::fill(vals.begin(), vals.end(), 0);

        for (int i{}; i < 10; i++) {
            gdl.GetRecord(i, keys.data(), vals.data());
            REQUIRE_EQ(i+1, keys.front());
            REQUIRE_EQ(23.0, vals.front());
            std::fill(keys.begin(), keys.end(), 0);
            gdl.GetKeys(i, keys.data());
            REQUIRE_EQ(i+1, keys.front());
            std::fill(vals.begin(), vals.end(), 0);
            gdl.GetData(i, vals.data());
            REQUIRE_EQ(23.0, vals.front());
            REQUIRE_EQ(23.0, static_cast<double *>(gdl.GetDataPtr(i))[0]);
        }

        gdl.Clear();
        REQUIRE_EQ(0, gdl.GetCount());

        std::fill(keys.begin(), keys.end(), 0);
        std::fill(vals.begin(), vals.end(), 0);
        vals.front() = 23.0;

        for (int i{9}; i >= 0; i--) {
            keys.front() = i + 1;
            gdl.AddRecord(keys.data(), vals.data());
        }
        gdl.Sort();
        for (int i{}; i < 10; i++) {
            gdl.GetRecord(i, keys.data(), vals.data());
            REQUIRE_EQ(i + 1, keys.front());
            REQUIRE_EQ(23.0, vals.front());
        }
    }

    TEST_CASE("Test basic usage of TTblGamsDataLegacy") {
        {
            TTblGamsData<double> gdl {2, sizeof(double)*2};
            commonTTblGamsDataTests<TTblGamsData<double>>(gdl);
        }
        {
            TTblGamsDataLegacy<double> gdl{2, sizeof(double) * 2};
            commonTTblGamsDataTests<TTblGamsDataLegacy<double>>(gdl);
        }
    }

    TEST_SUITE_END();
}