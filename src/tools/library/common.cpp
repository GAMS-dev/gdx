#include <array>
#include <cassert>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "common.hpp"
// Global constants
#include "generated/gclgms.h"

namespace library {

std::ostream &errorStream{std::cout};
// TODO: Possible improvement for later, but currently results in problems with the tests
// std::ostream &errorStream { std::cerr };

void printErrorMessage(const std::string &message) {
  errorStream << message << '\n';
}

void printErrorMessageWithError(const std::string &message) {
  errorStream << "Error: " << message << '\n';
}

// TODO: Make this a no-op in release mode builds (NDEBUG set)
// The assert checks in Delphi/P3 will be removed in debug builds
void assertWithMessage(const bool expression, const std::string &message) {
  if (!expression)
    printErrorMessage(message);
  assert(expression);
}

std::string gdxSpecialValuesStr(const int i) {
  if (i < 0 || i >= GMS_SVIDX_MAX) {
    assertWithMessage(false, "Unknown type");
    return "Unknown";
  } else
    return gmsSVText[i];
}

std::string gdxDataTypStr(const int i) {
  switch (i) {
  case 0:
    return "Set";
  case 1:
    return "Par";
  case 2:
    return "Var";
  case 3:
    return "Equ";
  case 4:
    return "Alias";
  default:
    assertWithMessage(false, "Unknown type");
    return "Unknown";
  }
}

std::string gdxDataTypStrL(const int i) {
  if (i < 0 || i >= GMS_DT_MAX) {
    assertWithMessage(false, "Unknown type");
    return "Unknown";
  } else
    return gmsGdxTypeText[i];
}

std::string valTypStr(const int i) {
  if (i < 0 || i >= GMS_VAL_MAX) {
    assertWithMessage(false, "Unknown type");
    return "Unknown";
  } else {
    constexpr std::array valsTypTxt{"L", "M", "LO", "UP", "SCALE"};
    return valsTypTxt[i];

    // std::string result { gmsValTypeText[i] };
    // result.erase( 0, 1 );
    // for( char &c: result ) c = static_cast<char>( toupper( c ) );
    // return result;
  }
}

std::string varTypStr(const int i) {
  if (i < 0 || i >= GMS_VARTYPE_MAX) {
    assertWithMessage(false, "Unknown type");
    return "Unknown";
  } else {
    constexpr std::array varsTypTxt{"unknown ", "binary  ", "integer ", "positive", "negative", "free    ", "sos1    ", "sos2    ", "semicont", "semiint "};
    return varsTypTxt[i];

    // return gmsVarTypeText[i];
  }
}

std::string specialValueStr(const int i) {
  if (i < 0 || i >= GMS_SVIDX_MAX) {
    assertWithMessage(false, "Unknown type");
    return "Unknown";
  } else {
    constexpr std::array svTxt{"Undf", "NA", "+Inf", "-Inf", "Eps", "0", "AcroN"};
    return svTxt[i];

    // return gmsSVText[i];
  }
}

std::vector<std::string> splitString(const std::string &string, const char delimiter) {
  std::vector<std::string> tokens;
  std::istringstream iss(string);
  std::string token;
  while (getline(iss, token, delimiter))
    tokens.emplace_back(token);
  return tokens;
}

bool canBeQuoted(const char *s, const size_t slen) {
  if (!s)
    return false;
  bool saw_single{}, saw_double{};
  for (int i{}; i < (int)slen; i++) {
    char Ch{s[i]};
    if (Ch == '\'') {
      if (saw_double)
        return false;
      saw_single = true;
    } else if (Ch == '\"') {
      if (saw_single)
        return false;
      saw_double = true;
    } else if (static_cast<unsigned char>(Ch) < ' ')
      return false;
  }
  return true;
}

bool goodUELString(const char *s, const size_t slen) {
  return slen < GLOBAL_UEL_IDENT_SIZE && canBeQuoted(s, slen);
}

void AuditLine::setAuditLine() {
  std::string
      GDL_REL_PLT,
      GDL_BLD_COD;
  bool auditreldates_header_file_found{};

  audit_line = system_name;
  audit_line.resize(17, ' ');

#if defined(_WIN32)
  GDL_REL_PLT = "x86 64bit/MS Windows";
  GDL_BLD_COD = "WEI";
#elif defined(__APPLE__)
#if !defined(__arm64__)
  GDL_REL_PLT = "x86 64bit/macOS";
  GDL_BLD_COD = "DEG";
#else
  GDL_REL_PLT = "arm 64bit/macOS";
  GDL_BLD_COD = "DAC";
#endif
#elif defined(__linux__)
#if !defined(__aarch64__)
  GDL_REL_PLT = "x86 64bit/Linux";
  GDL_BLD_COD = "LEG";
#else
  GDL_REL_PLT = "arm 64bit/Linux";
  GDL_BLD_COD = "LAG";
#endif
#endif

#if defined(__has_include)
#if __has_include("../../../../../../../btree/global/auditreldates.h")
#include "../../../../../../../btree/global/auditreldates.h"
  auditreldates_header_file_found = true;
  audit_line += std::to_string(GDL_REL_MAJ) + '.' +
                std::to_string(GDL_REL_MIN) + '.' +
                std::to_string(GDL_REL_GOLD) + ' ' +
                GDL_REVISION + ' ' + GDL_REL_DAT;
#endif
#endif

  if (!auditreldates_header_file_found) {
    std::time_t time{std::time(nullptr)};
    std::tm *localtime{std::localtime(&time)};

    std::stringstream date;
    date << std::put_time(localtime, "%b %d, %Y");

    audit_line += "??.?.? ???????? " + date.str();
  }

  audit_line.resize(55, ' ');
  audit_line += GDL_BLD_COD + ' ' + GDL_REL_PLT;
}

AuditLine::AuditLine(const std::string &system_name) {
  setSystemName(system_name);
}

void AuditLine::setSystemName(const std::string &system_name) {
  this->system_name = system_name;
  setAuditLine();
}

std::string AuditLine::getAuditLine() const {
  return audit_line;
}

} // namespace library
