#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "short_string.hpp"

// Description:
//   Object that handles command line parameters

namespace library::cmdpar {

// Parameter record
struct ParamRec {
  int Key{};        // key number
  std::string KeyS; // key value
};
using PParamRec = ParamRec *;

// Class to handle command line parameters
class CmdParams {
  std::vector<std::pair<std::string, int>> FKeyList;
  std::map<int, ParamRec> FParList;

  void ClearParams();
  void AddVS(int v, const std::string &s);
  int FindKeyV(int V);

public:
  bool CrackCommandLine(int ParamCount, const char *ParamStr[]);
  bool AddParameters(int AInsP, const std::string &CmdLine, int ParamCount, const char *ParamStr[]);
  void AddKeyWord(int v, const std::string &s);
  void AddParam(int v, const std::string &s);
  bool HasParam(int v, library::ShortString &s);
  bool HasKey(int v);

  ParamRec GetParams(int n);
  [[nodiscard]] int GetParamCount() const;
  [[nodiscard]] std::string GetParamText(int key) const;
};

enum class CmdParamStatus : int16_t {
  kp_input,
  ke_empty = -1,
  ke_unknown = -2,
  ke_noparam = -3,
  ke_badfile = -4,
  kk_big = 10000
};

} // namespace library::cmdpar
