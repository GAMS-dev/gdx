#pragma once
#include <string>
#include <array>
#include <set>

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace gdlib::strutilx{
	std::string UpperCase(const std::string &s);
	std::string LowerCase(const std::string &s);

	std::string IntToNiceStrW(int N, int Width);
    std::string IntToNiceStr(int N);
    std::string BlankStr(unsigned int Len);

    // Excel column names
    int StrExcelCol(const std::string &s);
    std::string ExcelColStr(int C);

    int IntegerWidth(int n);

    int PadModLength(const std::string& s, int M);

    std::string PadRightMod(const std::string& s, int M);

    std::string DblToStrSep(double V, char DecimalSep);

    std::string DblToStr(double V);

    std::string DblToStrSepClassic(double V, char DecimalSep);

    void StrAssign(std::string& dest, const std::string& src);

    bool StrAsDoubleEx(const std::string &s, double &v);
    bool StrAsIntEx2(const std::string &s, int &v);

    bool StrAsIntEx(const std::string& s, int& v);
    bool SpecialStrAsInt(const std::string& s, int& v);

    bool StrUEqual(const std::string &S1, const std::string &S2);

    std::string IncludeTrailingPathDelimiterEx(const std::string& S);
    std::string ExcludeTrailingPathDelimiterEx(const std::string &S);

    std::string ExtractFileNameEx(const std::string& FileName);

    std::string ExtractFilePathEx(const std::string &FileName);

    std::string PadRight(const std::string &s, int W);
    std::string PadLeft(const std::string &s, int W);

    std::string ExtractToken(const std::string &s, int &p);

    int StrAsInt(const std::string &s);

    std::string ChangeFileExtEx(const std::string &FileNamne, const std::string &Extension);
    std::string CompleteFileExtEx(const std::string& FileName, const std::string& Extension);
    std::string ExtractFileExtEx(const std::string &FileName);

    const int maxBOMLen = 4;
    using tBomIndic = std::array<uint8_t, maxBOMLen>;
    bool checkBOMOffset(const tBomIndic &potBOM, int &BOMOffset, std::string &msg);

    std::string ReplaceChar(const std::set<char> &ChSet, char New, const std::string &S);

    std::string ReplaceStr(const std::string &substr, const std::string &replacement, const std::string &s);

    std::string ExtractShortPathNameExcept(const std::string &FileName);

}
