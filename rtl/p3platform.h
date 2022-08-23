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

	const std::array OSFileTypeText = {
			"WIN", "UNIX", "XXX"
	};

	enum tOSPlatform {
			OSWindowsNT,
			OSWindows64EMT,
			OSLinux86_64,
			OSDarwin_x64,
			OSMissing    };

	const std::array OSPlatformText = {
			"WinNT",
			"Win64EMT",
			"Linux86_64",
			"Darwin-x64",
			"Missing"};
	
	const std::array OSBldCod = {
			"DOS",
			"Win95",
			"WinNT",
			"WEI",
			"AIX",
			"HP-UX",
			"IRIX64",
			"Linux",
			"SunOS-sparc32",
			"OSF1",
			"Darwin-ppc",
			"LEG",
			"SunOS-i86pc",
			"SunOS-sparc64",
			"Darwin-i386",
			"DEG",
			"BlueGene",
			"Missing"};

	const std::array OSDllExtension = {
			".dll",
			".dll",
			".so",
			".dylib",
			".XXX"
	};

	const std::array OSDllPrefix = {
			"",
			"",
			"lib",
			"lib",
			"lib"
	};

	tOSFileType OSFileType();
	tOSPlatform OSPlatform();
	std::string OSNullFilename();
	std::string OSConsoleName();
	std::string OSLanguagePascal();
	std::string OSLanguageC();
	bool nativeIsLittleEndian();
}

