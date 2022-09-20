#pragma once
#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <iterator>
#include <initializer_list>
#include <list>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <optional>

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace utils {

    template<class T>
    std::set<T> unionOp(const std::set<T>& a, const std::set<T>& b) {
        std::set<T> res = a;
        res.insert(b.begin(), b.end());
        return res;
    }

    inline void insertAllChars(std::set<char>& charset, const std::string& chars) {
        charset.insert(chars.begin(), chars.end());
    }

    inline void charRangeInsert(std::set<char>& charset, char lbIncl, char ubIncl) {
        //for (char c : std::ranges::iota_view{ lbIncl, ubIncl + 1 })
        for (char c = lbIncl; c <= ubIncl; c++)
            charset.insert(c);
    }

    template<class T>
    bool in(const T& val, const std::vector<T>& elems) {
        return std::find(elems.begin(), elems.end(), val) != elems.end();
    }

    template<typename T>
    bool in(const T& val, const T& last) {
        return val == last;
    }

    template<typename T, typename... Args>
    bool in(const T& val, const T& first, Args... rest) {
        return val == first || in(val, rest...);
    }

    template<typename T>
    bool in(const T& val, const std::set<T>& elems) {
        return elems.find(val) != elems.end(); // C++20 starts offering contains method
    }

    template<typename K, typename V>
    bool in(const K& val, const std::map<K, V>& m) {
        return m.find(val) != m.end(); // C++20 starts offering contains method
    }

    template<class T>
    std::set<T> intersectionOp(const std::set<T>& a, const std::set<T>& b) {
        std::set<T> res;
        for (const T& elem : a)
            if (utils::in(elem, b))
                res.insert(elem);
        return res;
    }

    inline void charRangeInsertIntersecting(std::set<char>& charset, char lbIncl, char ubIncl, const std::set<char>& other) {
        //for (char c : std::ranges::iota_view{ lbIncl, ubIncl + 1 })
        for (char c = lbIncl; c <= ubIncl; c++)
            if (utils::in<char>(c, other))
                charset.insert(c);
    }

    template<class T>
    bool any(std::function<bool(const T&)> predicate, const std::initializer_list<T>& elems) {
        return std::any_of(std::cbegin(elems), std::cend(elems), predicate);
    }

    bool anychar(const std::function<bool(char)>& predicate, const std::string& s);

    template<typename T, int count>
    int indexOf(const std::array<T, count> &arr, const T& elem, int notFound = -1) {
        for (int i = 0; i < count; i++)
            if (arr[i] == elem)
                return i;
        return notFound;
    }

    template<typename T>
    int indexOf(const std::vector<T> &elems, const T& elem, int notFound = -1) {
        int i{};
        for (const T& other : elems) {
            if (other == elem)
                return i;
            i++;
        }
        return notFound;
    }

    template<typename T>
    int indexOf(const std::list<T> &elems, const T& elem, int notFound = -1) {
        int i{};
        for (const T& other : elems) {
            if (other == elem)
                return i;
            i++;
        }
        return notFound;
    }

    template<typename T, int count>
    int indexOf(const std::array<T, count> &arr, std::function<bool(const T&)> predicate, int notFound = -1) {
        for (int i{}; i < count; i++)
            if (predicate(arr[i])) return i;
        return notFound;
    }

    template<typename T>
    int indexOf(const std::vector<T>& elems, std::function<bool(const T&)> predicate, int notFound = -1) {
        int i{};
        for (const T & elem : elems) {
            if (predicate(elem)) return i;
            i++;
        }
        return notFound;
    }

    template<typename T>
    int indexOf(const std::list<T>& elems, std::function<bool(const T&)> predicate, int notFound = -1) {
        int i{};
        for (const T & elem : elems) {
            if (predicate(elem)) return i;
            i++;
        }
        return notFound;
    }

    template<typename A, typename B>
    int pairIndexOfFirst(const std::vector<std::pair<A, B>>& elems, A& a, int notFound = -1) {
        int i{};
        for (const auto & [aa,b] : elems) {
            if (aa == a) return i;
            i++;
        }
        return notFound;
    }

    template<typename T>
    inline auto nth(const std::list<T> & elems, int n) {
        return *(std::next(elems.begin(), n));
    }

    template<typename T>
    inline auto &nthRef(std::list<T> & elems, int n) {
        return *(std::next(elems.begin(), n));
    }

    template<typename T>
    inline auto nth(const std::initializer_list<T> & elems, int n) {
        return *(std::next(elems.begin(), n));
    }

    template<typename T>
    void append(std::list<T>& l, const std::initializer_list<T>& elems) {
        std::copy(elems.begin(), elems.end(), std::back_inserter(l));
        /*for (const auto& elem : elems)
            l.push_back(elem);*/
    }

    template<typename T>
    T min(const T a, const T b) {
        return a < b ? a : b;
    }

    template<typename T>
    T adder(T v) { return v; }

    template<typename T, typename... Args>
    T adder(T first, Args... args) {
        return first + adder(args...);
    }

    template<typename... Args>
    void WriteLn(Args... args) {
        std::cout << adder(args...) << '\n';
    }

    void WriteLn();

    void WriteLn(std::fstream* s);

    void permutAssign(std::string& lhs, const std::string& rhs,
        const std::vector<int> &writeIndices, const std::vector<int> &readIndices);

    template<class T>
    void enforceNotInSet(std::set<T>& s, const std::initializer_list<T> forbiddenElements) {
        for (const T& elem : forbiddenElements) {
            const auto it = s.find(elem);
            if (it != s.end())
                s.erase(it);
        }
    }

    inline std::set<char> multiCharSetRanges(std::initializer_list<std::pair<char, char>> lbUbInclCharPairs) {
        std::set<char> res;
        for(const auto &[lb,ub] : lbUbInclCharPairs) {
            charRangeInsert(res, lb, ub);
        }
        return res;
    }

    std::string strInflateWidth(int num, int targetStrLen, char inflateChar = ' ');
    void removeTrailingCarriageReturnOrLineFeed(std::string &s);

    std::string uppercase(const std::string &s);
    std::string lowercase(const std::string& s);

    bool sameText(const std::string& a, const std::string& b, bool caseInvariant = true);
    bool sameTextAsAny(const std::string &a, const std::initializer_list<std::string> &bs);
    bool sameTextPrefix(const std::string &s, const std::string &prefix);

    std::string getLineWithSep(std::fstream &fs);

    std::string trim(const std::string& s);
    std::string trimRight(const std::string &s);
    std::string trimZeroesRight(const std::string& s, char DecimalSep = '.');

    bool hasCharLt(const std::string &s, int n);

    double round(double n, int ndigits);

	void replaceChar(char a, char b, std::string &s);

    std::vector<size_t> substrPositions(const std::string &s, const std::string &substr);
	std::string replaceSubstrs(const std::string &s, const std::string &substr, const std::string &replacement);

    std::string blanks(int n);
    std::string zeros(int n);

    int lastOccurence(const std::string& s, char c);

    // This is a frequent pattern in the CMEX source: temporarily add some elements to a set and remove them afterwords
    // Enclose an object of this type in its own scope such that the object lifetime inserts/removes where appropriate
    template<typename T>
    class TempInsert {
            std::set<T> &set;
            const std::vector<T> elements;
        public:
            TempInsert(std::set<T> &_set, const std::vector<T> &_elements) : set(_set), elements(_elements) {
                set.insert(elements.begin(), elements.end());
            }

            ~TempInsert() {
                for(const auto &elem : elements)
                    set.erase(elem);
            }
    };

    // Mimick val function of System unit in Delphi
    void val(const std::string &s, double &num, int &code);
    void val(const std::string &s, int &num, int &code);
    double parseNumber(const std::string& s);

    void sleep(int milliseconds);

    int strLenNoWhitespace(const std::string &s);

    char& getCharAtIndexOrAppend(std::string& s, int ix);

    bool strContains(const std::string &s, char c);

    bool strContains(const std::string& s, const std::initializer_list<char>& cs);

    template<typename T>
    int genericCount(T start, std::function<T(T)> next, std::function<bool(T)> predicate) {
        int acc{};
        for(T it = start; it; it = next(it)) if(predicate(it)) acc++;
        return acc;
    }

    bool excl_or(bool a, bool b);

    template<typename T>
    std::vector<T> constructVec(int size, std::function<T(int)> elemForIndex) {
        std::vector<T> elems(size);
        for (int i{}; i < size; i++) {
            elems[i] = elemForIndex(i);
        }
        return elems;
    }

    std::string constructStr(int size, const std::function<char(int)> &charForIndex);

    int posOfSubstr(const std::string& sub, const std::string& s);

    std::list<std::string> split(const std::string &s, char sep = ' ');

    std::list<std::string> splitWithQuotedItems(const std::string& s);

    std::string slurp(const std::string& fn);
    void spit(const std::string& fn, const std::string& contents);

    void assertOrMsg(bool condition, const std::string& msg);
    
    std::string substr(const std::string& s, int offset, int len);

    std::string join(char sep, const std::initializer_list<std::string> &parts);

    bool ends_with(const std::string &s, const std::string &suffix);

    bool starts_with(const std::string &s, const std::string &prefix);

    std::string quoteWhitespace(const std::string &s, char quotechar = '\'');

    std::string quoteWhitespaceDir(const std::string &s, char sep, char quotechar = '\"');

    bool hasNonBlank(const std::string &s);

    std::string doubleToString(double v, int width, int precision);

    class StringBuffer {
        std::string s;
        int bufferSize;
    public:
        explicit StringBuffer(int size = BUFSIZ);
        char *getPtr();
        std::string *getStr();
        int getBufferSize() const;
    };

    template<typename A, typename B>
    std::optional<A> keyForValue(const std::map<A, B> &mapping, const B& value) {
        for (const auto& [k, v] : mapping) {
            if (v == value) return k;
        }
        return std::nullopt;
    }

    bool strToBool(const std::string &s);

    // TODO: This should be more general and work with any sequential collection
    template<typename T, int size>
    void assignRange(std::array<T, size> &arr, int lbIncl, int ubIncl, T value) {
        std::fill_n(arr.begin() + lbIncl, ubIncl - lbIncl + 1, value);
    }

    struct BinaryDiffMismatch {
        BinaryDiffMismatch(uint64_t offset, uint8_t lhs, uint8_t rhs);

        uint64_t offset;
        uint8_t lhs, rhs;
    };

    std::optional<std::list<BinaryDiffMismatch>> binaryFileDiff(const std::string& filename1, const std::string& filename2, int countLimit = -1);

    template<class T, int size>
    std::array<T, size> arrayWithValue(T v) {
        std::array<T, size> res;
        res.fill(v);
        return res;
    }

    std::string asdelphifmt(double v, int precision = 8);;

}