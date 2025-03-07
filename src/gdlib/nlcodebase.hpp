#pragma once
#include "glookup.hpp"
#include <vector>

namespace gdlib::nlcodebase {

class TConstPoolXBase : public glookup::TConstPool<int>
{
public:
   explicit TConstPoolXBase( gmsheapnew::THeapMgr &Amyheap )
       : TConstPool( Amyheap ) {}
};

class TNLInstStoreBase {
public:
   bool NLCodeEmitted {};
   std::vector<int32_t> CodeTabFld {};

   void NLCodeUpdateFld( const int N, const int Fld)
   {
      CodeTabFld[N] = Fld;
   }

   void NLCodeRelocationVar( int varnr, int Ref );
};

}