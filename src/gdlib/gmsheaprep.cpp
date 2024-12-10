#include "gmsheaprep.hpp"

#include "gmsheapnew.hpp"
#include "gmslist.hpp"
#include "strutilx.hpp"

using namespace std::literals::string_literals;

namespace gdlib::gmsheaprep
{

void GMSHeapStats( gmsheapnew::THeapMgr &heapMgr, gmslist::TGmsList &gmsList, int32_t line )
{
   auto MemSizeFmt = []( const int64_t V, const int w) -> std::string {
      int64_t C;
      std::string s;
      if(V < 16 * 1024)
      {
         C = 1;
         s = " b";
      }
      else if(V < 16 * 1024 * 1024)
      {
         C = 1024;
         s = "Kb"s;
      } else
      {
         C = 1024 * 1024;
         s = "Mb"s;
      }
      return strutilx::PadLeft( strutilx::IntToNiceStr( static_cast<int>(( V + C / 2 ) / C )), w ) + ' ' + s;
   };

   std::array doTxt {""s, "64"s};

   const auto x {MemSizeFmt( gmsheapnew::BIGBLOCKSIZE, 0)};

   const auto BM = heapMgr.GetBBMgr();
   // ...
   // FIXME: Finish porting!
   STUBWARN();
}

}