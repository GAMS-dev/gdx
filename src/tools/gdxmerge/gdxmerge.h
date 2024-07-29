#ifndef GDX_GDXMERGE_H
#define GDX_GDXMERGE_H

#include <ctime>
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

#include "../library/short_string.h"
#include "../../gdlib/gmsdata.h"
#include "../../gdlib/gmsobj.h"

// GDX library interface
#include "../../../generated/gdxcc.h"
// Global constants
#include "../../../generated/gclgms.h"

namespace gdxmerge
{

enum class TProcessPass
{
   RpDoAll,
   RpScan,
   RpSmall,
   RpBig,
   RpTooBig
};

class TGAMSSymbol
{
public:
   int SyDim, SySubTyp;
   gdxSyType SyTyp;
   std::unique_ptr<gdlib::gmsdata::TTblGamsData<double>> SyData;
   library::short_string SyExplTxt;
   int64_t SySize {}, SyMemory {};
   bool SySkip {};

   TGAMSSymbol( int ADim, gdxSyType AType, int ASubTyp );

   // void SetCurrentFile( const std::string &S );
};

class TGDXFileEntry
{
public:
   std::string FFileName, FFileId, FFileInfo;

   TGDXFileEntry( const std::string &AFileName, const std::string &AFileId, const std::string &AFileInfo );
};

template<typename T>
class TFileList : public gdlib::gmsobj::TXList<T>
{
public:
   void AddFile( const std::string &AFileName, const std::string &AFileId, const std::string &AFileInfo );
   void FreeItem( int Index );// No-op?
   std::string FileName( int Index );
   std::string FileId( int Index );
   std::string FileInfo( int Index );
};

template<typename T>
class TSymbolList : public gdlib::gmsobj::TXHashedStringList<T>
{
private:
   std::unique_ptr<gdlib::gmsobj::TXStrPool<library::short_string>> StrPool;
   int FErrorCount {}, NextAcroNr {};
   std::unique_ptr<TFileList<TGDXFileEntry>> FileList;
   std::vector<std::string> IncludeList, ExcludeList;

public:
   TSymbolList();
   ~TSymbolList();

   static void OpenOutput( const library::short_string &AFileName, int &ErrNr );
   static int AddUEL( const std::string &S );
   int AddSymbol( const std::string &AName, int ADim, gdxSyType AType, int ASubTyp );
   void AddPGXFile( int FNr, TProcessPass Pass );
   bool CollectBigOne( int SyNr );
   bool FindGDXFiles( const std::string &Path );
   void WritePGXFile( int SyNr, TProcessPass Pass );
   void WriteNameList();
   void KeepNewAcronyms( const gdxHandle_t &PGX );
   void ShareAcronyms( const gdxHandle_t &PGX );
   int FindAcronym( const library::short_string &Id );

   int GetFErrorCount() const;
   int GetFileListSize() const;
   bool IsIncludeListEmpty() const;
   bool IsExcludeListEmpty() const;
   void AddToIncludeList( const std::string &item );
   void AddToExcludeList( const std::string &item );
};

std::string FormatDateTime( const std::tm &dt );

bool GetParameters( int argc, const char *argv[] );

void Usage();

int main( int argc, const char *argv[] );

}// namespace gdxmerge

#endif//GDX_GDXMERGE_H
