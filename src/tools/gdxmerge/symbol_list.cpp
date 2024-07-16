#include "symbol_list.h"
#include "../library/short_string.h"

namespace gdxmerge
{

template<typename T>
TGAMSSymbol<T>::TGAMSSymbol( const int ADim, const int AType, const int ASubTyp ) : syDim( ADim ), syTyp( AType ), sySubTyp( ASubTyp )
{
   syData = new gdlib::gmsdata::TTblGamsData<T>( ADim, sizeof( T ) );
   sySkip = false;
};

template<typename T>
TGAMSSymbol<T>::~TGAMSSymbol()
{
   delete syData;
}

template<typename T>
TSymbolList<T>::TSymbolList( gdxHandle_t &PGXMerge )
{
   gdlib::gmsobj::TXHashedStringList<T>();
   StrPool = new gdlib::gmsobj::TXStrPool<T>();
   StrPool.Add( "" );
   FileList = new TFileList<T>;
   library::short_string Msg;
   gdxCreate( &PGXMerge, Msg.data(), Msg.length() );
};

template<typename T>
TSymbolList<T>::~TSymbolList()
{
   delete StrPool;
   delete FileList;
   gdlib::gmsobj::~TXHashedStringList<T>();
};

}// namespace gdxmerge
