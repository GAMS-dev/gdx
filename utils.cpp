#include "utils.h"

#include <list>
#include <algorithm>
#include <chrono>
#include <cassert>
#include <numeric>
#include <cstring>

using namespace std::literals::string_literals;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace utils {

    std::string uppercase(const std::string_view s) {
        std::string out{ s };
        std::transform(s.begin(), s.end(), out.begin(), utils::toupper);
        return out;
    }

    bool sameTextInvariant(const std::string_view a, const std::string_view b) {
        const auto l = a.length();
        if (b.length() != a.length()) return false;
        for (size_t i{}; i < l; i++) {
            if (utils::tolower(a[i]) != utils::tolower(b[i]))
                return false;
        }
        return true;
    }

    std::string_view trim(const std::string_view s) {
        if(s.empty()) return {};
        int firstNonBlank {-1}, lastNonBlank {};
        for(int i{}; i<s.length(); i++) {
            if((unsigned char)s[i] > 32) {
                if(firstNonBlank == -1) firstNonBlank = i;
                lastNonBlank = i;
            }
        }
        if(firstNonBlank == -1) return {};
        return s.substr(firstNonBlank, (lastNonBlank - firstNonBlank) + 1);
    }

    const char *trimRight(const char *s, char *storage, int &slen) {
        int i;
        slen = -1; // all whitespace? => slen=0!
        for(i=0; s[i] != '\0'; i++)
            if(static_cast<unsigned char>(s[i]) > 32)
                slen = i;
        if(++slen == i) return s;
        std::memcpy(storage, s, slen);
        storage[slen] = '\0';
        return storage;
    }

    std::vector<size_t> substrPositions(const std::string_view s, const std::string_view substr) {
        std::vector<size_t> positions;
        for (size_t p{s.find(substr)}; p != std::string::npos; p = s.find(substr, p + substr.size()))
            positions.push_back(p);
        return positions;
    }

    std::string replaceSubstrs(const std::string_view s, const std::string_view substr, const std::string_view replacement) {
        if (substr == replacement) return std::string{ s };
        std::string out{};
        const int ssl = static_cast<int>(substr.length());
        const auto positions = substrPositions(s, substr);
        for (int i = 0; i < (int) s.length(); i++) {
            if (utils::in<size_t>(i, positions)) {
                out += replacement;
                i += ssl - 1;
                continue;
            }
            out += s[i];
        }
        return out;
    }

    bool strContains(const std::string_view s, char c) {
        return s.find(c) != std::string::npos;
    }

    bool excl_or(bool a, bool b) {
        return (a && !b) || (!a && b);
    }

    // same as std::string::substr but silent when offset > input size
    std::string_view substr(const std::string_view s, int offset, int len) {
        return (s.empty() || offset > (int) s.size() - 1) ? std::string_view {} : s.substr(offset, len);
    }

    std::string constructStr(int size, const std::function<char(int)> &charForIndex) {
        std::string s;
        s.resize(size);
        for (int i{}; i < size; i++)
            s[i] = charForIndex(i);
        return s;
    }

    bool starts_with(const std::string_view s, const std::string_view prefix) {
        if (prefix.length() > s.length()) return false;
        for (int i = 0; i < (int) prefix.length(); i++) {
            if (s[i] != prefix[i])
                return false;
        }
        return true;
    }

    std::string quoteWhitespace(const std::string &s, char quotechar) {
        return utils::strContains(s, ' ') ? ""s + quotechar + s + quotechar : s;
    }

    inline int b2i(bool b) { return b ? 1 : 0; }

    int strCompare(const std::string_view S1, const std::string_view S2, bool caseInsensitive) {
        if (S1.empty() || S2.empty()) return b2i(!S1.empty()) - b2i(!S2.empty());
        auto L = S1.length();
        if (L > S2.length()) L = S2.length();
        for (size_t K{}; K < L; K++) {
            int c1 = static_cast<unsigned char>(caseInsensitive ? utils::toupper(S1[K]) : S1[K]);
            int c2 = static_cast<unsigned char>(caseInsensitive ? utils::toupper(S2[K]) : S2[K]);
            int d = c1 - c2;
            if (d) return d;
        }
        return static_cast<int>(S1.length() - S2.length());
    }

    bool checkBOMOffset(const tBomIndic &potBOM, int &BOMOffset, std::string &msg) {
        enum tBOM {
            bUTF8, bUTF16BE, bUTF16LE, bUTF32BE, bUTF32LE, num_tboms
        };
        const std::array<std::string, num_tboms> BOMtxt = {"UTF8"s, "UTF16BE"s, "UTF16LE"s, "UTF32BE"s, "UTF32LE"s};
        const std::array<std::array<uint8_t, maxBOMLen + 1>, num_tboms> BOMS = {
                {{3, 239, 187, 191, 0},  // UTF8
                 {2, 254, 255, 0, 0},  // UTF16BE
                 {2, 255, 254, 0, 0},  // UTF16LE
                 {4, 0, 0, 254, 255},  // UTF32BE
                 {4, 255, 254, 0, 0} // UTF32LE
                }
        };
        msg.clear();
        BOMOffset = 0;
        for (int b = 0; b < num_tboms; b++) {
            bool match{true};
            for (int j{1}; j <= BOMS[b][0]; j++) {
                if (BOMS[b][j] != potBOM[j - 1]) {
                    match = false;
                    break;
                }
            }
            if (!match) continue;

            if (b == bUTF8) BOMOffset = BOMS[b].front(); // UTF8 is the only one, which is OK atm
            else {
                msg = BOMtxt[b] + " BOM detected. This is an unsupported encoding.";
                return false;
            }
            break;
        }
        return true;
    }

    /**
     * PORTING NOTES FROM ANDRE
     * Pascal/Delphi convention: 1 byte is size/length/charcount, then character bytes, then quote byte END
     * C/C++ convention here: raw character bytes, null terminator \0, quote char after that
     * Doing the quote char after the terminator keeps strlen etc working
     **/

    // Convert C++ standard library string to Delphi short string
    int strConvCppToDelphi(const std::string_view s, char *delphistr) {
        if (s.length() > std::numeric_limits<uint8_t>::max()) {
            const std::string errorMessage{"Error: Maximum short string length is 255 characters!"s};
            strConvCppToDelphi(errorMessage, delphistr);
            return static_cast<int>(errorMessage.length());
        }
        const auto l = static_cast<uint8_t>(s.length());
        delphistr[0] = static_cast<char>(l);
        std::memcpy(&delphistr[1], s.data(), l);
        return 0;
    }
}
