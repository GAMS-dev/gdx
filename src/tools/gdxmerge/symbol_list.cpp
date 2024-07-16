#include "symbol_list.h"

namespace gdxmerge
{

template<typename T>
TGAMSSymbol<T>::TGAMSSymbol( const int ADim, const int AType, const int ASubTyp ) : syDim( ADim ), syTyp( AType ), sySubTyp( ASubTyp )
{
   // syExplTxt.clear();
   syData = new gdlib::gmsdata::TTblGamsData<T>( ADim, sizeof( T ) );
   // sySize = 0;
   // syMemory = 0;
   sySkip = false;
};

template<typename T>
TGAMSSymbol<T>::~TGAMSSymbol()
{
   delete syData;
}

}// namespace gdxmerge
