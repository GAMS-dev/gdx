#pragma once
#include <algorithm>
#include <array>
#include <functional>
#include <iostream>
#include <iterator>
#include <initializer_list>
#include <unordered_set>
#include <list>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <optional>
#include <cstring>

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace utils {

    inline char toupper(char c) {
        return c >= 'a' && c <= 'z' ? static_cast<char>(c ^ 32) : c;
    }

    inline char tolower(char c) {
        return c >= 'A' && c <= 'Z' ? static_cast<char>(c ^ 32) : c;
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

    template<typename K, typename V>
    bool in(const K& val, const std::unordered_set<K, V>& m) {
        return m.find(val) != m.end(); // C++20 starts offering contains method
    }

    template<typename T>
    class IContainsPredicate {
    public:
        virtual ~IContainsPredicate() = default;
        virtual bool contains(const T& elem) const = 0;
    };

    template<typename T>
    bool in(const T& val, const IContainsPredicate<T>& coll) {
        return coll.contains(val);
    }

    template<class T>
    bool any(std::function<bool(const T&)> predicate, const std::initializer_list<T>& elems) {
        return std::any_of(std::cbegin(elems), std::cend(elems), predicate);
    }

    template<typename T, const int notFound = -1>
    int indexOf(const std::list<T> &elems, const T& elem) {
        int i{};
        for (const T& other : elems) {
            if (other == elem)
                return i;
            i++;
        }
        return notFound;
    }

    template<typename T, const int notFound = -1>
    int indexOf(const std::list<T>& elems, std::function<bool(const T&)> predicate) {
        int i{};
        for (const T & elem : elems) {
            if (predicate(elem)) return i;
            i++;
        }
        return notFound;
    }

    std::string uppercase(std::string_view s);

    bool sameTextInvariant(std::string_view a, std::string_view b);

    template<const bool caseInvariant = true>
    inline bool sameText(std::string_view a, std::string_view b) {
        return caseInvariant ? sameTextInvariant(a, b) : a == b;
    }

    // Port of PStr(U)Equal
    template<const bool caseInvariant = true>
    inline bool sameTextPChar(const char *a, const char *b) {
        if (!a || !b) return !a && !b;
        if constexpr(!caseInvariant) return !std::strcmp(a, b);
#if defined(_WIN32)
        return !_stricmp(a, b);
#else
        return !strcasecmp(a, b);
#endif
    }

    std::string_view trim(std::string_view s);
    const char *trimRight(const char *s, char *storage, int &slen);

    std::vector<size_t> substrPositions(std::string_view s, std::string_view substr);
	std::string replaceSubstrs(std::string_view s, std::string_view substr, std::string_view replacement);

    bool strContains(std::string_view s, char c);

    bool excl_or(bool a, bool b);

    std::string_view substr(std::string_view s, int offset, int len);

    bool starts_with(std::string_view s, std::string_view prefix);

    std::string quoteWhitespace(const std::string &s, char quotechar = '\'');

    template<class T, const int size>
    std::array<T, size> arrayWithValue(T v) {
        std::array<T, size> res;
        res.fill(v);
        return res;
    }

    int strCompare(std::string_view S1, std::string_view S2, bool caseInsensitive = true);

    int strConvCppToDelphi(std::string_view s, char *delphistr);

    inline void assignStrToBuf(const std::string &s, char *buf, int outBufSize = 256) {
        if((int)s.length() > outBufSize) return;
#if defined(_WIN32)
        std::memcpy(buf, s.c_str(), s.length()+1);
#else
        std::strcpy(buf, s.c_str());
#endif
    }

    inline void assignPCharToBuf(const char *s, size_t slen, char *buf, size_t outBufSize = 256) {
        if(slen + 1 > outBufSize) return;
        std::memcpy(buf, s, slen+1);
    }

    inline void assignPCharToBuf(const char *s, char *buf, size_t outBufSize = 256) {
        size_t i;
        for(i = 0; i < outBufSize && s[i] != '\0'; i++)
            buf[i] = s[i];
        buf[i == outBufSize ? i - 1 : i] = '\0'; // truncate when exceeding
    }
}