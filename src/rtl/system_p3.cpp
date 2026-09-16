#include "system_p3.hpp"

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace GDX_NS rtl::system_p3 {


// FIXME: this is a MS-DOS hack that "moving" to a location X: actually tries
// to move to the last known working directory for that specific drive.
// The official documentation does not mention this, but Raymond Chen delivers:
// https://devblogs.microsoft.com/oldnewthing/20100506-00/?p=14133
void getdir(uint8_t d, std::string &s)
{
#if defined(_WIN32)
   char Drive[3], DirBuf[MAX_PATH], SaveBuf[MAX_PATH];
   if (d) {
      Drive[0] = static_cast<char>('A' + d - 1);
      Drive[1] = ':';
      Drive[2] = '\0';
      GetCurrentDirectoryA(sizeof(SaveBuf), SaveBuf);
      SetCurrentDirectoryA(Drive);
   }
   GetCurrentDirectoryA(sizeof(DirBuf), DirBuf);
   if (d) SetCurrentDirectoryA(SaveBuf);
   s.assign(DirBuf);
#else
   char DirBuf[512];
   if( const char *p { getcwd( DirBuf, sizeof( DirBuf ) ) }; !p )
   {
      // this is unlikely and not handled well here: exceptions?
      s.clear();
      return;
   }
   s.assign( DirBuf );
#endif
}

}
