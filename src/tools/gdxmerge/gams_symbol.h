#ifndef GDX_GAMS_SYMBOL_H
#define GDX_GAMS_SYMBOL_H

#include "../../gdlib/gmsobj.h"

// GDX library interface
#include "../../../generated/gdxcc.h"

namespace gdxmerge
{

class TGAMSSymbol
{
private:
   int syDim, syTyp;

public:
   TGAMSSymbol();
   ~TGAMSSymbol();
};

}// namespace gdxmerge

#endif//GDX_GAMS_SYMBOL_H
