#ifndef GDX_SYMBOL_LIST_H
#define GDX_SYMBOL_LIST_H

#include <string>
#include <vector>
#include <cstdint>

#include "../../gdlib/gmsobj.h"
#include "../../gdlib/gmsdata.h"

// GDX library interface
#include "../../../generated/gdxcc.h"

namespace gdxmerge
{

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
   gdlib::gmsdata::TTblGamsData<T> syData;
   std::string syExplTxt;
   int64_t sySize {}, syMemory {};
   bool sySkip;

public:
   TGAMSSymbol( int ADim, int AType, int ASubTyp );
   ~TGAMSSymbol();

   // void SetCurrentFile( const &std::string s );
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
   gdlib::gmsobj::TXStrPool<T> StrPool;
   int FErrorCount {}, NextAcroNr {};
   TFileList<T> FileList;
   std::vector<std::string> IncludeList, ExcludeList;

public:
   TSymbolList();
   ~TSymbolList();

   void OpenOutput( const std::string &AFileName, int ErrNr );
   int AddUEL( const std::string &s );
   int AddSymbol( const std::string &AName, int ADim, int AType, int ASubTyp );
   void AddPGXFile( int FNr, TProcessPass Pass );
   void WriteNameList();
   void KeepNewAcronyms( const gdxHandle_t &PGX );
   void ShareAcronyms( const gdxHandle_t &PGX );
   int FindAcronym( const std::string &Id );
};

}// namespace gdxmerge

#endif//GDX_SYMBOL_LIST_H
