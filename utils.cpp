#include "utils.h"
#include <list>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cmath>
#include <chrono>
#include <thread>
#include <cassert>
#include <numeric>
#include <cstring>

using namespace std::literals::string_literals;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace utils {

    bool sameTextInvariant(const std::string &a, const std::string &b);

    void parseHex(const std::string &s, int &num, int &code);

    bool determineCode(const std::string &s, const std::function<bool(char)> &charIsLegalPredicate, int &code);

    bool anychar(const std::function<bool(char)> &predicate, const std::string &s) {
        return std::any_of(std::cbegin(s), std::cend(s), predicate);
    }

    void permutAssign(std::string &lhs, const std::string &rhs,
                      const std::vector<int> &writeIndices, const std::vector<int> &readIndices) {
        for (int i = 0; i < (int) writeIndices.size(); i++) {
            lhs[writeIndices[i]] = rhs[readIndices[i]];
        }
    }

    std::string strInflateWidth(int num, int targetStrLen, char inflateChar) {
        auto s = std::to_string(num);
        const auto l = s.length();
        if (l >= (size_t) targetStrLen) return s;
        return std::string(targetStrLen - l, inflateChar) + s;
    }

    void removeTrailingCarriageReturnOrLineFeed(std::string &s) {
        char lchar = s[s.length() - 1];
        if (lchar == '\r' || lchar == '\n')
            s = s.substr(0, s.length() - 1);
    }

    std::string uppercase(const std::string &s) {
        std::string out = s;
        std::transform(s.begin(), s.end(), out.begin(), ::toupper);
        return out;
    }

    std::string lowercase(const std::string &s) {
        if (s.empty()) return s;
        std::string out = s;
        std::transform(s.begin(), s.end(), out.begin(), ::tolower);
        return out;
    }

    bool sameTextInvariant(const std::string &a, const std::string &b) {
        const auto l = a.length();
        if (b.length() != a.length()) return false;
        for (size_t i{}; i < l; i++) {
            if (::tolower(a[i]) != ::tolower(b[i]))
                return false;
        }
        return true;
    }

    bool sameText(const std::string &a, const std::string &b, bool caseInvariant) {
        return caseInvariant ? sameTextInvariant(a, b) : a == b;
    }

    bool sameTextAsAny(const std::string &a, const std::initializer_list<std::string> &bs) {
        return any<std::string>([&a](const std::string &b) { return utils::sameText(a, b); }, bs);
    }

    bool sameTextPrefix(const std::string &s, const std::string &prefix) {
        return sameText(s.substr(0, prefix.length()), prefix);
    }

    std::string getLineWithSep(std::fstream &fs) {
        char c;
        std::stringstream ss;
        while (true) {
            fs.read(&c, 1);
            if (fs.eof() || c == '\0') break;
            ss << c;
            if (c == '\n' || c == '\r') break;
        }
        return ss.str();
    }

    bool hasNonBlank(const std::string &s) {
        return std::any_of(s.begin(), s.end(), [](char c) {
            return !utils::in(c, ' ', '\t', '\r', '\n');
        });
    }

    std::string trim(const std::string &s) {
        if (s.empty()) return s;
        if (!hasNonBlank(s)) return ""s;
        const auto firstNonBlank = s.find_first_not_of(" \t\n\r");
        const auto lastNonBlank = s.find_last_not_of(" \t\n\r");
        return s.substr(firstNonBlank, (lastNonBlank - firstNonBlank) + 1);
    }

    std::string trimRight(const std::string &s) {
        if (s.empty() || !isblank(s.back())) return s;
        const auto lastNonBlank = s.find_last_not_of(" \t");
        return s.substr(0, lastNonBlank + 1);
    }

    const char *trimRight(const char *s, char *storage, int &slen) {
        int i;
        slen = -1; // all whitespace? => slen=0!
        for(i=0; s[i] != '\0'; i++)
            if(!std::isspace(s[i]))
                slen = i;
        if(slen == i) return s;
        std::memcpy(storage, s, ++slen);
        storage[slen] = '\0';
        return storage;
    }

    void trimRight(const std::string &s, std::string &storage) {
        if (s.empty() || !isblank(s.back())) {
            storage = s;
            return;
        }
        const auto ub = s.find_last_not_of(" \t") + 1;
        //storage = s.substr(0, ub);
        storage.replace(0, ub, s, 0, ub);
        storage.resize(ub);
    }

    std::optional<std::string> maybeTrimRight(const std::string &s) {
        if (s.empty() || !isblank(s.back())) return std::nullopt;
        const auto lastNonBlank = s.find_last_not_of(" \t");
        return s.substr(0, lastNonBlank + 1);
    }

    std::string trimZeroesRight(const std::string &s, char DecimalSep) {
        if (s.find(DecimalSep) == std::string::npos) return s;
        int i{static_cast<int>(s.length()) - 1};
        for (; i >= 0; i--)
            if (s[i] != '0') break;
        return s.substr(0, i + 1);
    }

    bool hasCharLt(const std::string &s, int n) {
        return anychar([&n](char c) { return (int) c < n; }, s);
    }

    double round(double n, int ndigits) {
        return std::round(n * std::pow(10, ndigits)) * pow(10, -ndigits);
    }

    void replaceChar(char a, char b, std::string &s) {
        if (a == b) return;
        std::replace_if(s.begin(), s.end(), [a](char i) { return i == a; }, b);
        /*for(char &i : s)
            if(i == a) i = b;*/
    }

    std::vector<size_t> substrPositions(const std::string &s, const std::string &substr) {
        std::vector<size_t> positions;
        for (size_t p{s.find(substr)}; p != std::string::npos; p = s.find(substr, p + substr.size())) {
            positions.push_back(p);
        }
        return positions;
    }

    std::string replaceSubstrs(const std::string &s, const std::string &substr, const std::string &replacement) {
        if (substr == replacement) return s;
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

    bool determineCode(const std::string &s, const std::function<bool(char)> &charIsLegalPredicate, int &code) {
        // first check for offending char and return its position plus one (since 0 is code for "all ok")
        for (int i{}; i < (int) s.length(); i++) {
            char c = s[i];
            if (!charIsLegalPredicate(c)) {
                code = i + 1;
                return true;
            }
        }
        code = 0;
        return false;
    }

    void val(const std::string &s, double &num, int &code) {
        auto islegal = [](char c) {
            return isdigit(c) || c == '.' || toupper(c) == 'E' || c == '-' || c == '+';
        };
        if (determineCode(s, islegal, code)) return;
        num = utils::parseNumber(s);
    }

    inline uint8_t hexval(char c) {
        return c <= 9 ? c : c - 'A' + 10;
    }

    void parseHex(const std::string &s, int &num, int &code) {
        const int off = s.front() == '$' ? 1 : 2;
        int v{};
        for (int exp = 0; exp < (int) s.length() - off; exp++) {
            int i{(int) s.length() - 1 - exp};
            char c = s[i];
            if (!isalnum(c)) {
                code = i;
                return;
            }
            v += hexval(c) * (int) std::pow(16, exp);
        }
        num = v;
    }

    void val(const std::string &s, int &num, int &code) {
        if ((s.length() >= 3 && s.front() == '0' && s[1] == 'x')
            || (s.length() >= 2 && s.front() == '$')) {
            parseHex(s, num, code);
            return;
        }
        auto islegal = [](char c) {
            return isdigit(c) || c == '-';
        };
        if (determineCode(s, islegal, code)) return;
        num = std::stoi(s);
    }

    inline std::string repeatChar(int n, char c) {
        return n > 0 ? std::string(n, c) : "";
    }

    std::string blanks(int n) {
        return repeatChar(n, ' ');
    }

    std::string zeros(int n) {
        return repeatChar(n, '0');
    }

    int lastOccurence(const std::string &s, char c) {
        for (int i = (int) s.length() - 1; i >= 0; i--)
            if (s[i] == c) return i;
        return -1;
    }

    double parseNumber(const std::string &s) {
        /*std::istringstream ss(s);
        double num;
        ss >> num;
        return num;*/

        //return std::stod(s);

        return std::strtod(s.c_str(), nullptr);
    }

    void sleep(int milliseconds) {
        std::this_thread::sleep_for(std::chrono::milliseconds{milliseconds});
    }

    int strLenNoWhitespace(const std::string &s) {
        return (int) std::count_if(s.begin(), s.end(), [](char c) {
            return !std::isspace(c);
        });
    }

    char &getCharAtIndexOrAppend(std::string &s, int ix) {
        const auto l = s.length();
        assert(ix >= 0 && ix <= (int) l && "Index not in valid range");
        if ((size_t) ix == l) s.push_back('\0');
        return s[ix];
    }

    bool strContains(const std::string &s, char c) {
        return s.find(c) != std::string::npos;
    }

    bool strContains(const std::string &s, const std::initializer_list<char> &cs) {
        return std::any_of(std::cbegin(s), std::cend(s),
                           [&cs](char c) { return std::find(cs.begin(), cs.end(), c) != cs.end(); });
    }

    bool excl_or(bool a, bool b) {
        return (a && !b) || (!a && b);
    }

    int posOfSubstr(const std::string &sub, const std::string &s) {
        const auto pos = s.find(sub);
        return pos == std::string::npos ? -1 : (int) pos;
    }

    std::list<std::string> split(const std::string &s, char sep) {
        std::list<std::string> res;
        std::string cur;
        for (char c: s) {
            if (c != sep) cur += c;
            else if (!cur.empty()) {
                res.push_back(cur);
                cur.clear();
            }
        }
        if (!cur.empty()) res.push_back(cur);
        return res;
    }

    std::list<std::string> splitWithQuotedItems(const std::string &s) {
        const char sep = ' ';
        const std::set<char> &quoteChars = {'\"', '\''};
        std::list<std::string> res;
        std::string cur;
        bool inQuote{};
        for (char c: s) {
            if (utils::in(c, quoteChars)) {
                inQuote = !inQuote;
            }
            if (c != sep || inQuote) cur += c;
            else if (!cur.empty()) {
                res.push_back(cur);
                cur.clear();
            }
        }
        if (!cur.empty()) res.push_back(cur);
        return res;
    }

    std::string slurp(const std::string &fn) {
        std::ifstream fp{fn};
        std::stringstream ss;
        std::copy(std::istreambuf_iterator<char>(fp),
                  std::istreambuf_iterator<char>(),
                  std::ostreambuf_iterator<char>(ss));
        return ss.str();
    }

    void spit(const std::string &fn, const std::string &contents) {
        std::ofstream fp{fn};
        fp << contents;
    }

    void assertOrMsg(bool condition, const std::string &msg) {
        if (!condition)
            throw std::runtime_error("Assertion failed: " + msg);
    }

    // same as std::string::substr but silent when offset > input size
    std::string substr(const std::string &s, int offset, int len) {
        return (s.empty() || offset > (int) s.size() - 1) ? ""s : s.substr(offset, len);
    }

    std::string constructStr(int size, const std::function<char(int)> &charForIndex) {
        std::string s;
        s.resize(size);
        for (int i{}; i < size; i++)
            s[i] = charForIndex(i);
        return s;
    }

    void WriteLn() {
        std::cout << '\n';
    }

    void WriteLn(std::fstream *s) {
        *s << '\n';
    }

    std::string join(char sep, const std::initializer_list<std::string> &parts) {
        const int len = std::accumulate(parts.begin(), parts.end(), (int) parts.size() - 1,
                                        [](int acc, const std::string &s) -> int { return acc + (int) s.length(); });
        std::string res(len, sep);
        int i{};
        for (const std::string &part: parts) {
            for (int j{}; j < (int) part.length(); j++)
                res[i++] = part[j];
            if (i < len) i++;
        }
        return res;
    }

    bool starts_with(const std::string &s, const std::string &prefix) {
        if (prefix.length() > s.length()) return false;
        for (int i = 0; i < (int) prefix.length(); i++) {
            if (s[i] != prefix[i])
                return false;
        }
        return true;
    }

    bool ends_with(const std::string &s, const std::string &suffix) {
        if (suffix.length() > s.length()) return false;
        for (int i = 0; i < (int) suffix.length(); i++) {
            if (s[s.length() - 1 - i] != suffix[suffix.length() - 1 - i])
                return false;
        }
        return true;
    }

    std::string quoteWhitespace(const std::string &s, char quotechar) {
        return utils::strContains(s, ' ') ? ""s + quotechar + s + quotechar : s;
    }

    std::string quoteWhitespaceDir(const std::string &s, char sep, char quotechar) {
        if (!utils::strContains(s, ' ')) return s;
        std::string s2{};
        int ix{};
        for (const auto &part: utils::split(s, sep)) {
            if (ix++ > 0 || s.front() == sep) s2 += sep;
            s2 += utils::strContains(part, ' ') ? quotechar + part + quotechar : part;
        }
        return s.back() == sep ? s2 + sep : s2;
    }

    std::string doubleToString(double v, int width, int precision) {
        std::stringstream ss;
        ss.precision(precision);
        ss << std::fixed << v;
        std::string res = ss.str();
        return (int) res.length() >= width ? res : std::string(width - (int) res.length(), ' ') + res;
    }

    bool strToBool(const std::string &s) {
        if (s.empty() || s.length() > 4) return false;
        return utils::in(s, "1"s, "true"s, "on"s, "yes"s);
    }

    std::optional<std::list<BinaryDiffMismatch>>
    binaryFileDiff(const std::string &filename1, const std::string &filename2, int countLimit) {
        if (countLimit == -1) countLimit = std::numeric_limits<int>::max();
        std::ifstream f1{filename1, std::ios::binary}, f2{filename2, std::ios::binary};
        std::list<BinaryDiffMismatch> mismatches{};
        char c1, c2;
        uint64_t offset{};
        while (!f1.eof() && !f2.eof()) {
            f1.get(c1);
            f2.get(c2);
            if (c1 != c2) {
                mismatches.emplace_back(offset, c1, c2);
                if ((int) mismatches.size() >= countLimit)
                    break;
            }
            offset++;
        }
        return mismatches.empty() ? std::nullopt : std::make_optional(mismatches);
    }

    std::string asdelphifmt(double v, int precision) {
        std::stringstream ss;
        ss.precision(precision);
        ss << v;
        std::string s{replaceSubstrs(replaceSubstrs(ss.str(), "+", ""), "-0", "-")};
        replaceChar('e', 'E', s);
        return s;
    }

    void stocp(const std::string &s, char *cp) {
        std::memcpy(cp, s.c_str(), s.length() + 1);
    }

    inline int b2i(bool b) { return b ? 1 : 0; }

    int strCompare(const std::string &S1, const std::string &S2, bool caseInsensitive) {
        if (S1.empty() || S2.empty()) return b2i(!S1.empty()) - b2i(!S2.empty());
        auto L = S1.length();
        if (L > S2.length()) L = S2.length();
        for (size_t K{}; K < L; K++) {
            int c1 = caseInsensitive ? ::toupper(S1[K]) : S1[K];
            int c2 = caseInsensitive ? ::toupper(S2[K]) : S2[K];
            int d = c1 - c2;
            if (d) return d;
        }
        return static_cast<int>(S1.length() - S2.length());
    }

    StringBuffer::StringBuffer(int size) : s(size, '\0'), bufferSize{size} {}

    char *StringBuffer::getPtr() { return &s[0]; }

    std::string *StringBuffer::getStr() {
        s.resize(strlen(s.data()));
        return &s;
    }

    int StringBuffer::getBufferSize() const { return bufferSize; }

    BinaryDiffMismatch::BinaryDiffMismatch(uint64_t offset, uint8_t lhs, uint8_t rhs) : offset(offset), lhs(lhs),
                                                                                        rhs(rhs) {}

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
    int strConvCppToDelphi(const std::string &s, char *delphistr) {
        if (s.length() > std::numeric_limits<uint8_t>::max()) {
            const std::string errorMessage{"Error: Maximum short string length is 255 characters!"s};
            memcpy(&delphistr[1], errorMessage.c_str(), errorMessage.length() + 1);
            return static_cast<int>(errorMessage.length());
        }
        const auto l = static_cast<uint8_t>(s.length());
        delphistr[0] = static_cast<char>(l);
        memcpy(&delphistr[1], s.c_str(), l);
        return 0;
    }

    bool sameTextPChar(const char *a, const char *b, bool caseInvariant) {
        if (!a || !b) return !a && !b;
        for (int k{}; k < 256; k++) {
            if (a[k] == '\0' && b[k] == '\0') return true;
            if (std::tolower(a[k]) != std::tolower(b[k])) return false;
        }
        return false;
    }

}

// Needed for peak working set size query
#if defined(_WIN32)
#include <windows.h>
#include <Psapi.h>
#include <processthreadsapi.h>
#endif

namespace utils {
    int64_t queryPeakRSS() {
#if defined(_WIN32)
        PROCESS_MEMORY_COUNTERS info;
        if(!GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info)))
            return 0;
        return (int64_t)info.PeakWorkingSetSize;
#elif defined(__linux)
        std::ifstream ifs {"/proc/self/status"};
        if(!ifs.is_open()) return 0;
        std::string line;
        while(!ifs.eof()) {
            std::getline(ifs, line);
            if(utils::starts_with(line, "VmHWM")) {
                auto parts = utils::split(line);
                return std::stoi(utils::nth(parts, 1));
            }
        }
        return 0;
#elif defined(__APPLE__)
        return 0;
#endif
    }
}
