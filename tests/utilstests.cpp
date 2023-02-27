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
        // With std::string
        REQUIRE(utils::sameText(""s, ""s));
        REQUIRE(utils::sameText("aBc"s, "AbC"s));
        REQUIRE_FALSE(utils::sameText("aBc"s, "AbC"s, false));

        // With pchars (pointer to char -> char *)
        // Case-insensitive
        REQUIRE(utils::sameTextPChar(nullptr, nullptr));
        REQUIRE(utils::sameTextPChar("", ""));
        REQUIRE(utils::sameTextPChar("aBc", "AbC"));
        REQUIRE(utils::sameTextPChar("abc", "abc"));
        // Case-sensitive
        REQUIRE_FALSE(utils::sameTextPChar("aBc", "AbC "));
        REQUIRE_FALSE(utils::sameTextPChar("aBc", "AbC", false));
        REQUIRE(utils::sameTextPChar("abc", "abc", false));
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
        utils::trimRight(s, storage);
        REQUIRE_EQ(expectedStr, storage);

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
        
        REQUIRE_FALSE(utils::strConvCppToDelphi("abc", buf.data()));
        REQUIRE_EQ(3, buf.front());
        REQUIRE_EQ('a', buf[1]);
        REQUIRE_EQ('b', buf[2]);
        REQUIRE_EQ('c', buf[3]);

        REQUIRE_FALSE(utils::strConvCppToDelphi("", buf.data()));
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

    TEST_CASE("Test checking if any character from a string satisfies a given predicate") {
        REQUIRE(utils::anychar([](char c) { return c == 's';  }, "test"));
        REQUIRE_FALSE(utils::anychar([](char c) { return c == 'x';  }, "test"));
        REQUIRE_FALSE(utils::anychar([](char c) { return true;  }, ""));
    }

    TEST_CASE("Test permutated assigned of string characters") {
        std::string dest(3, ' '), src{"cab"};
        std::vector<int> readIndices{1,2,0}, writeIndices{0,1,2};
        utils::permutAssign(dest, src, writeIndices, readIndices);
        REQUIRE_EQ("abc"s, dest);
    }

    TEST_CASE("Test removing line ending marker") {
        std::string s1{ "First line\r\nSecond line\r"s },
                    s2{ "First line\r\nSecond line\n"s };
        utils::removeTrailingCarriageReturnOrLineFeed(s1);
        REQUIRE_EQ("First line\r\nSecond line"s, s1);
        utils::removeTrailingCarriageReturnOrLineFeed(s2);
        REQUIRE_EQ("First line\r\nSecond line"s, s2);
    }

    TEST_CASE("Test upper-/lowercasing of strings (copy)") {
        std::string s{ "This is a Test!"s },
                    expectUppercase{ "THIS IS A TEST!" },
                    expectLowercase{ "this is a test!" };
        REQUIRE_EQ(expectUppercase, utils::uppercase(s));
        REQUIRE_EQ(expectLowercase, utils::lowercase(s));
    }

    TEST_CASE("Test checking if a string case-insensitive matches at least one element from a string list") {
        REQUIRE_FALSE(utils::sameTextAsAny("test", { "abc", "aBc","test X" }));
        REQUIRE(utils::sameTextAsAny("test", { "abc", "TEST" }));
        REQUIRE_FALSE(utils::sameTextAsAny("", {}));
    }

    TEST_CASE("Test if a string starts with a prefix") {
        REQUIRE(utils::sameTextPrefix("test", "te"));
        REQUIRE_FALSE(utils::sameTextPrefix("asset", "te"));
        REQUIRE_FALSE(utils::sameTextPrefix("test", "testing"));
    }

    TEST_CASE("Test trimming zeroes from string from the right/back but only for decimals (not integers)") {
        REQUIRE_EQ("1230.045"s, utils::trimZeroesRight("1230.045000"s));
        REQUIRE_EQ("1230,045"s, utils::trimZeroesRight("1230,045000"s, ','));
        REQUIRE_EQ("1230045000"s, utils::trimZeroesRight("1230045000"s));
    }

    TEST_CASE("Test checking if a string contains a char with code less than given integer") {
        REQUIRE(utils::hasCharLt("a", 'b'));
        REQUIRE_FALSE(utils::hasCharLt("b", 'a'));
    }

    TEST_CASE("Test rounding to n-digits") {
        const double eps{1e-4};
        REQUIRE_LT(23.42 - utils::round(23.4242, 2), eps);
        REQUIRE_LT(23.4 - utils::round(23.4242, 1), eps);
        REQUIRE_LT(23.0 - utils::round(23.4242, 0), eps);
    }

    TEST_CASE("Test replacing specific char with another one in a string (in place)") {
        std::string s{ "Letter X"s };
        utils::replaceChar('X', 'Y', s);
        REQUIRE_EQ("Letter Y"s, s);
    }

    TEST_CASE("Test generating string of repeating zeroes") {
        REQUIRE_EQ("0000"s, utils::zeros(4));
        REQUIRE(utils::zeros(0).empty());
    }

    TEST_CASE("Test getting index of last occurence of character in string") {
        REQUIRE_EQ(13, utils::lastOccurence("abcdefabcdefabcdef", 'b'));
        REQUIRE_EQ(-1, utils::lastOccurence(std::string(23, 'a'), 'b'));
    }

    TEST_CASE("Test computing the length of a string without blanks") {
        REQUIRE_EQ(4, utils::strLenNoWhitespace(" te s t  "));
        REQUIRE_EQ(4, utils::strLenNoWhitespace("test"));
        REQUIRE_EQ(0, utils::strLenNoWhitespace(std::string(10, ' ')));
    }

    TEST_CASE("Test getting a ref to char in a str at index pos with null-terminator-tail enforcement") {
        std::string s{ "test"s };
        REQUIRE_EQ(s.front(), utils::getCharAtIndexOrAppend(s, 0));
        REQUIRE_EQ('\0', utils::getCharAtIndexOrAppend(s, 4));
    }

    TEST_CASE("Test if string contains a specific character or a char from a set of chars") {
        REQUIRE(utils::strContains("test", 't'));
        REQUIRE_FALSE(utils::strContains("test", 'x'));
        REQUIRE(utils::strContains("test", { 'f', 'g', 'e'}));
        REQUIRE_FALSE(utils::strContains("test", { 'x', 'y', 'z'}));
    }

    TEST_CASE("Test boolean 'exclusive or' (xor) operator") {
        REQUIRE(utils::excl_or(true, false));
        REQUIRE(utils::excl_or(false, true));
        REQUIRE_FALSE(utils::excl_or(false, false));
        REQUIRE_FALSE(utils::excl_or(true, true));
    }

    TEST_CASE("Test finding the index of where a substring starts in an enclosing string") {
        REQUIRE_EQ(4, utils::posOfSubstr("osteron", "testosteron"));
        REQUIRE_EQ(-1, utils::posOfSubstr("xyz", "test"));
    }

    TEST_CASE("Test constructing string via lambda with index arg") {
        std::string s { "test"s };
        REQUIRE_EQ(s, utils::constructStr((int)s.size(), [&s](int ix) { return s[ix]; }));
        REQUIRE(utils::constructStr(0, [](int ix) { return 0; }).empty());
    }

    TEST_CASE("Test quoting a string iff. it contains blanks") {
        REQUIRE_EQ("nowhitespace", utils::quoteWhitespace("nowhitespace", '\"'));
        REQUIRE_EQ("\"has whitespace\"", utils::quoteWhitespace("has whitespace", '\"'));
    }

    TEST_CASE("Test string to bool conversion") {
        REQUIRE(utils::strToBool("yes"));
        REQUIRE_FALSE(utils::strToBool("no"));
    }

    TEST_CASE("Test lexicographical string comparison (optionally case-sensitive)") {
        REQUIRE_EQ(-1, utils::strCompare("Alpha", "Beta"));
        REQUIRE_EQ(1, utils::strCompare("Beta", "Alpha"));
        REQUIRE_EQ(0, utils::strCompare("alpha", "Alpha"));
        REQUIRE_EQ(32, utils::strCompare("alpha", "Alpha", false));
    }

    TEST_CASE("Test copying the contents of a pchar into a char buffer") {
        std::array<char, 256> buf {};
        utils::assignPCharToBuf("abc", buf.data(), buf.size());
        REQUIRE(!std::strcmp("abc", buf.data()));
        std::string tooLong(256, 'x');
        utils::assignPCharToBuf(tooLong.c_str(), buf.data(), buf.size());
        REQUIRE_EQ('\0', buf.back());
    }

    TEST_CASE("Test slurping the contents from a text file") {
        std::string fn{ "tmp.txt"s },
                    contents{ "First line\nsecond line\nthird line"s };
        {
            std::ofstream ofs{ fn };
            ofs.write(contents.c_str(), contents.size());
        }
        REQUIRE_EQ(contents, utils::slurp(fn));
        std::filesystem::remove(fn);
    }

    TEST_SUITE_END();

}
