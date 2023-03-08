/*
 * GAMS - General Algebraic Modeling System GDX API
 *
 * Copyright (c) 2017-2023 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2023 GAMS Development Corp. <support@gams.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <strings.h>    // for strcasecmp
#include <array>        // for array
#include <cstring>      // for memcpy, size_t, strcmp, strcpy, strlen
#include <string>       // for string, char_traits
#include <string_view>  // for string_view, operator==, basic_string_view
#include <memory>

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace gdx::utils {

    inline char toupper(char c) {
        return c >= 'a' && c <= 'z' ? static_cast<char>(c ^ 32) : c;
    }

    inline char tolower(char c) {
        return c >= 'A' && c <= 'Z' ? static_cast<char>(c ^ 32) : c;
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
    class IContainsPredicate {
    public:
        virtual ~IContainsPredicate() = default;
        virtual bool contains(const T& elem) const = 0;
    };

    template<typename T>
    bool in(const T& val, const IContainsPredicate<T>& coll) {
        return coll.contains(val);
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

    std::string quoteWhitespace(const std::string &s, char quotechar = '\'');

    template<class T, const int size>
    std::array<T, size> arrayWithValue(T v) {
        std::array<T, size> res;
        res.fill(v);
        return res;
    }

    int strCompare(const char *S1, const char *S2, bool caseInsensitive = true);

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

    inline void assignViewToBuf(const std::string_view s, char *buf, size_t outBufSize = 256) {
        if(s.length() + 1 > outBufSize) return;
        std::memcpy(buf, s.data(), s.length());
        buf[s.length()] = '\0';
    }

    inline char *NewString(const char *s, size_t slen) {
        if(!s) return nullptr;
        char *buf{new char[slen+1]};
        utils::assignPCharToBuf(s, slen, buf, slen+1);
        return buf;
    }

    inline char *NewString(const char *s) {
        return !s ? nullptr : NewString(s, std::strlen(s));
    }

    inline std::unique_ptr<char[]> NewStringUniq(const char *s) {
        if(!s) return {};
        const auto slen {std::strlen(s)};
        std::unique_ptr<char[]> buf {std::make_unique<char[]>(slen + 1)};
        utils::assignPCharToBuf(s, slen, buf.get(), slen + 1);
        return buf;
    }

    inline std::unique_ptr<char[]> NewStringUniq(const std::string_view s) {
        std::unique_ptr<char[]> buf {std::make_unique<char[]>(s.length() + 1)};
        utils::assignViewToBuf(s, buf.get(), s.length()+1);
        return buf;
    }

    inline char *NewString(const std::string &s) {
        return NewString(s.c_str());
    }

    inline char *NewString(const char *s, size_t slen, size_t &memSize) {
        if(!s) {
            slen = 0;
            return nullptr;
        }
        const auto l {slen+1};
        char *buf{new char[l]};
        utils::assignPCharToBuf(s, slen, buf, l);
        memSize += l;
        return buf;
    }
}