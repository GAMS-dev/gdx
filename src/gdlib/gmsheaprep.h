#pragma once
#include <cstdint>

namespace gdlib
{
namespace gmsheapnew { class THeapMgr; }
namespace gmslist { class TGmsList; }
}

namespace gdlib::gmsheaprep
{
void GMSHeapStats(gmsheapnew::THeapMgr &heapMgr, gmslist::TGmsList &gmsList,  int32_t line);
}
