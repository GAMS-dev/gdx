#pragma once
#include <string>
#include <array>

#if defined(_WIN32)
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp
#endif


// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace rtl::p3platform {
	enum tOSFileType {
		OSFileWIN,
		OSFileUNIX,
		OSFileMissing
	};

	enum tOSPlatform {
			OSWindowsNT,
			OSWindows64EMT,
			OSLinux86_64,
			OSDarwin_x64,
			OSMissing    };

	tOSFileType OSFileType();
	tOSPlatform OSPlatform();
}

