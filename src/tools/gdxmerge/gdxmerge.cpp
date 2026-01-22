/**
 * GAMS - General Algebraic Modeling System C++ API
 *
 * Copyright (c) 2017-2026 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2026 GAMS Development Corp. <support@gams.com>
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

#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <utility>

#include "gdlib/strutilx.hpp"
#include "gdlib/utils.hpp"
#include "gdxmerge.hpp"
#include "library/cmdpar.hpp"
#include "rtl/sysutils_p3.hpp"

// #define OLD_MEMORY_CHECK

namespace gdxmerge {

bool DoBigSymbols, StrictMode;
std::int64_t SizeCutOff;
library::ShortString OutFile;
std::unique_ptr<gdlib::gmsobj::TXStrings> FilePatterns;
gdxHandle_t PGXMerge;
unsigned int InputFilesRead;
std::unique_ptr<SymbolList> SyList;

GAMSSymbol::GAMSSymbol(const int ADim, const gdxSyType AType, const int ASubTyp)
    : SyDim(ADim), SySubTyp(ASubTyp), SyTyp(AType),
      SyData(std::make_unique<gdlib::gmsdata::TTblGamsData<double>>(ADim, DataTypSize.at(AType) * sizeof(double))) {}

GDXFileEntry::GDXFileEntry(std::string AFileName, std::string AFileId, std::string AFileInfo)
    : FFileName(std::move(AFileName)), FFileId(std::move(AFileId)), FFileInfo(std::move(AFileInfo)) {}

template <typename T>
FileList<T>::~FileList() {
  Clear();
}

template <typename T>
void FileList<T>::Clear() {
  for (int i{}; i < this->GetCount(); i++) {
    delete (*this)[i];
  }
  gdlib::gmsobj::TXList<T>::Clear();
}

template <typename T>
void FileList<T>::AddFile(const std::string &AFileName, const std::string &AFileId, const std::string &AFileInfo) {
  gdlib::gmsobj::TXList<T>::Add(new GDXFileEntry(AFileName, AFileId, AFileInfo));
}

template <typename T>
std::string FileList<T>::FileName(const int Index) {
  return gdlib::gmsobj::TXList<T>::GetConst(Index)->FFileName;
}

template <typename T>
std::string FileList<T>::FileId(const int Index) {
  return gdlib::gmsobj::TXList<T>::GetConst(Index)->FFileId;
}

template <typename T>
std::string FileList<T>::FileInfo(const int Index) {
  return gdlib::gmsobj::TXList<T>::GetConst(Index)->FFileInfo;
}

SymbolList::SymbolList()
    : gdlib::gmsobj::TXHashedStringList<GAMSSymbol>() {
  StrPool = std::make_unique<gdlib::gmsobj::TXStrPool<library::ShortString>>();
  const library::ShortString empty_string;
  StrPool->Add(empty_string.data(), empty_string.length());
  fileList = std::make_unique<FileList<GDXFileEntry>>();
  library::ShortString Msg;
  gdxCreate(&PGXMerge, Msg.data(), Msg.length());
}

SymbolList::~SymbolList() {
  Clear();
}

// TODO: AS: The whole TX*List destruction and element deletion mechanism is not ideal now
// and requires duplicated code (see FileList destructor and clear)
void SymbolList::Clear() {
  for (int i{}; i < FCount; i++) {
    delete GetObject(i);
  }
  TXHashedStringList::Clear();
}

void SymbolList::OpenOutput(const library::ShortString &AFileName, int &ErrNr) {
  gdxOpenWrite(PGXMerge, AFileName.data(), "gdxmerge", &ErrNr);
  gdxStoreDomainSetsSet(PGXMerge, 0);
}

int SymbolList::AddUEL(const library::ShortString &S) {
  int result;
  gdxUELRegisterStr(PGXMerge, S.data(), &result);
  return result;
}

int SymbolList::AddSymbol(const std::string &AName, const int ADim, const gdxSyType AType, const int ASubTyp) {
  auto is_in_list = [](const std::vector<std::string> &list, const std::string &value) {
    return std::find(list.begin(), list.end(), value) != list.end();
  };

  if ((!IncludeList.empty() && !is_in_list(IncludeList, AName)) ||
      (!ExcludeList.empty() && is_in_list(ExcludeList, AName))) {
    return -1;
  }

  return TXHashedStringList::AddObject(AName.data(), AName.length(), new GAMSSymbol(ADim, AType, ASubTyp));
}

void SymbolList::AddPGXFile(const int FNr, const ProcessPass Pass) {
  bool FrstError;
  library::ShortString SyName;
  std::string FileName;

  auto CheckError = [&](const bool Cnd, const std::string &Msg) -> bool {
    bool Result{!Cnd};
    if (Result) {
      FErrorCount++;
      if (FrstError) {
        // TODO: Use four stars here instead of the usual three?
        library::printErrorMessage("\n**** Error in file " + FileName);
        FrstError = false;
      }
      library::printErrorMessage("     " + Msg + ": " + SyName);
    }
    return Result;
  };

  gdxHandle_t PGX;
  int NrSy, NrUel, N, Dim, SyITyp, SyIndx, NrRecs, FDim, D, INode, SySubTyp, DummyCount, ErrNr, RecLen;
  gdxSyType SyTyp;
  GAMSSymbol *SyObj;

  gdxStrIndex_t IndxS{};
  gdxStrIndexPtrs_t IndxSPtrs;
  GDXSTRINDEXPTRS_INIT(IndxS, IndxSPtrs);

  gdxUelIndex_t IndxI{};
  gdxValues_t Vals{};
  library::ShortString Txt, SyText, ErrMsg, FileId;
  std::int64_t XCount, Size;

  FileName = fileList->FileName(FNr);
  FileId = fileList->FileId(FNr);

  std::cout << "Reading file: " << FileName << '\n';
  gdxCreate(&PGX, ErrMsg.data(), ErrMsg.length());
  gdxOpenRead(PGX, FileName.data(), &ErrNr);
  if (ErrNr != 0) {
    gdxErrorStr(PGX, ErrNr, ErrMsg.data());
    library::printErrorMessage("\nError reading file, message: " + ErrMsg);
    return;
  }
  InputFilesRead++;

  ShareAcronyms(PGX);

  gdxUELRegisterStrStart(PGXMerge);
  gdxSystemInfo(PGX, &NrSy, &NrUel);
  FrstError = true;

  for (N = 1; N <= NrSy; N++) {
    gdxSymbolInfo(PGX, N, SyName.data(), &Dim, &SyITyp);
    gdxSymbolInfoX(PGX, N, &DummyCount, &SySubTyp, SyText.data());
    if (CheckError(Dim < GMS_MAX_INDEX_DIM, "Dimension too large")) {
      continue;
    }

    SyTyp = gdxSyType(SyITyp);
    if (SyITyp == GMS_DT_ALIAS) {
      // We cannot create two dimensional aliased sets!
      SyTyp = dt_set;
      SySubTyp = 0;
    }

    SyIndx = gdlib::gmsobj::TXHashedStringList<GAMSSymbol>::IndexOf(SyName.data());
    if (SyIndx < 0) {
      SyIndx = AddSymbol(SyName.data(), Dim + 1, SyTyp, SySubTyp);
      if (SyIndx < 0) {
        continue;
      }
    }

    SyObj = gdlib::gmsobj::TXHashedStringList<GAMSSymbol>::GetObject(SyIndx);
    if (!SyObj->SyData || SyObj->SySkip) {
      continue;
    }

    // 64 bit
    XCount = static_cast<std::int64_t>(DummyCount);
    Size = XCount * SyObj->SyDim;
    if (SyTyp == dt_var || SyTyp == dt_equ) {
      RecLen = 4;
    } else {
      RecLen = 1;
    }

    Size = Size * RecLen;

    if (Pass == ProcessPass::RpScan || Pass == ProcessPass::RpDoAll) {
      SyObj->SySize += Size;
      SyObj->SyMemory +=
          XCount *
          static_cast<std::int64_t>(
              (SyObj->SyDim * sizeof(int)) +
              (RecLen * sizeof(double)));

      if (CheckError(
              SyObj->SyData->GetCount() + XCount <= std::numeric_limits<int>::max(),
              "Element count for symbol > maxint")) {
        SyObj->SySkip = true;
        SyObj->SyData.reset();
        continue;
      }

#if defined(OLD_MEMORY_CHECK)
      if (CheckError(
              SyObj->SyMemory <= std::numeric_limits<int>::max(),
              "Symbol is too large")) {
        SyObj->SySkip = true;
        SyObj->SyData.reset();
        continue;
      }
#endif
    }

    if (Pass == ProcessPass::RpScan ||
        (Pass == ProcessPass::RpSmall && SyObj->SySize >= SizeCutOff) ||
        (Pass == ProcessPass::RpBig && SyObj->SySize < SizeCutOff) ||
        CheckError(Dim + 1 == SyObj->SyDim, "Dimensions do not match") ||
        CheckError(SyTyp == SyObj->SyTyp, "Types do not match") ||
        ((SyTyp == dt_var || SyTyp == dt_equ) &&
         CheckError(SySubTyp == SyObj->SySubTyp, "Var/Equ subtypes do not match"))) {
      continue;
    }

    if (SyObj->SyExplTxt.empty()) {
      SyObj->SyExplTxt = SyText;
    } else if (!SyText.empty()) {
      CheckError(SyObj->SyExplTxt == SyText, "Explanatory text is different");
    }

    IndxI[0] = AddUEL(FileId);
    gdxDataReadStrStart(PGX, N, &NrRecs);
    while (gdxDataReadStr(PGX, IndxSPtrs, Vals, &FDim) != 0) {
      if (Dim > 0) {
        for (D = FDim - 1; D < Dim; D++) {
          IndxI[D + 1] = AddUEL(library::ShortString{IndxSPtrs[D]});
        }
      }
      if (SyTyp == dt_set && Vals[GMS_VAL_LEVEL] != 0) {
        gdxGetElemText(PGX, utils::round<int>(Vals[GMS_VAL_LEVEL]), Txt.data(), &INode);
        Vals[GMS_VAL_LEVEL] = StrPool->Add(Txt.data(), Txt.length());
      }
      SyObj->SyData->AddRecord(IndxI, Vals);
    }
    gdxDataReadDone(PGX);
  }

  KeepNewAcronyms(PGX);
  gdxClose(PGX);
  gdxFree(&PGX);

  gdxUELRegisterDone(PGXMerge);
}

bool SymbolList::CollectBigOne(const int SyNr) {
  gdxHandle_t PGX;
  int N, NrRecs, FDim, D, INode, ErrNr, FNr;
  GAMSSymbol *SyObj;

  gdxStrIndex_t IndxS{};
  gdxStrIndexPtrs_t IndxSPtrs;
  GDXSTRINDEXPTRS_INIT(IndxS, IndxSPtrs);

  gdxUelIndex_t IndxI{};
  gdxValues_t Vals{};
  library::ShortString Txt, ErrMsg, FileName, FileId;

  SyObj = gdlib::gmsobj::TXHashedStringList<GAMSSymbol>::GetObject(SyNr);
  if (!SyObj->SyData) {
    return false;
  }

  // Replaced '\r' with '\n' here:
  std::cout << "\nlooking for symbol "
            << gdlib::gmsobj::TXHashedStringList<GAMSSymbol>::GetString(SyNr)
            << "     ";

  gdxUELRegisterStrStart(PGXMerge);

  for (FNr = 0; FNr < fileList->size(); FNr++) {
    FileName = fileList->FileName(FNr);
    FileId = fileList->FileId(FNr);
    gdxCreate(&PGX, ErrMsg.data(), ErrMsg.length());
    gdxOpenRead(PGX, FileName.data(), &ErrNr);
    if (ErrNr != 0) {
      gdxErrorStr(PGX, ErrNr, ErrMsg.data());
      library::printErrorMessage("Error reading file, message: " + ErrMsg);
      return false;
    }

    if (gdxFindSymbol(PGX, gdlib::gmsobj::TXHashedStringList<GAMSSymbol>::GetString(SyNr), &N) > 0) {
      // We did this already in AddPGXFile:
      // ShareAcronyms(PGX);
      IndxI[0] = AddUEL(FileId);
      gdxDataReadStrStart(PGX, N, &NrRecs);
      while (gdxDataReadStr(PGX, IndxSPtrs, Vals, &FDim) != 0) {
        for (D = FDim - 1; D <= SyObj->SyDim; D++) {
          IndxI[D + 1] = AddUEL(library::ShortString{IndxSPtrs[D]});
        }
        if (SyObj->SyTyp == dt_set && Vals[GMS_VAL_LEVEL] != 0) {
          gdxGetElemText(PGX, utils::round<int>(Vals[GMS_VAL_LEVEL]), Txt.data(), &INode);
          Vals[GMS_VAL_LEVEL] = StrPool->Add(Txt.data(), Txt.length());
        }
        SyObj->SyData->AddRecord(IndxI, Vals);
      }
      gdxDataReadDone(PGX);
    }

    KeepNewAcronyms(PGX);
    gdxClose(PGX);
    gdxFree(&PGX);
  }

  gdxUELRegisterDone(PGXMerge);
  return true;
}

bool SymbolList::FindGDXFiles(const std::string &Path) {
  rtl::sysutils_p3::TSearchRec Rec{};
  std::string WPath, BPath, ShortName, NewName, DTS;
  double DT;

  // Normal file or file pattern
  bool Result{true};

  WPath = Path;
  BPath = gdlib::strutilx::ExtractFilePathEx(WPath);
  if (FindFirst(WPath, rtl::sysutils_p3::faAnyFile, Rec) == 0) {
    do {
      if (Rec.Name == "." || Rec.Name == "..") {
        continue;
      }

      if (OutFile == BPath + Rec.Name) {
        std::cout << "Cannot use " << OutFile << " as input file and output file, skipped it as input\n";
        Result = false;
        continue;
      }

      ShortName = gdlib::strutilx::ChangeFileExtEx(Rec.Name, "");
      if (!library::goodUELString(ShortName.data(), ShortName.length()) || utils::trim(ShortName).empty()) {
        NewName = "File_" + std::to_string(fileList->size() + 1);
        std::cout << "*** Filename cannot be used as a valid UEL\n"
                  << "    Existing name: " << ShortName << '\n'
                  << "    Replaced with: " << NewName << '\n';
        ShortName = NewName;
      }

      DT = rtl::sysutils_p3::FileDateToDateTime(Rec.Time);

      std::uint16_t Year, Month, Day, Hour, Min, Sec, MSec;
      rtl::sysutils_p3::DecodeDate(DT, Year, Month, Day);
      rtl::sysutils_p3::DecodeTime(DT, Hour, Min, Sec, MSec);

      DTS = FormatDateTime(Year, Month, Day, Hour, Min, Sec);
      fileList->AddFile(BPath + Rec.Name, ShortName, DTS + "  " + BPath + Rec.Name);
    } while (FindNext(Rec) == 0);
    FindClose(Rec);
  } else {
    std::cout << '"' << Path << "\" is no valid pattern for an existing input file name, skipped it as input\n";
    Result = false;
  }

  return Result;
}

void SymbolList::WritePGXFile(const int SyNr, const ProcessPass Pass) {
  GAMSSymbol *SyObj;
  int R, INode;
  gdxUelIndex_t IndxI{};
  gdxValues_t Vals{};
  library::ShortString Txt;

  SyObj = gdlib::gmsobj::TXHashedStringList<GAMSSymbol>::GetObject(SyNr);
  if (!SyObj->SyData ||
      (Pass == ProcessPass::RpSmall && SyObj->SySize >= SizeCutOff)) {
    return;
  }
  SyObj->SyData->Sort();

  gdxDataWriteRawStart(
      PGXMerge,
      gdlib::gmsobj::TXHashedStringList<GAMSSymbol>::GetString(SyNr),
      SyObj->SyExplTxt.data(),
      SyObj->SyDim,
      static_cast<int>(SyObj->SyTyp),
      SyObj->SySubTyp);

  for (R = 0; R < SyObj->SyData->GetCount(); R++) {
    SyObj->SyData->GetRecord(R, IndxI, Vals);
    if (SyObj->SyTyp == dt_set && Vals[GMS_VAL_LEVEL] != 0) {
      Txt = StrPool->GetString(utils::round<int>(Vals[GMS_VAL_LEVEL]));
      gdxAddSetText(PGXMerge, Txt.data(), &INode);
      Vals[GMS_VAL_LEVEL] = INode;
    }
    gdxDataWriteRaw(PGXMerge, IndxI, Vals);
  }

  gdxDataWriteDone(PGXMerge);
  SyObj->SyData->Clear();
  SyObj->SyData.reset();
}

void SymbolList::WriteNameList() {
  const std::string BASE_NAME{"Merged_set_"};
  library::ShortString SetName;
  int N, SyNr, TextNr;

  gdxStrIndex_t AIndex{};
  gdxStrIndexPtrs_t AIndexPtrs;
  GDXSTRINDEXPTRS_INIT(AIndex, AIndexPtrs);

  gdxValues_t AVals{};

  // Find unique name for the merged set
  N = 1;
  do {
    SetName = BASE_NAME + std::to_string(N);
    gdxFindSymbol(PGXMerge, SetName.data(), &SyNr);
    if (SyNr < 0) {
      break;
    }
    N++;
  } while (true);

  gdxDataWriteStrStart(PGXMerge, SetName.data(), "Merge set", 1, 0, 0);
  for (N = 0; N < fileList->size(); N++) {
    gdxAddSetText(PGXMerge, fileList->FileInfo(N).data(), &TextNr);
    AVals[GMS_VAL_LEVEL] = TextNr;
    std::strcpy(AIndexPtrs[0], fileList->FileId(N).data());
    gdxDataWriteStr(PGXMerge, const_cast<const char **>(AIndexPtrs), AVals);
  }
  gdxDataWriteDone(PGXMerge);
}

void SymbolList::KeepNewAcronyms(const gdxHandle_t &PGX) {
  int NN{gdxAcronymNextNr(PGX, -1)};
  if (NN <= NextAcroNr) {
    return;
  }

  int OrgIndx, NewIndx, AutoIndx, AIndx;
  library::ShortString AName, AText;
  for (int N{1}; N <= gdxAcronymCount(PGX); N++) {
    gdxAcronymGetMapping(PGX, N, &OrgIndx, &NewIndx, &AutoIndx);
    if (NewIndx >= NextAcroNr) {
      gdxAcronymGetInfo(PGX, N, AName.data(), AText.data(), &AIndx);
      if (AName.empty()) {
        // Should not happen
        for (int K{1}; K <= std::numeric_limits<int>::max(); K++) {
          AName = "Acronym_Auto_" + std::to_string(K);
          if (FindAcronym(AName) < 0) {
            break;
          }
        }
        AText = "GDX file did not have a name for acronym";
      }
      gdxAcronymAdd(PGXMerge, AName.data(), AText.data(), NewIndx);
    }
  }

  NextAcroNr = NN;
}

void SymbolList::ShareAcronyms(const gdxHandle_t &PGX) {
  if (gdxAcronymCount(PGX) == 0) {
    gdxAcronymNextNr(PGX, 0);
    return;
  }

  library::ShortString AName, AText;
  int AIndx;
  if (NextAcroNr == 0) {
    gdxAcronymGetInfo(PGX, 1, AName.data(), AText.data(), &AIndx);
    NextAcroNr = AIndx;
  }

  gdxAcronymNextNr(PGX, NextAcroNr);

  library::ShortString ANameM, ATextM;
  int NM, AIndxM;
  for (int N{1}; N <= gdxAcronymCount(PGX); N++) {
    gdxAcronymGetInfo(PGX, N, AName.data(), AText.data(), &AIndx);
    NM = FindAcronym(AName);

    if (NM <= 0) {
      continue;
    }

    gdxAcronymGetInfo(PGXMerge, NM, ANameM.data(), ATextM.data(), &AIndxM);
    library::assertWithMessage(AIndxM > 0, "ShareAcronyms-1");
    gdxAcronymSetInfo(PGX, N, AName.data(), AText.data(), AIndxM);
  }
}

int SymbolList::FindAcronym(const library::ShortString &Id) {
  library::ShortString AName, AText;
  int AIndx;
  for (int N{1}; N <= gdxAcronymCount(PGXMerge); N++) {
    gdxAcronymGetInfo(PGXMerge, N, AName.data(), AText.data(), &AIndx);
    if (gdlib::strutilx::StrUEqual(Id.data(), AName.data())) {
      return N;
    }
  }
  return {};
}

int SymbolList::GetFErrorCount() const {
  return FErrorCount;
}

int SymbolList::GetFileListSize() const {
  return fileList->size();
}

bool SymbolList::IsIncludeListEmpty() const {
  return IncludeList.empty();
}

bool SymbolList::IsExcludeListEmpty() const {
  return ExcludeList.empty();
}

void SymbolList::AddToIncludeList(const std::string &item) {
  IncludeList.emplace_back(item);
}

void SymbolList::AddToExcludeList(const std::string &item) {
  ExcludeList.emplace_back(item);
}

std::string FormatDateTime(const std::uint16_t Year, const std::uint16_t Month, const std::uint16_t Day,
                           const std::uint16_t Hour, const std::uint16_t Min, const std::uint16_t Sec) {
  auto Int2 = [](const int n) -> std::string {
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << n;
    return oss.str();
  };

  return Int2(Year) + '/' + Int2(Month) + '/' + Int2(Day) + ' ' +
         Int2(Hour) + ':' + Int2(Min) + ':' + Int2(Sec);
}

bool GetParameters(const int argc, const char *argv[]) {
  enum class KP : std::uint8_t {
    Id = 1,
    Exclude,
    Big,
    Strict,
    Output
  };

  std::unique_ptr<library::cmdpar::CmdParams> CmdParams;
  int ParNr, KW, X, K;
  std::string KS, Id;

  // TODO: Merge declarations and initializations?
  DoBigSymbols = false;
  SizeCutOff = 10000000;
  // Probably unnecessary:
  OutFile.clear();
  StrictMode = false;
  FilePatterns = std::make_unique<gdlib::gmsobj::TXStrings>();
  CmdParams = std::make_unique<library::cmdpar::CmdParams>();

  CmdParams->AddParam(static_cast<int>(KP::Id), "ID");
  CmdParams->AddParam(static_cast<int>(KP::Exclude), "EXCLUDE");
  CmdParams->AddParam(static_cast<int>(KP::Big), "BIG");
  CmdParams->AddParam(static_cast<int>(KP::Strict), "STRICT");
  // TODO: Output also in capital letters?
  CmdParams->AddParam(static_cast<int>(KP::Output), "Output");
  CmdParams->AddParam(static_cast<int>(KP::Output), "O");

  bool Result{CmdParams->CrackCommandLine(argc, argv)};
  if (Result) {
    ParNr = 0;
    do {
      library::cmdpar::ParamRec paramRec{CmdParams->GetParams(ParNr)};
      KW = paramRec.Key;
      KS = paramRec.KeyS;

      ParNr++;

      switch (static_cast<KP>(KW)) {
      case KP::Id:
      case KP::Exclude:
        while (!KS.empty()) {
          K = gdlib::strutilx::LChSetPos(
              ", ",
              KS.data(),
              static_cast<int>(KS.length()));
          if (K == -1) {
            Id = KS;
            KS.clear();
          } else {
            Id = KS.substr(0, K);
            KS.erase(0, K + 1);
            KS = utils::trim(KS);
          }
          Id = utils::trim(Id);
          if (!Id.empty()) {
            if (KW == static_cast<int>(KP::Id)) {
              std::cout << "Include Id: " << Id << '\n';
              SyList->AddToIncludeList(utils::trim(Id));
            } else {
              std::cout << "Exclude Id: " << Id << '\n';
              SyList->AddToExcludeList(utils::trim(Id));
            }
          }
        }
        break;

      case KP::Big:
        DoBigSymbols = true;
        if (!KS.empty()) {
          utils::val(KS, X, K);
          if (K == 0) {
            SizeCutOff = X;
          }
        }
        break;

      case KP::Strict:
        if (gdlib::strutilx::StrUEqual(KS, "true")) {
          StrictMode = true;
        }
        break;

      case KP::Output:
        if (OutFile.empty()) {
          OutFile = KS;
        } else {
          library::printErrorMessage("*** Error: Only one output file can be specified");
          Result = false;
        }
        break;

      default:
        FilePatterns->Add(KS.data(), KS.length());
        // SyList->FindGDXFiles( KS );
      }
    } while (ParNr < CmdParams->GetParamCount());
  }

  if (OutFile.empty()) {
    OutFile = "merged.gdx";
  } else if (gdlib::strutilx::ExtractFileExtEx(OutFile).empty()) {
    OutFile = gdlib::strutilx::ChangeFileExtEx(OutFile, ".gdx");
  }

  return Result;
}

void Usage(const library::AuditLine &auditLine) {
  std::cout
      << "gdxmerge: Merge GDX files\n"
      << auditLine.getAuditLine()
      << "\n\nUsage:\n"
         "   gdxmerge filepat1 filepat2 ... filepatn\n"
         "     Optional parameters:\n"
         "          id=ident1, ident2: Merge specified IDs only\n"
         "          exclude=ident1, ident2: Do not merge specified IDs\n"
         "          big=<integer>    : Size indicator for a large symbol\n"
         "          output=filename  : Output file; merged.gdx by default\n"
         "          strict=true/false: Terminate on failure, e.g. missing input files\n"
         "Filepat represents a filename or a file pattern.\n"
         "The form: @filename will process parameters from that file\n";
}

int main(const int argc, const char *argv[]) {
  library::ShortString Msg;
  int N, ErrNr;

  library::AuditLine auditLine{"GDXMERGE"};
  if (argc > 1 && gdlib::strutilx::StrUEqual(argv[1], "AUDIT")) {
    std::cout << auditLine.getAuditLine() << '\n';
    return {};
  }

  if (argc == 1 || (argc == 2 && std::strcmp(argv[1], "/?") == 0)) {
    Usage(auditLine);
    return {};
  }

  if (gdxGetReady(Msg.data(), Msg.length()) == 0) {
    library::printErrorMessage("*** Error: Unable to load gdx library, message:\n" + Msg);
    return 1;
  }

  SyList = std::make_unique<SymbolList>();

  if (!GetParameters(argc, argv)) {
    library::printErrorMessage("*** Error: Parameter error");
    FilePatterns->Clear();
    return 1;
  }

  if (rtl::sysutils_p3::FileExists(OutFile)) {
    if (StrictMode) {
      library::printErrorMessage("*** Error  : Output file \"" + OutFile + "\" already exists (strict mode)");
      return 1;
    }

    rtl::sysutils_p3::DeleteFileFromDisk(OutFile);
  }

  if (!SyList->IsIncludeListEmpty() && !SyList->IsExcludeListEmpty()) {
    // TODO: Use four stars here instead of the usual three?
    library::printErrorMessage(R"(**** The options "ID" and "Exclude" are mutual exclusive)");
    return 1;
  }

  gdxmerge::SymbolList::OpenOutput(OutFile, ErrNr);
  std::cout << "Output file: " << OutFile << '\n';
  if (ErrNr != 0) {
    library::printErrorMessage("*** Error  : Cannot write to output file, Error Nr = " + std::to_string(ErrNr));
    gdxErrorStr(PGXMerge, ErrNr, Msg.data());
    library::printErrorMessage("*** Message: " + Msg);
    return 1;
  }

  for (N = 0; N < FilePatterns->GetCount(); N++) {
    if (!SyList->FindGDXFiles(FilePatterns->GetConst(N)) && StrictMode) {
      library::printErrorMessage("*** Error  : Issue with file name \"" + std::string{FilePatterns->GetConst(N)} + "\" (strict mode)");
      return 1;
    }
  }
  FilePatterns->Clear();

  InputFilesRead = 0;
  if (!DoBigSymbols) {
    for (N = 0; N < SyList->GetFileListSize(); N++) {
      SyList->AddPGXFile(N, ProcessPass::RpDoAll);
    }

    for (N = 0; N < SyList->size(); N++) {
      SyList->WritePGXFile(N, ProcessPass::RpDoAll);
    }
  } else {
    for (N = 0; N < SyList->GetFileListSize(); N++) {
      SyList->AddPGXFile(N, ProcessPass::RpScan);
    }

    for (N = 0; N < SyList->GetFileListSize(); N++) {
      SyList->AddPGXFile(N, ProcessPass::RpSmall);
    }

    for (N = 0; N < SyList->size(); N++) {
      SyList->WritePGXFile(N, ProcessPass::RpSmall);
    }

    for (N = 0; N < SyList->size(); N++) {
      if (SyList->CollectBigOne(N)) {
        SyList->WritePGXFile(N, ProcessPass::RpBig);
      }
    }
  }

  SyList->WriteNameList();

  gdxClose(PGXMerge);
  gdxFree(&PGXMerge);

  std::cout << '\n';

  if (SyList->GetFErrorCount() > 0) {
    std::cout << "Number of errors reported = " << SyList->GetFErrorCount() << '\n';
  }

  SyList->Clear();
  // UnloadGDXLibrary();

  if (InputFilesRead == 0) {
    if (StrictMode) {
      library::printErrorMessage("*** Error  : No valid input files specified (strict mode)");
      return 1;
    }

    std::cout << "No valid input files specified\n";
  } else {
    std::cout << "Merge complete, " << InputFilesRead << " input files merged\n";
  }

  return {};
}

} // namespace gdxmerge

int main(const int argc, const char *argv[]) {
  return gdxmerge::main(argc, argv);
}
