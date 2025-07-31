#ifndef GDX_GDXDUMP_H
#define GDX_GDXDUMP_H

#include <array>
#include <cstdint>
#include <functional>
#include <string>

#include "library/common.hpp"

namespace gdxdump {

enum class OutFormat : uint8_t {
  fmt_none,
  fmt_normal,
  fmt_gamsbas,
  fmt_csv
};

enum class DblFormat : uint8_t {
  dbl_none,
  dbl_normal,
  dbl_hexponential,
  dbl_hexBytes
};

char QQ(const std::string &s);

std::string QQCSV(const std::string &s);

// Function is commented out in the Delphi source code
// std::string QUEL(const std::string &S);

// Auxiliary function for WriteQText and WriteQUELText
void WriteQuotedCommon(const std::string &S, const std::function<bool(char)> &isSpecialPredicate, int i = {}, bool G = true);

void WriteQText(const std::string &S, bool checkParenthesis);

void WriteQUELText(const std::string &S);

void WriteUEL(const std::string &S);

void WriteUELTable(const std::string &name);

void WrVal(double V);

std::string GetUELAsString(int N);

std::string GetUel4CSV(int N);

bool WriteSymbolAsItem(int SyNr, bool DomInfo);

void WriteSymbolsAsSet(bool DomInfo);

void WriteSymbol(int SyNr);

void WriteSymbolCSV(int SyNr);

void WriteSymbolInfo();

void WriteDomainInfo();

void WriteSetText();

void Usage(const library::AuditLine &auditLine);

std::string NextParam();

void WriteAcronyms();

int main(int argc, const char *argv[]);

} // namespace gdxdump

#endif // GDX_GDXDUMP_H
