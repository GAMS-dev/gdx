#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "gdlib/gmsdata.hpp"
#include "gdlib/gmsobj.hpp"
#include "library/common.hpp"
#include "library/short_string.hpp"

// GDX library interface
#include "generated/gdxcc.h"
// Global constants
#include "generated/gclgms.h"

namespace gdxmerge {

constexpr std::array DataTypSize{1, 1, 5, 5, 0};

enum class ProcessPass : uint8_t {
  RpDoAll,
  RpScan,
  RpSmall,
  RpBig,
  RpTooBig
};

struct GAMSSymbol {
  int SyDim, SySubTyp;
  gdxSyType SyTyp;
  std::unique_ptr<gdlib::gmsdata::TTblGamsData<double>> SyData;
  library::ShortString SyExplTxt;
  int64_t SySize{}, SyMemory{};
  bool SySkip{};

  GAMSSymbol(int ADim, gdxSyType AType, int ASubTyp);

  // void SetCurrentFile( const std::string &S );
};

struct GDXFileEntry {
  std::string FFileName, FFileId, FFileInfo;

  GDXFileEntry(std::string AFileName, std::string AFileId, std::string AFileInfo);
};

template <typename T>
struct FileList final : public gdlib::gmsobj::TXList<T> {
  ~FileList() override;
  void Clear() override;
  void AddFile(const std::string &AFileName, const std::string &AFileId, const std::string &AFileInfo);
  std::string FileName(int Index);
  std::string FileId(int Index);
  std::string FileInfo(int Index);
};

class SymbolList : public gdlib::gmsobj::TXHashedStringList<GAMSSymbol> {
  std::unique_ptr<gdlib::gmsobj::TXStrPool<library::ShortString>> StrPool;
  int FErrorCount{}, NextAcroNr{};
  std::unique_ptr<FileList<GDXFileEntry>> fileList;
  std::vector<std::string> IncludeList, ExcludeList;

public:
  SymbolList();
  ~SymbolList() override;
  void Clear() override;

  static void OpenOutput(const library::ShortString &AFileName, int &ErrNr);
  static int AddUEL(const library::ShortString &S);
  int AddSymbol(const std::string &AName, int ADim, gdxSyType AType, int ASubTyp);
  void AddPGXFile(int FNr, ProcessPass Pass);
  bool CollectBigOne(int SyNr);
  bool FindGDXFiles(const std::string &Path);
  void WritePGXFile(int SyNr, ProcessPass Pass);
  void WriteNameList();
  void KeepNewAcronyms(const gdxHandle_t &PGX);
  void ShareAcronyms(const gdxHandle_t &PGX);
  static int FindAcronym(const library::ShortString &Id);

  int GetFErrorCount() const;
  int GetFileListSize() const;
  bool IsIncludeListEmpty() const;
  bool IsExcludeListEmpty() const;
  void AddToIncludeList(const std::string &item);
  void AddToExcludeList(const std::string &item);
};

std::string FormatDateTime(uint16_t Year, uint16_t Month, uint16_t Day,
                           uint16_t Hour, uint16_t Min, uint16_t Sec);

bool GetParameters(int argc, const char *argv[]);

void Usage(const library::AuditLine &auditLine);

int main(int argc, const char *argv[]);

} // namespace gdxmerge
