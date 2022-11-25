#include "strutilx.h"
#include <string>
#include <limits>
#include <cmath>
#include <cfloat>
#include <array>
#include <cassert>
#include <cstring>

#include "../utils.h"

#include "../rtl/sysutils_p3.h"
#include "../rtl/p3platform.h"

using namespace std::literals::string_literals;
using namespace rtl::sysutils_p3;
using namespace rtl::p3platform;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace gdlib::strutilx {

    const std::string MAXINT_S = "maxint"s, MININT_S = "minint"s;
    const std::string MAXDOUBLE_S = "maxdouble"s, EPSDOUBLE_S = "eps", MINDOUBLE_S = "mindouble";

	std::string UpperCase(const std::string& s) {
		std::string out = s;
        std::transform(s.begin(), s.end(), out.begin(), ::toupper);
        return out;
	}

	std::string LowerCase(const std::string& s) {
		std::string out = s;
        std::transform(s.begin(), s.end(), out.begin(), ::tolower);
        return out;
	}

	// Brief:
	//  Convert an integer to a string with leading blanks and thousands separators
	// Arguments:
	//  N: The number to be converted
	//  Width: Minimum total width of result
	// Returns:
	//  The converted number as a string
	std::string IntToNiceStrW(int N, int Width) {
        std::string s = std::to_string(N);
        if(s.length() < 4)
            return s.length() >= Width ? s : utils::strInflateWidth(N, Width, ' ');
        else {
            std::vector<char> outBuf(Width+1, ' ');
            outBuf.back() = '\0';
            int j{Width-1};
            for(int i{}; i<s.length(); i++) {
                if(i > 0 && i % 3 == 0)
                    outBuf[j--] = '.';
                outBuf[j--] = s[s.length()-i-1];
            }
            return outBuf.data();
        }
	}

    std::string IntToNiceStr(int N) {
        return IntToNiceStrW(N, 0);
    }

    std::string BlankStr(unsigned int Len) {
        return std::string(Len, ' ');
    }

    int StrExcelCol(const std::string &s) {
        int res{};
        for(int i{}; i<s.length(); i++) {
            int j {static_cast<int>(std::toupper(s[i]) - static_cast<int>('A'))};
            if(j < 0 || j > 25 || res >= std::numeric_limits<int>::max() / 26 + 26)
                return 0;
            res = res * 26 + j + 1;
        }
        return res;
    }

    std::string ExcelColStr(int C) {
        if(C <= 0) return {};
        std::string res;
        for(res.clear(); C; C /= 26)
            res += static_cast<char>(static_cast<int>('A')+(--C % 26));
        return res;
    }

    int IntegerWidth(int n) {
        int res = n >= 0 ? 0 : 1;
        if (res) n = -n;
        do {
            res++;
            n /= 10;
        } while (n);
        return res;
    }

    int PadModLength(const std::string& s, int M) {
        int res{ static_cast<int>(s.length()) };
        if (M > 0 && res % M != 0) res += M - (res % M);
        return res;
    }

    std::string PadRightMod(const std::string& s, int M) {
        return s + BlankStr(PadModLength(s, M) - s.length());
    }

    // Brief:
    //  Search for a character from the left from a starting position
    // Arguments:
    //  Ch: Character to search
    //  S: String to be searched
    //  Sp: Starting position
    // Returns:
    //  Location of the character when found; -1 otherwise
    int LChPosSp(char Ch, const std::string& S, int Sp) {
        if (Sp < 0) Sp = 0;
        for (int K{Sp}; K < S.length(); K++)
            if (S[K] == Ch) return K;
        return -1;
    }

    int LChPos(char Ch, const std::string& S) {
        return LChPosSp(Ch, S, 0);
    }

    // Brief:
    //  Search for a set of characters from the right
    // Arguments:
    //  Cs: Character set to search
    //  S: String to be searched
    // Returns:
    //  Location of the character when found; -1 otherwise
    int RChSetPos(const std::set<char>& Cs, const std::string& S) {
        for (int k{ static_cast<int>(S.length()-1) }; k >= 0; k--)
            if (Cs.find(S[k]) != Cs.end()) return k;
        return -1;
    }

    std::string quickDblToStr(double V) {
        const int precision = DBL_DIG - 1;
        std::string buf(1 + 1 + 1 + precision + 1 + 1 + 1 + 5 + 1, '\0');
        snprintf(buf.data(), buf.size(), "%.*e", precision, V);
        buf.resize(strlen(buf.data()));
        return buf;
    }

    // Closer port of corresponding Delphi function (faster?)
    // Brief:
    //   Convert a double to its string representation
    //   using the fullest precision.
    // Parameters:
    //   V: Value to be converted to a string
    // Returns:
    //   String representation of V
    std::string DblToStrSepClassic(double V, char DecimalSep) {
        if (!V) return "0"s;
        std::string s = quickDblToStr(V);
        if (V < 0.0) V = -V;
        auto k{ /*s.find_last_of("+-")*/ RChSetPos({'+', '-'}, s) };
        auto j{ /*s.find_first_of(".")*/ LChPos('.', s) };
        int i, e;
        if (V >= 1e-4 && V < 1e15) {
            utils::val(s.substr(k+1), e, i);
            for (i = k - 1; i < s.length(); i++) s[i] = '0';
            if (e >= 0) {
                for (i = j + 1; i <= j + e; i++) s[i - 1] = s[i];
                s[j + e] = DecimalSep;
                for (i = s.length()-1; i >= j + e + 1; i--) {
                    if (s[i] == '0') {
                        s[i] = ' ';
                        if (i == j + e + 1) s[j + e] = ' ';
                    }
                    else break;
                }
            }
            else {
                s[j] = s[j - 1];
                s[j - 1] = '0';
                e = -e;
                for (i = k - 2; i >= j; i--) s[i + e] = s[i];
                for (i = j + 1; i <= j + e - 1; i++) s[i] = '0';
                s[j] = DecimalSep;
                for (i = s.length(); i >= j + e + 1; i--) {
                    if (s[i] == '0') s[i] = ' ';
                    else break;
                }
            }
        }
        else {
            if (s[k] == '+') s[k] = ' ';
            for (i = k + 1; i < s.length(); i++) {
                if (s[i] == '0') {
                    s[i] = ' ';
                    if (i == s.length()) s[k - 1] = ' ';
                }
            }
            for (i = k - 2; i >= j + 1; i--) {
                if (s[i] == '0') {
                    s[i] = ' ';
                    if (i == j + 1) s[j] = ' ';
                }
                else break;
            }
        }
        // only with short strings
        s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
        return s;
    }

    std::string DblToStrSep(double V, char DecimalSep)
    {
        //return DblToStrSepClassic(V, DecimalSep);

        // FIXME: eval01 works with this line but it breaks eval08
        //std::string x = std::to_string(V);
        std::ostringstream ss;
        ss.precision(std::numeric_limits<double>::max_digits10);
        ss << V;
        std::string s = ss.str();
        // FIXME: Temporarily using a dirty hack to workaround the eval01 vs. eval08 issues
        if (utils::ends_with(s, "999")) s = std::to_string(V);
        utils::replaceChar('.', DecimalSep, s);
        utils::replaceChar(',', DecimalSep, s);
        s = utils::trimZeroesRight(s, DecimalSep);
        return s.back() == '.' || s.back() == DecimalSep || s.back() == ',' ? s.substr(0, s.length() - 1) : s;
    }

    std::string DblToStr(double V) {
        return DblToStrSep(V, '.');
    }

    void StrAssign(std::string& dest, const std::string& src) {
        dest = src;
    }

    bool StrAsIntEx(const std::string &s, int &v) {
        if (utils::sameText(s, MAXINT_S)) {
            v = std::numeric_limits<int>::max();
            return true;
        }
        if (utils::sameText(s, MININT_S)) {
            v = std::numeric_limits<int>::min();
            return true;
        }

        int k;
        utils::val(s, v, k);
        return !k;
    }

    bool SpecialStrAsInt(const std::string& s, int& v)
    {
        std::array<std::string, 3> specialStrs = {
            "off"s, "on"s, "silent"s
        };
        const auto it = std::find(specialStrs.begin(), specialStrs.end(), s);
        if (it != specialStrs.end()) {
            v = it - specialStrs.begin();
            return true;
        }
        return false;
    }

    std::string IncludeTrailingPathDelimiterEx(const std::string& S)
    {
        std::set<char> myDelim = { PathDelim };
        if (OSFileType() == OSFileWIN) myDelim.insert('/');
        return !S.empty() && utils::in(S.back(), myDelim) ? S : S+PathDelim;
    }

    std::string ExcludeTrailingPathDelimiterEx(const std::string &S) {
        std::set<char> myDelim = { PathDelim };
        if (OSFileType() == OSFileWIN) myDelim.insert('/');
        return !S.empty() && utils::in(S.back(), myDelim) ? S.substr(0, S.length()-1) : S;
    }

    std::string ExtractFileNameEx(const std::string& FileName) {
        return FileName.substr(LastDelimiter(""s + PathDelim + (OSFileType() == OSFileWIN ? "/" : "") + DriveDelim, FileName) + 1);
    }

    bool StrAsDoubleEx(const std::string &s, double &v) {
        if (utils::sameText(s, MAXDOUBLE_S)) {
            v = std::numeric_limits<double>::max();
            return true;
        }
        if (utils::sameText(s, MINDOUBLE_S)) {
            v = std::numeric_limits<double>::min();
            return true;
        }
        if (utils::sameText(s, EPSDOUBLE_S)) {
            v = std::numeric_limits<double>::epsilon();
            return true;
        }
        std::string ws = s;
        utils::replaceChar('D', 'E', ws);
        utils::replaceChar('d', 'E', ws);
        int k;
        utils::val(ws, v, k);
        if(std::isnan(v) || std::isinf(v)) return false;
        return !k;
    }

    bool StrAsIntEx2(const std::string &s, int &v) {
        bool res = StrAsIntEx(s, v);
        if(!res) {
            v = 0;
            double d;
            res = StrAsDoubleEx(s, d);
            if(res) {
                double intpart;
                res = d >= std::numeric_limits<int>::min() && d <= std::numeric_limits<int>::max() && std::modf(d, &intpart) == 0.0;
                if(res) v = static_cast<int>(trunc(d));
            }
        }
        return res;
    }

    // Brief:
    //  Compare two strings for equality ignoring case
    // Arguments:
    //  S1: First string
    //  S2: Second string
    // Returns:
    //  True if the strings are equal; False otherwise
    bool StrUEqual(const std::string &S1, const std::string &S2) {
        int L {static_cast<int>(S1.length())};
        if(L != S2.length()) return false;
        for(int K{L-1}; K>=0; K--) // significant stuff at the end?
            if(toupper(S1[K]) != toupper(S2[K])) return false;
        return true;
    }

    std::string ExtractFilePathEx(const std::string &FileName) {
        return FileName.substr(0, LastDelimiter(""s + PathDelim + (OSFileType() == OSFileWIN ? "/" : "") + DriveDelim, FileName) + 1);
    }

    std::string PadRight(const std::string &s, int W) {
        const int ww = std::min<int>(255, W) - s.length();
        return ww <= 0 ? s : s + std::string(ww, ' ');
    }

    std::string PadLeft(const std::string &s, int W) {
        const int ww = std::min<int>(255, W) - s.length();
        return ww <= 0 ? s : std::string(ww, ' ') + s;
    }

    // Brief:
    //   Extract the next token from a string
    // Arguments:
    //   s: String to extract token from
    //   p: Starting position of scan
    // Returns:
    //   Extracted token
    // Description:
    //   Scanning starts at position p; blanks are skipped.
    //   A token can be enclosed by a single or double quote character
    //   Such a quote character will be removed in the returned value.
    std::string ExtractToken(const std::string &s, int &p) {
        if(p <= 0) return ""s;
        const int L = s.length();
        // skip leading blanks
        while(p <= L && s[p] == ' ') p++;
        if(p > L) return ""s;
        char Stop;
        if(!utils::in(s[p], '\'', '\"')) Stop = ' ';
        else {
            Stop = s[p];
            p++;
        }
        int rs = p;
        while(p <= L && s[p] != Stop) p++;
        std::string res {s.substr(rs-1, p-rs)};
        if(p <= L && s[p] == Stop) p++;
        return res;
    }

    // Brief:
    //   Decode a string as an integer
    // Parameters:
    //   S: The string to be decoded
    // Returns:
    //   The integer value of the string, or zero when
    //   the string does not represent an integer
    // Note:
    //   could use a lot more error checking
    int StrAsInt(const std::string &s) {
        int k, res;
        utils::val(s, res, k);
        return k ? 0 : res;
    }

    std::string CompleteFileExtEx(const std::string& FileName, const std::string& Extension) {
        return ExtractFileExtEx(FileName).empty() ? ChangeFileExtEx(FileName, Extension) : FileName;
    }

    std::string ChangeFileExtEx(const std::string &FileName, const std::string &Extension) {
        int I {LastDelimiter(OSFileType() == rtl::p3platform::OSFileWIN ? "\\/:."s : "/."s, FileName) };
        return FileName.substr(0, I == -1 || FileName[I] != '.' ? static_cast<int>(FileName.length()) : I) + Extension;
    }

    std::string ExtractFileExtEx(const std::string &FileName) {
        int I {LastDelimiter(OSFileType() == rtl::p3platform::OSFileWIN ? "\\/:."s : "/."s, FileName) };
        return I >= 0 && FileName[I] == '.' ? FileName.substr(I) : ""s;
    }

    bool checkBOMOffset(const tBomIndic &potBOM, int &BOMOffset, std::string &msg) {
        enum tBOM { bUTF8, bUTF16BE, bUTF16LE, bUTF32BE, bUTF32LE, num_tboms };
        const std::array<std::string, num_tboms> BOMtxt = { "UTF8"s, "UTF16BE"s, "UTF16LE"s, "UTF32BE"s, "UTF32LE"s };
        const std::array<std::array<uint8_t, maxBOMLen+1>, num_tboms> BOMS = {
                {{3, 239, 187, 191, 0},  // UTF8
                 {2, 254, 255, 0, 0},  // UTF16BE
                 {2, 255, 254, 0, 0},  // UTF16LE
                 {4, 0, 0, 254, 255},  // UTF32BE
                 {4, 255, 254, 0, 0} // UTF32LE
                }
        };
        msg.clear();
        BOMOffset = 0;
        for(int b = 0; b<num_tboms; b++) {
            bool match{true};
            for(int j{1}; j<=BOMS[b][0]; j++) {
                if(BOMS[b][j] != potBOM[j-1]) {
                    match = false;
                    break;
                }
            }
            if(!match) continue;

            if(b == bUTF8) BOMOffset = BOMS[b].front(); // UTF8 is the only one, which is OK atm
            else {
                msg = BOMtxt[b] + " BOM detected. This is an unsupported encoding.";
                return false;
            }
            break;
        }
        return true;
    }

    // Brief:
    //  Replace a set of characters by another character
    // Arguments:
    //  ChSet: Set of character to be replaced
    //  New: Replacement character
    //  S: Source string
    // Returns:
    //  String with characters replaced
    std::string ReplaceChar(const std::set<char> &ChSet, char New, const std::string &S) {
        std::string out = S;
        for(char & i : out)
            if(utils::in(i, ChSet))
                i = New;
        return out;
    }

    // Brief:
    //  Replace every occurance of a string with another string
    // Arguments:
    //  Old: String to be replaced
    //  New: Replacement string
    //  S: Source string
    // Returns:
    //  String with substrings replaced
    std::string ReplaceStr(const std::string &Old, const std::string &New, const std::string &S) {
        return utils::replaceSubstrs(S, Old, New);
    }

    // Brief:
    //   Converts a file name to the short 8.3 form
    // Arguments:
    //   FileName: file/folder name to be converted
    // Returns:
    //   Converted name, empty string if the file or directory does not exist
    // Description:
    //   This function throws an exception if there was no conversion an the result
    //   contains a blank or an unicode character. Both can be problematic. (distinguish which should cause an error by argument?)
    //   see also http://blogs.msdn.com/b/winsdk/archive/2013/10/09/getshortpathname-doesn-t-return-short-path-name.aspx
    std::string ExtractShortPathNameExcept(const std::string &FileName) {
        std::string res {rtl::sysutils_p3::ExtractShortPathName(FileName)};
        for(char c : res) {
            if(static_cast<unsigned char>(c) >= 128) throw std::runtime_error("Problem extracting short path, result contains extended ASCII codes: "s + res + " (maybe 8.3 form is disabled)"s);
            if(c == ' ') throw std::runtime_error("Problem extracting short path, result contains spaces: "s + res + " (maybe 8.3 form is disabled)"s);
        }
        return res;
    }

    /**
     * PORTING NOTES FROM ANDRE
     * Pascal/Delphi convention: 1 byte is size/length/charcount, then character bytes, then quote byte END
     * C/C++ convention here: raw character bytes, null terminator \0, quote char after that
     * Doing the quote char after the terminator keeps strlen etc working
     **/

    // In-place conversion of C string (0 terminated suffix) to Pascal/Delphi string (size byte prefix)
    // Returns 1 iff. the C string exceeds maximum short string length of 255 characters
    int strConvCtoDelphi(char *cstr) {
        const auto len = strlen(cstr);
        if(len > std::numeric_limits<uint8_t>::max()) {
            const std::string errMsg { "Error: Maximum short string length is 255 characters!"s };
            cstr[0] = 0;
            memcpy(&cstr[1], errMsg.c_str(), errMsg.length() + 1);
            return strlen(&cstr[1]);
        }
        memmove(cstr+1, cstr, len);
        cstr[0] = static_cast<uint8_t>(len);
        return 0;
    }

    // In-place conversion of Pascal/Delphi string (size byte prefix) to C string (0 terminated suffix)
    void strConvDelphiToC(char *delphistr) {
        const auto len = static_cast<uint8_t>(delphistr[0]);
        memmove(delphistr, delphistr+1, len);
        delphistr[len] = '\0';
    }

    // Value-copy conversion of Pascal/Delphi string (size byte prefix) to C++ standard library (STL) string
    std::string strConvDelphiToCpp(const char *delphistr) {
        std::array<char, 256> buffer{};
        const auto len = static_cast<uint8_t>(delphistr[0]);
        for(int i=0; i<len; i++)
            buffer[i] = delphistr[i+1];
        buffer[len] = '\0';
        return std::string {buffer.data()};
    }

    // Convert C++ standard library string to Delphi short string
    int strConvCppToDelphi(const std::string &s, char *delphistr) {
        if(s.length() > std::numeric_limits<uint8_t>::max()) {
            const std::string errorMessage {"Error: Maximum short string length is 255 characters!"s};
            memcpy(&delphistr[1], errorMessage.c_str(), errorMessage.length()+1);
            return static_cast<int>(errorMessage.length());
        }
        const auto l = static_cast<uint8_t>(s.length());
        delphistr[0] = static_cast<char>(l);
        memcpy(&delphistr[1], s.c_str(), l);
        return 0;
    }

    bool PStrUEqual(const std::string &P1, const std::string &P2) {
        if(P1.empty() || P2.empty()) return P1.empty() && P2.empty();
        else {
            size_t L{P1.length()};
            if(L != P2.length()) return false;
            for(int K{static_cast<int>(L)-1}; K >= 0; K--) {
                if(std::toupper(P1[K]) != std::toupper(P2[K]))
                    return false;
            }
            return true;
        }
    }

    inline int b2i(bool b) { return b ? 1 : 0; }

    int PStrUCmp(const std::string &P1, const std::string &P2) {
        return !P1.empty() && !P2.empty() ? StrUCmp(P1, P2) : b2i(!P1.empty()) - b2i(!P2.empty());
    }

    int StrUCmp(const std::string &S1, const std::string &S2) {
        auto L = S1.length();
        if(L > S2.length()) L = S2.length();
        for(int K{}; K<L; K++) {
            int d = std::toupper(S1[K]) - std::toupper(S2[K]);
            if(d) return d;
        }
        return static_cast<int>(S1.length() - S2.length());
    }

    bool PStrEqual(const std::string &P1, const std::string &P2) {
        if(P1.empty() || P2.empty()) return P1.empty() && P2.empty();
        else {
            size_t L{P1.length()};
            if(L != P2.length()) return false;
            for(int K{static_cast<int>(L)-1}; K >= 0; K--) {
                if(P1[K] != P2[K])
                    return false;
            }
            return true;
        }
    }

}
