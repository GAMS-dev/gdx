/**
 * GAMS - General Algebraic Modeling System C++ API
 *
 * Copyright (c) 2017-2025 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2025 GAMS Development Corp. <support@gams.com>
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

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>

#include "gdlib/gmsobj.hpp"
#include "gdlib/strhash.hpp"
#include "gdlib/strutilx.hpp"
#include "gdlib/utils.hpp"
#include "gdxdiff.hpp"
#include "library/cmdpar.hpp"
#include "rtl/p3process.hpp"
#include "rtl/sysutils_p3.hpp"

// Global constants
#include "generated/gclgms.h"

// Increase value to use
#define VERBOSE 0

namespace gdxdiff {

using tvarvaltype = uint8_t;

library::ShortString DiffTmpName;
gdxHandle_t PGX1, PGX2, PGXDIF;
bool diffUELsRegistered;
// TODO: Use the correct type instead of nullptr type?
std::unique_ptr<gdlib::strhash::TXStrHashList<std::nullptr_t>> UELTable;
int staticUELNum;
double EpsAbsolute, EpsRelative;
std::map<library::ShortString, StatusCode> StatusTable;
std::unique_ptr<library::cmdpar::CmdParams> CmdParams;
utils::bsSet<tvarvaltype, GMS_VAL_MAX> ActiveFields;
FldOnly fldOnly;
tvarvaltype fldOnlyFld;
bool DiffOnly, CompSetText, matrixFile, ignoreOrder;
std::unique_ptr<gdlib::gmsobj::TXStrings> IDsOnly;
std::unique_ptr<gdlib::gmsobj::TXStrings> SkipIDs;
bool ShowDefRec, CompDomains;

std::string ValAsString(const gdxHandle_t &PGX, const double V) {
  constexpr int WIDTH{14};
  library::ShortString result;
  if (gdxAcronymName(PGX, V, result.data()) == 0) {
    int iSV;
    gdxMapValue(PGX, V, &iSV);
    if (iSV != sv_normal)
      return gdlib::strutilx::PadLeft(library::gdxSpecialValuesStr(iSV), WIDTH);
    else {
      // TODO: Improve this conversion
      std::ostringstream oss;
      oss << std::fixed << std::setprecision(5) << V;
      result = oss.str();
      return gdlib::strutilx::PadLeft(result, WIDTH);
    }
  }
  // Empty string will be returned
  return {};
}

void FatalErrorExit(const int ErrNr) {
  if (!DiffTmpName.empty() && rtl::sysutils_p3::FileExists(DiffTmpName)) {
    if (PGXDIF) {
      gdxClose(PGXDIF);
      gdxFree(&PGXDIF);
    }
    rtl::sysutils_p3::DeleteFileFromDisk(DiffTmpName);
  }
  std::exit(ErrNr);
}

void FatalError(const std::string &Msg, const int ErrNr) {
  library::printErrorMessage("GDXDIFF error: " + Msg);
  FatalErrorExit(ErrNr);
}

void FatalError2(const std::string &Msg1, const std::string &Msg2, const int ErrNr) {
  library::printErrorMessage("GDXDIFF error: " + Msg1);
  library::printErrorMessage("               " + Msg2);
  FatalErrorExit(ErrNr);
}

void CheckGDXError(const gdxHandle_t &PGX) {
  int ErrNr{gdxGetLastError(PGX)};
  if (ErrNr != 0) {
    library::ShortString S;
    gdxErrorStr(PGX, ErrNr, S.data());
    library::printErrorMessage("GDXDIFF GDX Error: " + S);
  }
}

void OpenGDX(const std::string &fn, gdxHandle_t &PGX) {
  if (!rtl::sysutils_p3::FileExists(fn))
    FatalError("Input file not found " + fn, static_cast<int>(ErrorCode::ERR_NOFILE));

  library::ShortString S;
  if (!gdxCreate(&PGX, S.data(), S.length()))
    FatalError("Cannot load GDX library " + S, static_cast<int>(ErrorCode::ERR_LOADDLL));

  int ErrNr;
  gdxOpenRead(PGX, fn.data(), &ErrNr);
  if (ErrNr != 0) {
    gdxErrorStr(PGX, ErrNr, S.data());
    FatalError2("Problem reading GDX file + " + fn, S, static_cast<int>(ErrorCode::ERR_READGDX));
  }

  int NrElem, HighV;
  gdxUMUelInfo(PGX, &NrElem, &HighV);
  gdxUELRegisterMapStart(PGX);
  for (int N{1}; N <= NrElem; N++) {
    int NN;
    library::ShortString UEL;
    gdxUMUelGet(PGX, N, UEL.data(), &NN);
    NN = UELTable->Add(UEL.data(), UEL.length());
    gdxUELRegisterMap(PGX, NN, UEL.data());
  }
  gdxUELRegisterDone(PGX);
  CheckGDXError(PGX);
}

void RegisterDiffUELs() {
  if (diffUELsRegistered)
    return;

  const int maxUEL{ignoreOrder ? staticUELNum : UELTable->Count()};

  gdxUELRegisterStrStart(PGXDIF);
  int d;
  for (int N{}; N < maxUEL; N++)
    gdxUELRegisterStr(PGXDIF, UELTable->GetString(N), &d);
  gdxUELRegisterDone(PGXDIF);
  CheckGDXError(PGXDIF);

  diffUELsRegistered = true;
}

void CompareSy(const int Sy1, const int Sy2) {
  int Dim, VarEquType;
  gdxSyType ST;
  library::ShortString ID;
  bool SymbOpen{};
  StatusCode Status;
  gdxValues_t DefValues{};

  auto CheckSymbOpen = [&]() -> bool {
    RegisterDiffUELs();
    if (Status == StatusCode::sc_dim10)
      Status = StatusCode::sc_dim10_diff;
    if (!SymbOpen && Status != StatusCode::sc_dim10_diff) {
      if (fldOnly == FldOnly::fld_yes && (ST == dt_var || ST == dt_equ)) {
        library::ShortString ExplTxt{"Differences Field = " + GamsFieldNames[fldOnlyFld]};
        gdxDataWriteStrStart(PGXDIF, ID.data(), ExplTxt.data(), Dim + 1, static_cast<int>(dt_par), 0);
      } else if (DiffOnly && (ST == dt_var || ST == dt_equ))
        gdxDataWriteStrStart(PGXDIF, ID.data(), "Differences Only", Dim + 2, static_cast<int>(dt_par), 0);
      else
        gdxDataWriteStrStart(PGXDIF, ID.data(), "Differences", Dim + 1, static_cast<int>(ST), VarEquType);
      SymbOpen = true;
    }
    return SymbOpen;
  };

  auto SymbClose = [&]() -> void {
    if (SymbOpen) {
      SymbOpen = false;
      CheckGDXError(PGXDIF);
      gdxDataWriteDone(PGXDIF);
      CheckGDXError(PGXDIF);
    }
  };

  auto WriteDiff = [&](const std::string &Act, const std::string &FldName, const gdxUelIndex_t &Keys, const gdxValues_t &Vals) -> void {
    gdxStrIndex_t StrKeys{};
    gdxStrIndexPtrs_t StrKeysPtrs;
    GDXSTRINDEXPTRS_INIT(StrKeys, StrKeysPtrs);

    gdxValues_t Vals2{};

    RegisterDiffUELs();
    for (int D{}; D < Dim; D++)
      // TODO: Improve this check (especially the else case)
      if (Keys[D] < UELTable->Count())
        strcpy(StrKeysPtrs[D], UELTable->GetString(Keys[D]));
      else
        snprintf(StrKeysPtrs[D], GMS_SSSIZE, "L__%d", Keys[D]);
    if (!(DiffOnly && (ST == dt_var || ST == dt_equ)))
      strcpy(StrKeysPtrs[Dim], Act.data());
    else {
      strcpy(StrKeysPtrs[Dim], FldName.data());
      strcpy(StrKeysPtrs[Dim + 1], Act.data());
    }

#if VERBOSE >= 3
    for (int D{}; D < Dim + 1; D++) {
      std::cout << StrKeys[D];
      if (D < Dim + 1)
        std::cout << ", ";
    }
    std::cout << '\n';
#endif

    if (fldOnly == FldOnly::fld_yes && (ST == dt_var || ST == dt_equ)) {
      Vals2[GMS_VAL_LEVEL] = Vals[fldOnlyFld];
      gdxDataWriteStr(PGXDIF, const_cast<const char **>(StrKeysPtrs), Vals2);
    } else
      gdxDataWriteStr(PGXDIF, const_cast<const char **>(StrKeysPtrs), Vals);
  };

  auto WriteSetDiff = [&](const std::string &Act, const gdxUelIndex_t &Keys, const library::ShortString &S) -> void {
    gdxStrIndex_t StrKeys{};
    gdxStrIndexPtrs_t StrKeysPtrs;
    GDXSTRINDEXPTRS_INIT(StrKeys, StrKeysPtrs);

    gdxValues_t Vals{};
    int iNode;

    RegisterDiffUELs();
    for (int D{}; D < Dim; D++)
      strcpy(StrKeysPtrs[D], UELTable->GetString(Keys[D]));
    strcpy(StrKeysPtrs[Dim], Act.data());
    gdxAddSetText(PGXDIF, S.data(), &iNode);
    Vals[GMS_VAL_LEVEL] = iNode;
    gdxDataWriteStr(PGXDIF, const_cast<const char **>(StrKeysPtrs), Vals);
  };

#if VERBOSE >= 2
  auto WriteValues = [&](const gdxHandle_t &PGX, const gdxValues_t &Vals) -> void {
    switch (ST) {
    case dt_set:
      library::assertWithMessage(false, "Should not be called");
      break;

    case dt_par:
      std::cout << ValAsString(PGX, Vals[GMS_VAL_LEVEL]) << '\n';
      break;

    default:
      for (int T{}; T < GMS_VAL_MAX; T++)
        std::cout << ValAsString(PGX, Vals[T]) << ' ';
      std::cout << '\n';
      break;
    }
  };

  auto WriteKeys = [&](const gdxUelIndex_t &Keys) -> void {
    RegisterDiffUELs();
    for (int D{}; D < Dim; D++) {
      std::cout << ' ' << UELTable->GetString(Keys[D]);
      if (D < Dim)
        std::cout << " .";
    }
  };
#endif

  auto DoublesEqual = [](const double V1, const double V2) -> bool {
    auto DMin = [](const double a, const double b) -> double {
      return a <= b ? a : b;
    };

    // auto DMax = []( const double a, const double b ) -> double {
    //    if( a >= b )
    //       return a;
    //    else
    //       return b;
    // };

    int iSV1, iSV2;
    gdxMapValue(PGX1, V1, &iSV1);
    gdxMapValue(PGX2, V2, &iSV2);

    bool result;
    library::ShortString S1, S2;
    double AbsDiff;
    if (iSV1 == sv_normal) {
      if (iSV2 == sv_normal) {
        if (gdxAcronymName(PGX1, V1, S1.data()) != 0 && gdxAcronymName(PGX2, V2, S2.data()) != 0)
          result = gdlib::strutilx::StrUEqual(S1.data(), S2.data());
        else {
          AbsDiff = std::abs(V1 - V2);
          if (AbsDiff <= EpsAbsolute)
            result = true;
          else if (EpsRelative > 0)
            result = AbsDiff / (1 + DMin(std::abs(V1), std::abs(V2))) <= EpsRelative;
          else
            result = false;
        }
      } else
        result = iSV2 == sv_valeps && EpsAbsolute > 0 && std::abs(V1) <= EpsAbsolute;
    } else {
      if (iSV2 == sv_normal)
        result = iSV1 == sv_valeps && EpsAbsolute > 0 && std::abs(V2) <= EpsAbsolute;
      else
        result = iSV1 == iSV2;
    }
    return result;
  };

  auto CheckParDifference = [&](const gdxUelIndex_t &Keys, const gdxValues_t &V1, const gdxValues_t &V2) -> bool {
    bool result{true};
    if (ST == dt_par)
      result = DoublesEqual(V1[GMS_VAL_LEVEL], V2[GMS_VAL_LEVEL]);
    else if (fldOnly == FldOnly::fld_yes)
      result = DoublesEqual(V1[fldOnlyFld], V2[fldOnlyFld]);
    else {
      for (int T{}; T < GMS_VAL_MAX; T++) {
        if (ActiveFields.contains(static_cast<tvarvaltype>(T)) && !DoublesEqual(V1[T], V2[T])) {
          result = false;
          break;
        }
      }
    }
    if (!result) {
#if VERBOSE >= 2
      std::cout << "Different ";
      WriteKeys(Keys);
      std::cout << '\n';
      WriteValues(PGX1, V1);
      WriteValues(PGX2, V2);
#endif

      if (!CheckSymbOpen())
        return {};
      if (!(DiffOnly && (ST == dt_var || ST == dt_equ))) {
        WriteDiff(c_dif1, {}, Keys, V1);
        WriteDiff(c_dif2, {}, Keys, V2);
      } else {
        for (int T{}; T < GMS_VAL_MAX; T++) {
          if (!ActiveFields.contains(static_cast<tvarvaltype>(T)))
            continue;
          if (DoublesEqual(V1[T], V2[T]))
            continue;

          gdxValues_t Vals{};
          Vals[GMS_VAL_LEVEL] = V1[T];
          WriteDiff(c_dif1, GamsFieldNames[T], Keys, Vals);
          Vals[GMS_VAL_LEVEL] = V2[T];
          WriteDiff(c_dif2, GamsFieldNames[T], Keys, Vals);
        }
      }
    }
    return result;
  };

  auto CheckSetDifference = [&](const gdxUelIndex_t &Keys, const int txt1, const int txt2) -> bool {
    library::ShortString S1, S2;
    int iNode;
    if (txt1 == 0)
      S1.clear();
    else
      gdxGetElemText(PGX1, txt1, S1.data(), &iNode);
    if (txt2 == 0)
      S2.clear();
    else
      gdxGetElemText(PGX2, txt2, S2.data(), &iNode);

    bool result{S1 == S2};
    if (!result) {
#if VERBOSE >= 2
      std::cout << "Associated text is different ";
      WriteKeys(Keys);
      std::cout << '\n'
                << S1 << '\n'
                << S2 << '\n';
#endif

      if (!CheckSymbOpen())
        return {};
      WriteSetDiff(c_dif1, Keys, S1);
      WriteSetDiff(c_dif2, Keys, S2);
    }
    return result;
  };

  auto ShowInsert = [&](const std::string &Act, const gdxUelIndex_t &Keys, gdxValues_t &Vals) -> void {
    // We check if this insert has values we want to ignore
    bool Eq{};
    switch (ST) {
    case dt_par:
      Eq = DoublesEqual(Vals[GMS_VAL_LEVEL], 0);
      break;

    case dt_var:
    case dt_equ:
      Eq = true;
      for (int T{}; T < GMS_VAL_MAX; T++) {
        if (ActiveFields.contains(static_cast<tvarvaltype>(T)) && !DoublesEqual(Vals[T], DefValues[T])) {
          Eq = false;
          break;
        }
      }
      break;

    default:
      // TODO: Print error message?
      break;
    }

    if (Eq && !ShowDefRec)
      return;

    if (Status == StatusCode::sc_same)
      Status = StatusCode::sc_key;
    if (Status == StatusCode::sc_dim10)
      Status = StatusCode::sc_dim10_diff;

    if (Status == StatusCode::sc_dim10_diff || !CheckSymbOpen())
      return;

#if VERBOSE >= 2
    std::cout << "Insert: " << Act << ' ';
#endif

    if (ST == dt_set && Vals[GMS_VAL_LEVEL] != 0) {
      library::ShortString stxt;
      int N;
      gdxGetElemText(
          Act == c_ins1 ? PGX1 : PGX2,
          utils::round<int>(Vals[GMS_VAL_LEVEL]), stxt.data(), &N);
      gdxAddSetText(PGXDIF, stxt.data(), &N);
      Vals[GMS_VAL_LEVEL] = N;
    }

    if (!(DiffOnly && (ST == dt_var || ST == dt_equ)))
      WriteDiff(Act, {}, Keys, Vals);
    else {
      gdxValues_t Vals2{};
      for (int T{}; T < GMS_VAL_MAX; T++) {
        if (!ActiveFields.contains(static_cast<tvarvaltype>(T)))
          continue;
        Vals2[GMS_VAL_LEVEL] = Vals[T];
        WriteDiff(Act, GamsFieldNames[T], Keys, Vals2);
      }
    }
  };

  int Dim2, AFDim, iST, iST2, R1Last, R2Last, C, acount;
  gdxSyType ST2;
  bool Flg1, Flg2, Eq, DomFlg;
  gdxUelIndex_t Keys1{}, Keys2{};
  gdxValues_t Vals1{}, Vals2{};
  library::ShortString stxt;

  gdxStrIndex_t DomSy1{};
  gdxStrIndexPtrs_t DomSy1Ptrs;
  GDXSTRINDEXPTRS_INIT(DomSy1, DomSy1Ptrs);

  gdxStrIndex_t DomSy2{};
  gdxStrIndexPtrs_t DomSy2Ptrs;
  GDXSTRINDEXPTRS_INIT(DomSy2, DomSy2Ptrs);

  SymbOpen = false;
  gdxSymbolInfo(PGX1, Sy1, ID.data(), &Dim, &iST);
  ST = static_cast<gdxSyType>(iST);
  if (ST == dt_alias)
    ST = dt_set;
  // We do nothing with type in file2
  gdxSymbolInfoX(PGX1, Sy1, &acount, &VarEquType, stxt.data());

  gdxSymbolInfo(PGX2, Sy2, ID.data(), &Dim2, &iST2);
  ST2 = static_cast<gdxSyType>(iST2);
  if (ST2 == dt_alias)
    ST2 = dt_set;
  Status = StatusCode::sc_same;

  if (Dim != Dim2 || ST != ST2) {
    std::cout << "*** symbol = " << ID << " cannot be compared\n";
    if (ST != ST2) {
      std::cout << "Typ1 = " << library::gdxDataTypStrL(ST) << ", Typ2 = " << library::gdxDataTypStrL(ST2) << '\n';
      Status = StatusCode::sc_typ;
    }
    if (Dim != Dim2) {
      std::cout << "Dim1 = " << Dim << ", Dim2 = " << Dim2 << '\n';
      if (Status == StatusCode::sc_same)
        Status = StatusCode::sc_dim;
    }
    goto label999;
  }

  // Check domains
  if (CompDomains && Dim > 0) {
    gdxSymbolGetDomainX(PGX1, Sy1, DomSy1Ptrs);
    gdxSymbolGetDomainX(PGX2, Sy2, DomSy2Ptrs);
    DomFlg = false;
    for (int D{}; D < Dim; D++)
      if (!gdlib::strutilx::StrUEqual(DomSy1[D], DomSy2[D])) {
        DomFlg = true;
        break;
      }
    if (DomFlg) {
      Status = StatusCode::sc_domain;

#if VERBOSE >= 1
      std::cout << "Domain differences for symbol = " << ID << '\n';
      for (int D{}; D < Dim; D++) {
        if (gdlib::strutilx::StrUEqual(DomSy1[D], DomSy2[D]))
          continue;
        std::cout << gdlib::strutilx::PadRight(std::to_string(D), 2) << ' ' << DomSy1[D] << ' ' << DomSy2[D] << '\n';
      }
      std::cout << '\n';
#endif

      goto label999;
    }
  }

#if VERBOSE >= 1
  std::cout << library::gdxDataTypStrL(ST) << ' ' << ID << '\n';
#endif

  // Create default record for this type
  if (ST == dt_var || ST == dt_equ) {
    if (ST == dt_var)
      std::copy(std::begin(gmsDefRecVar[VarEquType]), std::end(gmsDefRecVar[VarEquType]), std::begin(DefValues));
    else
      std::copy(std::begin(gmsDefRecEqu[VarEquType]), std::end(gmsDefRecEqu[VarEquType]), std::begin(DefValues));
  }

  if (Dim >= GMS_MAX_INDEX_DIM || (DiffOnly && Dim - 1 >= GMS_MAX_INDEX_DIM))
    Status = StatusCode::sc_dim10;

  if (matrixFile) {
    Flg1 = gdxDataReadRawStart(PGX1, Sy1, &R1Last) != 0;
    if (Flg1)
      Flg1 = gdxDataReadRaw(PGX1, Keys1, Vals1, &AFDim) != 0;

    Flg2 = gdxDataReadRawStart(PGX2, Sy2, &R2Last) != 0;
    if (Flg2)
      Flg2 = gdxDataReadRaw(PGX2, Keys2, Vals2, &AFDim) != 0;
  } else {
    Flg1 = gdxDataReadMapStart(PGX1, Sy1, &R1Last) != 0;
    if (Flg1)
      Flg1 = gdxDataReadMap(PGX1, 0, Keys1, Vals1, &AFDim) != 0;

    Flg2 = gdxDataReadMapStart(PGX2, Sy2, &R2Last) != 0;
    if (Flg2)
      Flg2 = gdxDataReadMap(PGX2, 0, Keys2, Vals2, &AFDim) != 0;
  }

  while (Flg1 && Flg2) {
    C = 0;
    if (Dim > 0) {
      for (int D{}; D < Dim; D++) {
        C = Keys1[D] - Keys2[D];
        if (C != 0)
          break;
      }
    }
    if (C == 0) {
      if (ST == dt_set) {
        if (CompSetText)
          Eq = CheckSetDifference(
              Keys1,
              utils::round<int>(Vals1[GMS_VAL_LEVEL]),
              utils::round<int>(Vals2[GMS_VAL_LEVEL]));
        else
          Eq = true;
      } else
        Eq = CheckParDifference(Keys1, Vals1, Vals2);

      if (!Eq && Status == StatusCode::sc_same)
        Status = StatusCode::sc_data;

      if (matrixFile) {
        Flg1 = gdxDataReadRaw(PGX1, Keys1, Vals1, &AFDim) != 0;
        Flg2 = gdxDataReadRaw(PGX2, Keys2, Vals2, &AFDim) != 0;
      } else {
        Flg1 = gdxDataReadMap(PGX1, 0, Keys1, Vals1, &AFDim) != 0;
        Flg2 = gdxDataReadMap(PGX2, 0, Keys2, Vals2, &AFDim) != 0;
      }
    }
    // Change in status happens inside ShowInsert
    else if (C < 0) {
      ShowInsert(c_ins1, Keys1, Vals1);
      if (matrixFile)
        Flg1 = gdxDataReadRaw(PGX1, Keys1, Vals1, &AFDim) != 0;
      else
        Flg1 = gdxDataReadMap(PGX1, 0, Keys1, Vals1, &AFDim) != 0;
    } else {
      ShowInsert(c_ins2, Keys2, Vals2);
      if (matrixFile)
        Flg2 = gdxDataReadRaw(PGX2, Keys2, Vals2, &AFDim) != 0;
      else
        Flg2 = gdxDataReadMap(PGX2, 0, Keys2, Vals2, &AFDim) != 0;
    }
  }

  while (Flg1) {
    ShowInsert(c_ins1, Keys1, Vals1);
    if (matrixFile)
      Flg1 = gdxDataReadRaw(PGX1, Keys1, Vals1, &AFDim) != 0;
    else
      Flg1 = gdxDataReadMap(PGX1, 0, Keys1, Vals1, &AFDim) != 0;
  }

  while (Flg2) {
    ShowInsert(c_ins2, Keys2, Vals2);
    if (matrixFile)
      Flg2 = gdxDataReadRaw(PGX2, Keys2, Vals2, &AFDim) != 0;
    else
      Flg2 = gdxDataReadMap(PGX2, 0, Keys2, Vals2, &AFDim) != 0;
  }

  SymbClose();

label999:
  if (!(Status == StatusCode::sc_same || Status == StatusCode::sc_dim10))
    StatusTable.insert({ID, Status});
}

bool GetAsDouble(const library::ShortString &S, double &V) {
  int k;
  utils::val(S, V, k);
  bool result{k == 0 && V >= 0};
  if (!result)
    V = 0;
  return result;
}

void Usage(const library::AuditLine &auditLine) {
  std::cout
      << "gdxdiff: GDX file differ\n"
      << auditLine.getAuditLine()
      << "\n\nUsage: \n"
         "   gdxdiff file1.gdx file2.gdx [diffile.gdx] [options]\n"
         "   Options:\n"
         "      Eps     = Val       epsilon for comparison\n"
         "      RelEps  = Val       epsilon for relative comparison\n"
         "      Field   = gamsfield (L, M, Up, Lo, Prior, Scale or All)\n"
         "      FldOnly             write var or equ as parameter for selected field\n"
         "      DiffOnly            write var or equ as parameter with field as an extra dimension\n"
         "      CmpDefaults         compare default values\n"
         "      CmpDomains          compare domains\n"
         "      MatrixFile          compare GAMS matrix files in GDX format\n"
         "      IgnoreOrder         ignore UEL order of input files - reduces size of output file\n"
         "      SetDesc = Y|N       compare explanatory texts for set elements, activated by default (=Y)\n"
         "      ID      = one or more identifiers; only ids listed will be compared\n"
         "      SkipID  = one or more identifiers; ids listed will be skipped\n"
         "   The .gdx file extension is the default\n";
}

// Function is empty in Delphi code
// void CopyAcronyms( const gdxHandle_t &PGX ) {}

void CheckFile(std::string &fn) {
  if (!rtl::sysutils_p3::FileExists(fn) && gdlib::strutilx::ExtractFileExtEx(fn).empty())
    fn = gdlib::strutilx::ChangeFileExtEx(fn, ".gdx");
}

int main(const int argc, const char *argv[]) {
  int ErrorCode, ErrNr, Dim, iST, StrNr;
  library::ShortString S, ID, DiffFileName;
  std::string InFile1, InFile2;
  std::map<library::ShortString, int> IDTable;
  bool UsingIDE, RenameOK;

  gdxStrIndex_t StrKeys{};
  gdxStrIndexPtrs_t StrKeysPtrs;
  GDXSTRINDEXPTRS_INIT(StrKeys, StrKeysPtrs);

  gdxValues_t StrVals{};

  library::AuditLine auditLine{"GDXDIFF"};
  if (argc > 1 && gdlib::strutilx::StrUEqual(argv[1], "AUDIT")) {
    std::cout << auditLine.getAuditLine() << '\n';
    return 0;
  }

  // So we can check later
  DiffTmpName.clear();

  CmdParams = std::make_unique<library::cmdpar::CmdParams>();

  CmdParams->AddParam(static_cast<int>(library::cmdpar::CmdParamStatus::kp_input), "I");
  CmdParams->AddParam(static_cast<int>(library::cmdpar::CmdParamStatus::kp_input), "Input");
  CmdParams->AddParam(static_cast<int>(KP::kp_output), "Output");
  CmdParams->AddParam(static_cast<int>(KP::kp_output), "O");
  CmdParams->AddParam(static_cast<int>(KP::kp_eps), "Eps");
  CmdParams->AddParam(static_cast<int>(KP::kp_releps), "RelEps");
  CmdParams->AddParam(static_cast<int>(KP::kp_cmpfld), "Field");
  CmdParams->AddParam(static_cast<int>(KP::kp_settext), "SetDesc");
  CmdParams->AddParam(static_cast<int>(KP::kp_ide), "IDE");
  CmdParams->AddParam(static_cast<int>(KP::kp_id), "ID");
  CmdParams->AddParam(static_cast<int>(KP::kp_skip_id), "SkipID");

  CmdParams->AddKeyWord(static_cast<int>(KP::kp_fldonly), "FldOnly");
  CmdParams->AddKeyWord(static_cast<int>(KP::kp_diffonly), "DiffOnly");
  CmdParams->AddKeyWord(static_cast<int>(KP::kp_showdef), "CmpDefaults");
  CmdParams->AddKeyWord(static_cast<int>(KP::kp_cmpdomain), "CmpDomains");
  CmdParams->AddKeyWord(static_cast<int>(KP::kp_matrixfile), "MatrixFile");
  CmdParams->AddKeyWord(static_cast<int>(KP::kp_ignoreOrd), "IgnoreOrder");

  if (!CmdParams->CrackCommandLine(argc, argv)) {
    Usage(auditLine);
    return static_cast<int>(ErrorCode::ERR_USAGE);
  }

  ErrorCode = 0;
  UsingIDE = false;
  matrixFile = false;
  diffUELsRegistered = false;

  library::cmdpar::ParamRec Parameter{CmdParams->GetParams(0)};
  if (Parameter.Key == static_cast<int>(library::cmdpar::CmdParamStatus::kp_input))
    InFile1 = Parameter.KeyS;
  // else
  //    InFile1.clear();

  Parameter = CmdParams->GetParams(1);
  if (Parameter.Key == static_cast<int>(library::cmdpar::CmdParamStatus::ke_unknown))
    InFile2 = Parameter.KeyS;
  // else
  //    InFile2.clear();

  if (InFile1.empty() || InFile2.empty())
    ErrorCode = 1;

  if (!CmdParams->HasParam(static_cast<int>(KP::kp_output), DiffFileName)) {
    Parameter = CmdParams->GetParams(2);
    if (Parameter.Key == static_cast<int>(library::cmdpar::CmdParamStatus::ke_unknown))
      DiffFileName = Parameter.KeyS;
    // else
    //    DiffFileName.clear();
  }

  if (DiffFileName.empty())
    DiffFileName = "diffile";

  if (gdlib::strutilx::ExtractFileExtEx(DiffFileName).empty())
    DiffFileName = gdlib::strutilx::ChangeFileExtEx(DiffFileName, ".gdx");

  if (!CmdParams->HasParam(static_cast<int>(KP::kp_eps), S))
    EpsAbsolute = 0;
  else if (GetAsDouble(S, EpsAbsolute)) {
    if (EpsAbsolute < 0) {
      std::cout << "Eps cannot be negative\n";
      ErrorCode = 2;
    }
  } else {
    std::cout << "Bad value for Eps = " << S << '\n';
    ErrorCode = 2;
  }

  if (!CmdParams->HasParam(static_cast<int>(KP::kp_releps), S))
    EpsRelative = 0;
  else if (GetAsDouble(S, EpsRelative)) {
    if (EpsRelative < 0) {
      std::cout << "RelEps cannot be negative\n";
      ErrorCode = 2;
    }
  } else {
    std::cout << "Bad value for RelEps = " << S << '\n';
    ErrorCode = 2;
  }

  DiffOnly = CmdParams->HasKey(static_cast<int>(KP::kp_diffonly));
  fldOnly = FldOnly::fld_no;
  matrixFile = CmdParams->HasKey(static_cast<int>(KP::kp_matrixfile));
  ignoreOrder = CmdParams->HasKey(static_cast<int>(KP::kp_ignoreOrd));

  if (!CmdParams->HasParam(static_cast<int>(KP::kp_cmpfld), S))
    ActiveFields = {GMS_VAL_LEVEL, GMS_VAL_MARGINAL, GMS_VAL_LOWER, GMS_VAL_UPPER, GMS_VAL_SCALE};
  else {
    if (gdlib::strutilx::StrUEqual(S.data(), "All"))
      ActiveFields = {GMS_VAL_LEVEL, GMS_VAL_MARGINAL, GMS_VAL_LOWER, GMS_VAL_UPPER, GMS_VAL_SCALE};
    else if (gdlib::strutilx::StrUEqual(S.data(), "L")) {
      fldOnlyFld = GMS_VAL_LEVEL;
      fldOnly = FldOnly::fld_maybe;
    } else if (gdlib::strutilx::StrUEqual(S.data(), "M")) {
      fldOnlyFld = GMS_VAL_MARGINAL;
      fldOnly = FldOnly::fld_maybe;
    } else if (gdlib::strutilx::StrUEqual(S.data(), "Up")) {
      fldOnlyFld = GMS_VAL_UPPER;
      fldOnly = FldOnly::fld_maybe;
    } else if (gdlib::strutilx::StrUEqual(S.data(), "Lo")) {
      fldOnlyFld = GMS_VAL_LOWER;
      fldOnly = FldOnly::fld_maybe;
    } else if (gdlib::strutilx::StrUEqual(S.data(), "Prior") || gdlib::strutilx::StrUEqual(S.data(), "Scale")) {
      fldOnlyFld = GMS_VAL_SCALE;
      fldOnly = FldOnly::fld_maybe;
    } else {
      std::cout << "Bad field name = " << S << '\n';
      ErrorCode = 4;
    }

    if (fldOnly == FldOnly::fld_maybe)
      ActiveFields = {fldOnlyFld};
  }

  if (CmdParams->HasKey(static_cast<int>(KP::kp_fldonly))) {
    if (fldOnly == FldOnly::fld_maybe) {
      fldOnly = FldOnly::fld_yes;
      if (DiffOnly) {
        // TODO: Change combines to combined?
        std::cout << "Diff only cannot be combined with FldOnly\n";
        ErrorCode = 4;
      }
    } else {
      std::cout << "FldOnly option used with a single field comparison\n";
      ErrorCode = 4;
    }
  }

  if (CmdParams->HasParam(static_cast<int>(KP::kp_ide), S))
    UsingIDE = !S.empty() && (S.front() == 'Y' || S.front() == 'y' || S.front() == '1');

  CompSetText = true;

  // This is a mistake; should be HasKey but leave it
  if (CmdParams->HasParam(static_cast<int>(KP::kp_settext), S)) {
    S = gdlib::strutilx::UpperCase(S.data());
    if (S == "0" || S == "N" || S == "F")
      CompSetText = false;
    else if (S.empty() || S == "1" || S == "Y" || S == "T")
      CompSetText = true;
    else {
      std::cout << "Bad value for CompSetText = " << S << '\n';
      ErrorCode = 4;
    }
  }

  ShowDefRec = CmdParams->HasKey(static_cast<int>(KP::kp_showdef));
  CompDomains = CmdParams->HasKey(static_cast<int>(KP::kp_cmpdomain));

  auto populateListFromParams = [&](const gdxdiff::KP paramKey,
                                    std::unique_ptr<gdlib::gmsobj::TXStrings> &idList) {
    if (CmdParams->HasParam(static_cast<int>(paramKey), S)) {
      idList = std::make_unique<gdlib::gmsobj::TXStrings>();
      for (int N{}; N < CmdParams->GetParamCount(); N++)
        if (CmdParams->GetParams(N).Key == static_cast<int>(paramKey)) {
          S = utils::trim(CmdParams->GetParams(N).KeyS);
          // std::cout << S << '\n';
          while (!S.empty()) {
            int k{gdlib::strutilx::LChSetPos(
                ", ",
                S.data(),
                static_cast<int>(S.length()))};
            if (k == -1) {
              ID = S;
              S.clear();
            } else {
              ID = S.string().substr(0, k);
              S = S.string().erase(0, k + 1);
              S = utils::trim(S);
            }
            ID = utils::trim(ID);
            if (!ID.empty()) {
              if (idList->IndexOf(ID.data()) < 0)
                idList->Add(ID.data(), ID.length());
              // std::cout << "Include ID: " << ID << '\n';
            }
          }
        }
    }
  };

  populateListFromParams(KP::kp_id, IDsOnly);

  // if( IDsOnly && IDsOnly->GetCount() == 0 )
  //    // Like ID = "" (or ID.empty())
  //    FreeAndNil( IDsOnly );

  populateListFromParams(KP::kp_skip_id, SkipIDs);

  // We removed this but not sure why
  // if( rtl::sysutils_p3::FileExists( DiffFileName ) )
  //    rtl::sysutils_p3::DeleteFileFromDisk( DiffFileName );

  // Parameter errors
  if (ErrorCode > 0) {
    // TODO: Remove?
    // std::cout << '\n';
    Usage(auditLine);
    return static_cast<int>(ErrorCode::ERR_USAGE);
  }

  std::cout << auditLine.getAuditLine() << '\n';

  CheckFile(InFile1);
  CheckFile(InFile2);

  std::cout
      << "File1 : " << InFile1 << '\n'
      << "File2 : " << InFile2 << '\n';

  if (IDsOnly) {
    std::cout << "ID    :";
    for (int N{}; N < IDsOnly->GetCount(); N++)
      std::cout << ' ' << IDsOnly->GetConst(N);
    std::cout << '\n';
  }

  if (SkipIDs) {
    std::cout << "SkipID:";
    for (int N{}; N < SkipIDs->GetCount(); N++)
      std::cout << ' ' << SkipIDs->GetConst(N);
    std::cout << '\n';
  }

  library::ShortString S2;
  if (!gdxCreate(&PGXDIF, S2.data(), S2.length()))
    FatalError("Unable to load GDX library: " + S2, static_cast<int>(ErrorCode::ERR_LOADDLL));

  // Temporary file name
  for (int N{1}; N <= std::numeric_limits<int>::max(); N++) {
    DiffTmpName = "tmpdifffile" + std::to_string(N) + ".gdx";
    if (!rtl::sysutils_p3::FileExists(DiffTmpName))
      break;
  }

  gdxOpenWrite(PGXDIF, DiffTmpName.data(), "GDXDIFF", &ErrNr);
  if (ErrNr != 0) {
    int N{gdxGetLastError(PGXDIF)};
    // Nil is used instead of PGXDIF in Delphi code
    gdxErrorStr(PGXDIF, N, S.data());
    FatalError2("Cannot create file: " + DiffTmpName, S, static_cast<int>(ErrorCode::ERR_WRITEGDX));
  }

  UELTable = std::make_unique<gdlib::strhash::TXStrHashList<std::nullptr_t>>();
  // UELTable->OneBased = true;
  gdxStoreDomainSetsSet(PGXDIF, false);

  UELTable->Add(c_ins1.data(), c_ins1.length());
  UELTable->Add(c_ins2.data(), c_ins2.length());
  UELTable->Add(c_dif1.data(), c_dif1.length());
  UELTable->Add(c_dif2.data(), c_dif2.length());

  if (DiffOnly)
    for (int T{}; T < GMS_VAL_MAX; T++)
      UELTable->Add(GamsFieldNames[T].data(), GamsFieldNames[T].length());

  staticUELNum = UELTable->Count();

  // These calls register UELs
  OpenGDX(InFile1, PGX1);
  OpenGDX(InFile2, PGX2);

  {
    int N{1};
    while (gdxSymbolInfo(PGX1, N, ID.data(), &Dim, &iST) != 0) {
      if ((!IDsOnly || IDsOnly->IndexOf(ID.data()) >= 0) &&
          (!SkipIDs || SkipIDs->IndexOf(ID.data()) < 0))
        IDTable.insert({ID, N});
      N++;
    }
  }

  for (const auto &pair : IDTable) {
    int NN;
    if (gdxFindSymbol(PGX2, pair.first.data(), &NN) != 0)
      CompareSy(pair.second, NN);
    else
      StatusTable.insert({pair.first, StatusCode::sc_notf2});
  }

  // Find symbols in file 2 that are not in file 1
  IDTable.clear();

  for (int N{1}; gdxSymbolInfo(PGX2, N, ID.data(), &Dim, &iST) != 0; N++) {
    if ((!IDsOnly || IDsOnly->IndexOf(ID.data()) >= 0) &&
        (!SkipIDs || SkipIDs->IndexOf(ID.data()) < 0))
      IDTable.insert({ID, N});
  }

  for (const auto &pair : IDTable) {
    int NN;
    if (gdxFindSymbol(PGX1, pair.first.data(), &NN) == 0)
      StatusTable.insert({pair.first, StatusCode::sc_notf1});
  }

  if (StatusTable.empty())
    std::cout << "No differences found\n";
  else {
    std::cout << "Summary of differences:\n";
    // Find longest ID
    int NN{1};
    for (const auto &pair : StatusTable)
      if (pair.first.length() > NN)
        NN = static_cast<int>(pair.first.length());
    for (const auto &pair : StatusTable)
      std::cout << gdlib::strutilx::PadLeft(pair.first, NN) << "   "
                << StatusText.at(static_cast<int>(pair.second)) << '\n';
  }

  // CopyAcronyms( PGX1 );
  // CopyAcronyms( PGX2 );

  // Write the two filenames used as explanatory texts
  {

    int N{};
    int NN;
    do {
      ID = "FilesCompared";
      if (N > 0)
        ID += std::to_string('_') + std::to_string(N);
      if (gdxFindSymbol(PGXDIF, ID.data(), &NN) == 0)
        break;
      N++;
    } while (true);
  }

  gdxDataWriteStrStart(PGXDIF, ID.data(), {}, 1, static_cast<int>(dt_set), 0);
  strcpy(StrKeysPtrs[0], "File1");
  gdxAddSetText(PGXDIF, InFile1.data(), &StrNr);
  StrVals[GMS_VAL_LEVEL] = StrNr;
  gdxDataWriteStr(PGXDIF, const_cast<const char **>(StrKeysPtrs), StrVals);
  strcpy(StrKeysPtrs[0], "File2");
  gdxAddSetText(PGXDIF, InFile2.data(), &StrNr);
  StrVals[GMS_VAL_LEVEL] = StrNr;
  gdxDataWriteStr(PGXDIF, const_cast<const char **>(StrKeysPtrs), StrVals);
  gdxDataWriteDone(PGXDIF);

  // Note that input files are not closed at this point; so if we wrote
  // to an input file, the delete will fail and we keep the original input file alive
  gdxClose(PGXDIF);
  gdxFree(&PGXDIF);

  if (!rtl::sysutils_p3::FileExists(DiffFileName))
    RenameOK = true;
  else {
    RenameOK = rtl::sysutils_p3::DeleteFileFromDisk(DiffFileName);
#if defined(_WIN32)
    if (!RenameOK) {
      int ShellCode;
      if (rtl::p3process::P3ExecP("IDECmds.exe ViewClose \"" + DiffFileName + "\"", ShellCode) == 0)
        RenameOK = rtl::sysutils_p3::DeleteFileFromDisk(DiffFileName);
    }
#endif
  }

  if (RenameOK) {
    rtl::sysutils_p3::RenameFile(DiffTmpName, DiffFileName);
    RenameOK = rtl::sysutils_p3::FileExists(DiffFileName);
  }

  gdxClose(PGX1);
  gdxFree(&PGX1);
  gdxClose(PGX2);
  gdxFree(&PGX2);

  int ExitCode{};
  if (!RenameOK) {
    std::cout << "Could not rename " << DiffTmpName << " to " << DiffFileName << '\n';
    DiffFileName = DiffTmpName;
    ExitCode = static_cast<int>(ErrorCode::ERR_RENAME);
  }
  std::cout << "Output: " << DiffFileName << '\n';

  // UnloadGDXLibrary;
  if (UsingIDE)
    std::cout << "GDXDiff file written: " << DiffFileName << "[FIL:\"" << DiffFileName << "\",0,0]\n";
  std::cout << "GDXDiff finished\n";
  // FreeAndNil( UELTable );

  if (ExitCode == 0 && !StatusTable.empty())
    return static_cast<int>(ErrorCode::ERR_DIFFERENT);

  return ExitCode;
}

} // namespace gdxdiff

int main(const int argc, const char *argv[]) {
  return gdxdiff::main(argc, argv);
}
