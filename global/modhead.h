#pragma once

#include <fstream>

#ifndef STUBWARN
#define STUBWARN()  if(global::modhead::stubofs.is_open()) {\
						global::modhead::stubofs << "WARNING: Calling stub method " << __FUNCTION__ << " in line " << __LINE__ << " of file "  << __FILE__ << ":" << __LINE__ << std::endl; \
						global::modhead::stubofs.flush(); }

#define STUBWARN_MSG(msg)	global::modhead::stubofs << "WARNING: " << (msg) << " when calling stub method " << __FUNCTION__ << " in line " << __LINE__ << " of file "  << __FILE__ << ":" << __LINE__ << std::endl; \
							global::modhead::stubofs.flush();
#endif

namespace global::modhead {
	extern std::ofstream stubofs;
}