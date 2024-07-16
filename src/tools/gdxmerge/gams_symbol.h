#ifndef GDX_GAMS_SYMBOL_H
#define GDX_GAMS_SYMBOL_H

#include <string>
#include <cstdint>

#include "../../gdlib/gmsdata.h"

namespace gdxmerge
{

template<typename T>
class TGAMSSymbol
{
private:
   int syDim, syTyp, sySubTyp;
   gdlib::gmsdata::TTblGamsData<T> syData;
   std::string syExplTxt;
   int64_t sySize, syMemory;
   bool sySkip;

public:
   TGAMSSymbol( int ADim, int AType, int ASubTyp );
   ~TGAMSSymbol();

   // void SetCurrentFile( const &std::string s );
};

}// namespace gdxmerge

#endif//GDX_GAMS_SYMBOL_H
