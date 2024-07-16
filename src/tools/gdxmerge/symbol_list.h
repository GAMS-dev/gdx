#ifndef GDX_SYMBOL_LIST_H
#define GDX_SYMBOL_LIST_H

#include <string>
#include <vector>

#include "../../gdlib/gmsobj.h"

// GDX library interface
#include "../../../generated/gdxcc.h"

namespace gdxmerge
{

enum class TProcessPass
{
   rp_do_all,
   rp_scan,
   rp_small,
   rp_big,
   rp_tooBig
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
class TFileList : public TXList<T>
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
class TSymbolList : public TXHashedStringList<T>
{
private:
   TXStrPool StrPool;
   int FErrorCount {}, NextAcroNr {};
   TFileList<T> FileList;
   std::vector<std::string> IncludeList, ExcludeList;

public:
   TSymbolList();
   ~TSymbolList();

   void OpenOutput( const std::string &AFileName, int ErrNr );
   int AddUEL( const std::string &s );
   int AddSymbol( const std::string &AName, int ADim, TgdxDataType AType, int ASubTyp );
   void AddPGXFile( int FNr, TProcessPass Pass );
   void WriteNameList();
   void KeepNewAcronyms( const gdxHandle_t &PGX );
   void ShareAcronyms( const gdxHandle_t &PGX );
   int FindAcronym( const std::string &Id );
};

}// namespace gdxmerge

#endif//GDX_SYMBOL_LIST_H
