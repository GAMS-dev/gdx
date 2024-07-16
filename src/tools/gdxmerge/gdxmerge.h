#ifndef GDX_GDXMERGE_H
#define GDX_GDXMERGE_H

#include <ctime>
#include <string>
#include <vector>
#include <cstdint>

#include "../../gdlib/gmsdata.h"
#include "../../gdlib/gmsobj.h"

// GDX library interface
#include "../../../generated/gdxcc.h"

namespace gdxmerge
{

std::string FormatDateTime( const std::tm &dt );

enum class TProcessPass
{
   rpDoAll,
   rpScan,
   rpSmall,
   rpBig,
   rpTooBig
};

template<typename T>
class TGAMSSymbol
{
private:
   int syDim, syTyp, sySubTyp;
   gdlib::gmsdata::TTblGamsData<T> *syData;
   std::string syExplTxt;
   int64_t sySize {}, syMemory {};
   bool sySkip;

public:
   TGAMSSymbol( int ADim, int AType, int ASubTyp );
   ~TGAMSSymbol();

   // void SetCurrentFile( const std::string &S );
};

class TGDXFileEntry
{
private:
   std::string FFileName, FFileId, FFileInfo;

public:
   TGDXFileEntry( const std::string &AFileName, const std::string &AFileId, const std::string &AFileInfo );
   ~TGDXFileEntry();
};

template<typename T>
class TFileList : public gdlib::gmsobj::TXList<T>
{
public:
   TFileList();
   ~TFileList();

   void AddFile( const std::string &AFileName, const std::string &AFileId, const std::string &AFileInfo );
   void FreeItem( int Index );// No-op?
   std::string FileId( int Index );
   std::string FileInfo( int Index );
   std::string FileName( int Index );
};

template<typename T>
class TSymbolList : public gdlib::gmsobj::TXHashedStringList<T>
{
private:
   gdlib::gmsobj::TXStrPool<T> *StrPool;
   int FErrorCount {}, NextAcroNr {};
   TFileList<T> *FileList;
   std::vector<std::string> IncludeList, ExcludeList;

public:
   TSymbolList();
   ~TSymbolList();

   void OpenOutput( const std::string &AFileName, int &ErrNr );
   int AddUEL( const std::string &S );
   int AddSymbol( const std::string &AName, int ADim, int AType, int ASubTyp );
   void AddPGXFile( int FNr, TProcessPass Pass );
   void WriteNameList();
   void KeepNewAcronyms( const gdxHandle_t &PGX );
   void ShareAcronyms( const gdxHandle_t &PGX );
   int FindAcronym( const std::string &Id );
};

int main( int argc, const char *argv[] );

}// namespace gdxmerge

#endif//GDX_GDXMERGE_H
