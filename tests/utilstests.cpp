#include <set>
#include <fstream>
#include <filesystem>

#include "../utils.h"

#include "doctest.h"

using namespace std::literals::string_literals;

namespace tests::utilstests {

    TEST_SUITE_BEGIN("utils");

    TEST_CASE("Test set operations") {
        const std::set<int> a = { 1, 3, 5 },
                b = { 3, 5, 8 },
                expInterC = { 3,5 },
                expUnionC = { 1, 3, 5, 8 };
        REQUIRE_EQ(utils::intersectionOp(a, b), expInterC);
        REQUIRE_EQ(utils::unionOp(a, b), expUnionC);
    }

    TEST_CASE("Test inclusion predicate") {
        const std::vector nums = {1, 2, 3, 23, 0};
        REQUIRE(utils::in(23, nums));
        REQUIRE_FALSE(utils::in(24, nums));

        REQUIRE(utils::in(23, 1, 2, 23, 0));
        REQUIRE_FALSE(utils::in(24, 1, 2, 23, 0));

        REQUIRE(utils::in(1, 1));
        REQUIRE_FALSE(utils::in(1, 2));

        {
            std::vector<int> v;
            REQUIRE_FALSE(utils::in(1, v));
            v.push_back(1);
            REQUIRE(utils::in(1, v));

            std::map<int, int> m;
            REQUIRE_FALSE(utils::in(1, m));
            m[23] = 42;
            REQUIRE(utils::in(23, m));

            std::map<std::string, int> m2;
            REQUIRE_FALSE(utils::in("abc"s, m2));
            m2["abc"] = 42;
            REQUIRE(utils::in("abc"s, m2));
        }
    }

    TEST_CASE("System.Val function to parse numbers from strings") {
        {
            int num, code;
            utils::val("23", num, code);
            REQUIRE_EQ(num, 23);
            REQUIRE_FALSE(code);
            utils::val("23$", num, code);
            REQUIRE_EQ(code, 3);
        }

        {
            double num;
            int code;
            utils::val("23.1", num, code);
            REQUIRE_EQ(num, 23.1);
            REQUIRE_FALSE(code);
            utils::val("23$", num, code);
            REQUIRE_EQ(3, code);
        }

        {
            auto valTest = [](std::string in, auto out, int expectedEC = 0) {
                decltype(out) x{};
                int errorCode{};
                utils::val(in, x, errorCode);
                REQUIRE_EQ(expectedEC, errorCode);
                if(!expectedEC) // only parsed value if we don't expect failure!
                    REQUIRE_EQ(out, x);
            };

            valTest("23", 23);
            valTest("3.141", 3.141);
            valTest("3.141", (int)0, 2);
            valTest("003e+0000", 3.0);
            valTest("1e-1", 0.1);
            valTest("12345_678", (int)0, 6);
            valTest("0xFF", (double)0.0, 2);
            valTest("$FF", (double)0.0, 1);
            valTest("0xFF", 255);
            valTest("$FF", 255);
        }
    }

    TEST_CASE("Comparing strings the Delphi way") {
        REQUIRE(utils::sameText("aBc", "AbC"));
        REQUIRE_FALSE(utils::sameText("aBc", "AbC", false));
    }

    TEST_CASE("Arbitrary size blank strings") {
        REQUIRE_EQ("   ", utils::blanks(3));
    }

    TEST_CASE("Trim blanks from beginning and end of strings but not in between") {
        REQUIRE_EQ("abc def", utils::trim("   abc def "));
        REQUIRE_EQ("abc", utils::trim("     abc"));
        REQUIRE_EQ("abc", utils::trim("abc     "));
    }

    TEST_CASE("Split string into parts using a separating character") {
        REQUIRE_EQ(utils::split("a;b;c", ';'), std::list {"a"s, "b"s, "c"s});
    }

    TEST_CASE("Split string into its whitespace separated parts but don't split inside quotes") {
        REQUIRE_EQ(std::list {"first"s, "\"second part\""s, "\'third part\'"s, "fourth"s}, utils::splitWithQuotedItems(" first   \"second part\" \'third part\' fourth "));
    }

    TEST_CASE("Uppercase all characters in a string") {
        REQUIRE_EQ(utils::uppercase(" gams software gmbh "s), " GAMS SOFTWARE GMBH "s);
    }

    TEST_CASE("Join multiple strings with separator. Inverse operation to split.") {
        REQUIRE_EQ("a;b;c"s, utils::join(';', {"a"s, "b"s, "c"s}));
    }

    TEST_CASE("All positions (index) of substrings in a string") {
        REQUIRE_EQ(std::vector<size_t> {5}, utils::substrPositions("3.14e+01"s, "+"s));
        REQUIRE_EQ(std::vector<size_t> {0}, utils::substrPositions("+3.14"s, "+"s));
        REQUIRE_EQ(std::vector<size_t> {0,1,3}, utils::substrPositions("??x?x"s, "?"s));
    }

