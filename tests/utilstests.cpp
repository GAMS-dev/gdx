#include <set>
#include <fstream>
#include <filesystem>

#include "../utils.h"

#include "doctest.h"

using namespace std::literals::string_literals;

namespace tests::utilstests {

    TEST_SUITE_BEGIN("utils");

    TEST_CASE("Test character upper/lowercase conversion") {
        REQUIRE_EQ('A', utils::toupper('a'));
        REQUIRE_EQ('Z', utils::toupper('z'));
        REQUIRE_EQ('$', utils::toupper('$'));
        REQUIRE_EQ('a', utils::tolower('A'));
        REQUIRE_EQ('z', utils::tolower('Z'));
        REQUIRE_EQ('$', utils::tolower('$'));
    }

    class ClassWithContains : public utils::IContainsPredicate<int> {
    public:
        bool contains(const int &elem) const override {
            return elem == 23;
        }
    };

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

        {
            ClassWithContains cwc;
            REQUIRE(utils::in(23, cwc));
            REQUIRE_FALSE(utils::in(1, cwc));
        }
    }

    TEST_CASE("Comparing strings the Delphi way") {
        // With std::string
        REQUIRE(utils::sameText(""s, ""s));
        REQUIRE(utils::sameText("aBc"s, "AbC"s));
        REQUIRE_FALSE(utils::sameText<false>("aBc"s, "AbC"s));

        // With pchars (pointer to char -> char *)
        // Case-insensitive
        REQUIRE(utils::sameTextPChar(nullptr, nullptr));
        REQUIRE(utils::sameTextPChar("", ""));
        REQUIRE(utils::sameTextPChar("aBc", "AbC"));
        REQUIRE(utils::sameTextPChar("abc", "abc"));
        // Case-sensitive
        REQUIRE_FALSE(utils::sameTextPChar<false>("aBc", "AbC "));
        REQUIRE_FALSE(utils::sameTextPChar<false>("aBc", "AbC"));
        REQUIRE(utils::sameTextPChar<false>("abc", "abc"));
    }

    TEST_CASE("Trim blanks from beginning and end of strings but not in between") {
        REQUIRE_EQ("", utils::trim(""));
        REQUIRE_EQ("", utils::trim("                  \t \n           "));
        REQUIRE_EQ("abc def", utils::trim("   abc def "));
        REQUIRE_EQ("abc", utils::trim("     abc"));
        REQUIRE_EQ("abc", utils::trim("abc     "));
    }

    TEST_CASE("Uppercase all characters in a string") {
        REQUIRE_EQ(utils::uppercase(" gams software gmbh "s), " GAMS SOFTWARE GMBH "s);
    }

    TEST_CASE("All positions (index) of substrings in a string") {
        REQUIRE_EQ(std::vector<size_t> {5}, utils::substrPositions("3.14e+01"s, "+"s));
        REQUIRE_EQ(std::vector<size_t> {0}, utils::substrPositions("+3.14"s, "+"s));
        REQUIRE_EQ(std::vector<size_t> {0,1,3}, utils::substrPositions("??x?x"s, "?"s));
    }

    TEST_CASE("Test checking for a prefix in a string") {
        REQUIRE_FALSE(utils::starts_with("9", "999"));
        REQUIRE_FALSE(utils::starts_with("Das hier ist ein Test", "Test"));
        REQUIRE(utils::starts_with("999", "9"));
        REQUIRE(utils::starts_with("999", "999"));
    }

    TEST_CASE("Test replacing all matching substrings of given string with a replacement string") {
        REQUIRE_EQ("Amazing.", utils::replaceSubstrs("This is Amazing.", "This is ", ""));
        REQUIRE_EQ("", utils::replaceSubstrs("          ", " ", ""));

        const auto res = utils::replaceSubstrs("There is no such thing as a substring called substring in this string.",
                              "substring",
                              "thing");
        REQUIRE_EQ("There is no such thing as a thing called thing in this string.", res);
    }

    TEST_CASE("Test trimming blanks from the tail of a string") {
        const std::string expectedStr {"  surrounded by blanks"s};
        std::string s {"  surrounded by blanks  "s};

        int len;
        std::array<char, 256> buf {};
        const char *out = utils::trimRight(s.c_str(), buf.data(), len);
        REQUIRE_EQ(expectedStr.length(), len);
        REQUIRE(!strcmp(expectedStr.c_str(), out));

        out = utils::trimRight(" ", buf.data(), len);
        REQUIRE_EQ(0, len);
        REQUIRE_EQ('\0', out[0]);

        out = utils::trimRight("1", buf.data(), len);
        REQUIRE_EQ(1, len);
        REQUIRE(!strcmp("1", out));

        std::string _64_chars(64, 'i');
        const char *backing = _64_chars.c_str();
        out = utils::trimRight(backing, buf.data(), len);
        REQUIRE_EQ(64, len);
        REQUIRE(!strcmp(_64_chars.c_str(), out));
        // no blanks at end of this i*64 ('iii...iii') str, so should not create new buffer!
        REQUIRE_EQ(backing, out);
    }

    TEST_CASE("Test conversion of string view into Delphi-format ShortString") {
        std::array<char, 256> buf{};
        
        REQUIRE_FALSE(utils::strConvCppToDelphi("abc"s, buf.data()));
        REQUIRE_EQ(3, buf.front());
        REQUIRE_EQ('a', buf[1]);
        REQUIRE_EQ('b', buf[2]);
        REQUIRE_EQ('c', buf[3]);

        REQUIRE_FALSE(utils::strConvCppToDelphi(""s, buf.data()));
        REQUIRE_EQ(0, buf.front());

        REQUIRE_FALSE(utils::strConvCppToDelphi(std::string(255, 'x'), buf.data()));
        REQUIRE_EQ(255, (uint8_t)buf.front());
        for (int i{}; i < 255; i++)
            REQUIRE_EQ('x', buf[i + 1]);

        REQUIRE(utils::strConvCppToDelphi(std::string(256, 'x'), buf.data()));
        REQUIRE_GT(buf.front(), 0);
        REQUIRE_EQ('E', buf[1]);
        REQUIRE_EQ('r', buf[2]);
        REQUIRE_EQ('r', buf[3]);
        REQUIRE_EQ('o', buf[4]);
        REQUIRE_EQ('r', buf[5]);
    }

    TEST_CASE("Test if string contains a specific character or a char from a set of chars") {
        REQUIRE(utils::strContains("test"s, 't'));
        REQUIRE_FALSE(utils::strContains("test"s, 'x'));
    }

    TEST_CASE("Test boolean 'exclusive or' (xor) operator") {
        REQUIRE(utils::excl_or(true, false));
        REQUIRE(utils::excl_or(false, true));
        REQUIRE_FALSE(utils::excl_or(false, false));
        REQUIRE_FALSE(utils::excl_or(true, true));
    }

    TEST_CASE("Test constructing string via lambda with index arg") {
        std::string s { "test"s };
        REQUIRE_EQ(s, utils::constructStr((int)s.size(), [&s](int ix) { return s[ix]; }));
        REQUIRE(utils::constructStr(0, [](int ix) { return 0; }).empty());
    }

    TEST_CASE("Test quoting a string iff. it contains blanks") {
        REQUIRE_EQ("nowhitespace"s, utils::quoteWhitespace("nowhitespace"s, '\"'));
        REQUIRE_EQ("\"has whitespace\""s, utils::quoteWhitespace("has whitespace"s, '\"'));
    }

    TEST_CASE("Test lexicographical string comparison (optionally case-sensitive)") {
        REQUIRE_EQ(-1, utils::strCompare("Alpha"s, "Beta"s));
        REQUIRE_EQ(1, utils::strCompare("Beta"s, "Alpha"s));
        REQUIRE_EQ(0, utils::strCompare("alpha"s, "Alpha"s));
        REQUIRE_EQ(32, utils::strCompare("alpha"s, "Alpha"s, false));
    }

    TEST_CASE("Test copying the contents of a pchar into a char buffer") {
        std::array<char, 256> buf {};
        utils::assignPCharToBuf("abc", buf.data(), buf.size());
        REQUIRE(!std::strcmp("abc", buf.data()));
        std::string tooLong(256, 'x');
        utils::assignPCharToBuf(tooLong.c_str(), buf.data(), buf.size());
        REQUIRE_EQ('\0', buf.back());
    }


    TEST_CASE("Test getting index of element in collection (list, vector, array, ...)") {
        auto checkForTwo = [](const int &elem) {
            return elem == 2;
        };

        // list
        REQUIRE_EQ(1, utils::indexOf(std::list<int>{1,2,3}, 2));
        REQUIRE_EQ(-1, utils::indexOf(std::list<int>{1,2,3}, 5));
        REQUIRE_EQ(-1, utils::indexOf<int>(std::list<int>{}, 2));
        REQUIRE_EQ(1, utils::indexOf<int>(std::list<int>{1,2,3}, checkForTwo));
    }

    TEST_CASE("Test constructing a std::array object filled by repeating a single value") {
        std::array<int, 3>  arr {utils::arrayWithValue<int, 3>(23)},
                            expArr {23,23,23};
        REQUIRE_EQ(expArr, arr);
    }

    TEST_SUITE_END();

}
