#include "p3platform.h"
#include "../global/unit.h"
#include <string>

using namespace std::literals::string_literals;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace rtl::p3platform {

	static tOSFileType localOSFileType;
	static tOSPlatform localOSPlatform;

	static std::string
		localOSNullFileName,
		localOSConsoleName,
		localOSLanguagePascal,
		localOSLanguageC;
	static bool localIsLittleEndian;

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

	std::string OSNullFilename()
	{
		return localOSNullFileName;
	}

	std::string OSConsoleName()
	{
		return localOSConsoleName;
	}

	std::string OSLanguagePascal()
	{
		return localOSLanguagePascal;
	}

	std::string OSLanguageC()
	{
		return localOSLanguageC;
	}

	bool nativeIsLittleEndian()
	{
		return localIsLittleEndian;
	}

	void initialization() {
		localOSFileType = OSFileType();
		localOSPlatform = OSPlatform();

		switch (localOSFileType) {
		case OSFileWIN:
			localOSNullFileName = "nul"s;
			localOSConsoleName = "con"s;
			break;
		case OSFileUNIX:
			localOSNullFileName = "/dev/null"s;
			localOSConsoleName = "/dev/tty"s;
			break;
		case OSFileMissing:
			localOSNullFileName = ""s;
			localOSConsoleName = ""s;
			break;
		}

		localIsLittleEndian = true;
	}

	void finalization() {
	}

	UNIT_INIT_FINI();

}
