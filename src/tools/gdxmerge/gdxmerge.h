#ifndef GDX_GDXMERGE_H
#define GDX_GDXMERGE_H

#include <array>
#include <vector>
#include <string>
#include <cstdint>
#include <memory>

#include "../library/common.h"
#include "../library/short_string.h"
#include "../../gdlib/gmsdata.h"
#include "../../gdlib/gmsobj.h"

// GDX library interface
#include "../../../generated/gdxcc.h"
// Global constants
#include "../../../generated/gclgms.h"

namespace gdxmerge
{

constexpr std::array DataTypSize { 1, 1, 5, 5, 0 };

enum class ProcessPass_t : uint8_t
{
   RpDoAll,
   RpScan,
   RpSmall,
   RpBig,
   RpTooBig
};

struct GAMSSymbol_t {
   int SyDim, SySubTyp;
   gdxSyType SyTyp;
   std::unique_ptr<gdlib::gmsdata::TTblGamsData<double>> SyData;
   library::ShortString_t SyExplTxt;
   int64_t SySize {}, SyMemory {};
   bool SySkip {};

   GAMSSymbol_t( int ADim, gdxSyType AType, int ASubTyp );

   // void SetCurrentFile( const std::string &S );
};

struct GDXFileEntry_t {
   std::string FFileName, FFileId, FFileInfo;

   GDXFileEntry_t( const std::string &AFileName, const std::string &AFileId, const std::string &AFileInfo );
};

template<typename T>
struct FileList_t final : public gdlib::gmsobj::TXList<T> {
   ~FileList_t() override;
   void Clear() override;
   void AddFile( const std::string &AFileName, const std::string &AFileId, const std::string &AFileInfo );
   std::string FileName( int Index );
   std::string FileId( int Index );
   std::string FileInfo( int Index );
};

class SymbolList_t : public gdlib::gmsobj::TXHashedStringList<GAMSSymbol_t>
{
   std::unique_ptr<gdlib::gmsobj::TXStrPool<library::ShortString_t>> StrPool;
   int FErrorCount {}, NextAcroNr {};
   std::unique_ptr<FileList_t<GDXFileEntry_t>> FileList;
   std::vector<std::string> IncludeList, ExcludeList;

public:
   SymbolList_t();
   ~SymbolList_t() override;
   void Clear() override;

   static void OpenOutput( const library::ShortString_t &AFileName, int &ErrNr );
   static int AddUEL( const library::ShortString_t &S );
   int AddSymbol( const std::string &AName, int ADim, gdxSyType AType, int ASubTyp );
   void AddPGXFile( int FNr, ProcessPass_t Pass );
   bool CollectBigOne( int SyNr );
   bool FindGDXFiles( const std::string &Path );
   void WritePGXFile( int SyNr, ProcessPass_t Pass );
   void WriteNameList();
   void KeepNewAcronyms( const gdxHandle_t &PGX );
   void ShareAcronyms( const gdxHandle_t &PGX );
   int FindAcronym( const library::ShortString_t &Id );

   int GetFErrorCount() const;
   int GetFileListSize() const;
   bool IsIncludeListEmpty() const;
   bool IsExcludeListEmpty() const;
   void AddToIncludeList( const std::string &item );
   void AddToExcludeList( const std::string &item );
};

std::string FormatDateTime( uint16_t Year, uint16_t Month, uint16_t Day,
                            uint16_t Hour, uint16_t Min, uint16_t Sec );

bool GetParameters( int argc, const char *argv[] );

void Usage( const library::AuditLine_t &AuditLine );

int main( int argc, const char *argv[] );

}// namespace gdxmerge

#endif//GDX_GDXMERGE_H
