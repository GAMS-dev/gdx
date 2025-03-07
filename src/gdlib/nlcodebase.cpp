#include "nlcodebase.hpp"

namespace gdlib::nlcodebase {

void TNLInstStoreBase::NLCodeRelocationVar( const int varnr, int Ref )
{
   while(Ref)
   {
      const int Fld{CodeTabFld[Ref]};
      CodeTabFld[Ref] = varnr;
      if(!Fld) break;
      Ref -= Fld;
   }
}

}// namespace gdlib::nlcodebase