    TEST_CASE("Test get line with separator") {
        const std::string tmpfname = "tempfile";
        utils::spit(tmpfname, "first\nsecond\nthird"s);
        std::fstream fs { tmpfname, std::ios::in };
        REQUIRE_EQ("first\n"s, utils::getLineWithSep(fs));
        REQUIRE_EQ("second\n"s, utils::getLineWithSep(fs));
        REQUIRE_EQ("third"s, utils::getLineWithSep(fs));
        fs.close();
        std::filesystem::remove(tmpfname);
    }

    TEST_CASE("Test double to string function with width and precision") {
        REQUIRE_EQ("3"s, utils::doubleToString(3.141, 1, 0));
        REQUIRE_EQ("  23", utils::doubleToString(23, 4, 0));
        REQUIRE_EQ("   3.1416"s, utils::doubleToString(3.141592653589793, 9, 4));
    }

    TEST_CASE("Test integer num to string conversion with specified width and padding from left") {
        REQUIRE_EQ("  23"s, utils::strInflateWidth(23, 4));
        REQUIRE_EQ("23"s, utils::strInflateWidth(23, 2));
        REQUIRE_EQ("23"s, utils::strInflateWidth(23, 0));
    }

    TEST_CASE("Test quote directory names in path with whitespace") {
        REQUIRE_EQ("/home/andre/\"gms test\"/"s, utils::quoteWhitespaceDir("/home/andre/gms test/", '/', '\"'));
        REQUIRE_EQ("/home/andre/\"gms test\""s, utils::quoteWhitespaceDir("/home/andre/gms test", '/', '\"'));
    }

    TEST_CASE("Test checking for a prefix in a string") {
        REQUIRE_FALSE(utils::starts_with("9", "999"));
        REQUIRE_FALSE(utils::starts_with("Das hier ist ein Test", "Test"));
        REQUIRE(utils::starts_with("999", "9"));
        REQUIRE(utils::starts_with("999", "999"));
    }

    TEST_CASE("Test checking for a suffix in a string") {
        REQUIRE_FALSE(utils::ends_with("9", "999"));
        REQUIRE_FALSE(utils::ends_with("Das hier ist ein Test!", "Test"));
        REQUIRE(utils::ends_with("999", "9"));
        REQUIRE(utils::ends_with("999", "999"));
    }

    TEST_CASE("Test parse number") {
        REQUIRE_EQ(3, utils::parseNumber("3"));
        REQUIRE_EQ(3.141, utils::parseNumber("3.141"));
        REQUIRE_EQ(-1, utils::parseNumber("-1"));
        REQUIRE_EQ(0, utils::parseNumber("0"));
        REQUIRE_EQ(3, utils::parseNumber("3e0"));
        REQUIRE_EQ(30, utils::parseNumber("3e1"));
        REQUIRE_EQ(0.3, utils::parseNumber("3e-1"));
    }

    TEST_CASE("Test temporary set element insertion") {
        std::set<int> nums {1,2,3,4}, numsExpectedWith {1,2,3,4,5,6,7,8};
        std::set<int> numsExpectedWithout {nums};
        {
            utils::TempInsert<int> tins {nums, {5,6,7,8} };
            REQUIRE_EQ(numsExpectedWith, nums);
        }
        REQUIRE_EQ(numsExpectedWithout, nums);
    }

    TEST_CASE("Test replacing all matching substrings of given string with a replacement string") {
        REQUIRE_EQ("Amazing.", utils::replaceSubstrs("This is Amazing.", "This is ", ""));
        REQUIRE_EQ("", utils::replaceSubstrs("          ", " ", ""));

        const auto res = utils::replaceSubstrs("There is no such thing as a substring called substring in this string.",
                              "substring",
                              "thing");
        REQUIRE_EQ("There is no such thing as a thing called thing in this string.", res);
    }

    TEST_CASE("Test assigning a single value to all array cells in a range between bounds (inclusive)") {
        std::array<int, 5> nums {1,2,3,4,5}, expNums {1,0,0,0,5};
        utils::assignRange<int, 5>(nums, 1, 3, 0);
        REQUIRE_EQ(expNums, nums);
    }

    TEST_CASE("Test trimming blanks from the tail of a string") {
        const std::string expectedStr {"  surrounded by blanks"s};
        std::string s {"  surrounded by blanks  "s};
        REQUIRE_EQ(expectedStr, utils::trimRight(s));
        std::string storage(64, 'x');
        const std::string &ref = utils::trimRight(s, storage);
        REQUIRE_EQ(expectedStr, ref);
    }

    TEST_SUITE_END();

}
