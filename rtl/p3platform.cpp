#include "p3platform.h"

using namespace std::literals::string_literals;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace rtl::p3platform {

	tOSFileType OSFileType() {
#if defined(WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(_WIN32) || defined(__NT__)
		return OSFileWIN;
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__)
		return OSFileUNIX;
#else
		return OSFileMissing;
#endif
	}

	tOSPlatform OSPlatform() {
#if defined(_WIN64)
		return OSWindows64EMT;
#elif defined(WIN32) || defined(__WIN32__)
		return OSWindowsNT;
#elif defined(__APPLE__)
		return OSDarwin_x64;
#elif defined(__linux__)
		return OSLinux86_64;
#else
		return OSMissing;
#endif
	}
}
