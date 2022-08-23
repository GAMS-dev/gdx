#include "../rtl/p3library.h"
#include "../global/gmslibname.h"

#include "../utils.h"
//#include "../ctv.h"

#include "xcompress.h"
#include "strutilx.h"

using namespace rtl::p3library;
using namespace gdlib::strutilx;
using namespace std::literals::string_literals;

namespace gdlib::xcompress {
	using Tcompress = int (*)(void*, ulong*, const void*, ulong);
	using Tuncompress = int(*)(void*, ulong*, const void*, ulong);
	using TgzReadOpen = pgzFile(*)(const char*, const char* mode);
	using TgzRead = int(*)(pgzFile, void*, ulong);
	using TgzReadClose = int(*)(pgzFile);

	static TLibHandle ZLibHandle{};
	static Tcompress pcompress{};
	static Tuncompress puncompress{};
	static TgzReadOpen pgzReadOpen{};
	static TgzRead pgzRead{};
	static TgzReadClose pgzReadClose{};

	// Brief:
	//   Loads ZLib DLL
	// Arguments:
	//   fn: Complete file name for gmszlib.dll without the file extension, but can include a full path
	//   LoadMsg: A string indicating a reason for not loading the DLL
	// Returns:
	//   True if DLL loaded successfully and all entry points were resolved;
	//     False otherwise.
	// See Also:
	//  ZLibDllLoaded, UnloadZLibLibrary
	bool LoadZLibLibrary(const std::string& fn, std::string& LoadMsg)
	{
		static std::array<std::pair<std::string, void**>, 5> fncNamesAndPtrs = {
			std::pair<std::string, void**> {"compress"s, (void **)&pcompress},
			{"uncompress"s, (void**)&puncompress},
			{"gzopen", (void **)&pgzReadOpen},
			{"gzread", (void**)&pgzRead},
			{"gzclose", (void**)&pgzReadClose}
		};

		std::string wfn, baseName;

		auto LoadEntry = [&](const std::string& n) {
			if (!LoadMsg.empty()) return (void *)nullptr;
			void* res{ P3GetProcAddress(ZLibHandle, utils::lowercase(n)) };
			if (!res)
				LoadMsg = "Entry not found: " + n + " in " + wfn;
			return res;
		};

		LoadMsg.clear();
		if (!ZLibHandle) {
			std::string Path = ExtractFilePathEx(fn);
			baseName = ExtractFileNameEx(fn);
			if (baseName.empty()) baseName = "gmszlib1";
			wfn = Path + global::gmslibname::gamslibnamep3(baseName);
			ZLibHandle = P3LoadLibrary(wfn, LoadMsg);
			if (ZLibHandle && LoadMsg.empty()) {
				for (std::pair<std::string, void **>& pair : fncNamesAndPtrs)
					*pair.second = LoadEntry(pair.first);
			}
		}

		if (!LoadMsg.empty()) {
			pcompress = nullptr;
			puncompress = nullptr;
			pgzReadOpen = nullptr;
			pgzRead = nullptr;
			pgzReadClose = nullptr;
		}

		return pcompress;
	}

	// Brief:
	//   Unloads the ZLib DLL
	//  ZlibDllLoaded, LoadZlibLibrary
	void UnloadZLibLibrary()
	{
		if (ZLibHandle) {
			P3FreeLibrary(ZLibHandle);
			ZLibHandle = nullptr;
		}
		pcompress = nullptr;
		puncompress = nullptr;
		pgzReadOpen = nullptr;
		pgzRead = nullptr;
		pgzReadClose = nullptr;
	}

	// Brief:
	//  Indicates loading status of the ZLib DLL
	// Returns:
	//   True if ZLib dll has been loaded, false otherwise
	// See Also:
	//   LoadZLibLibrary, UnloadZLibLibrary
	bool ZLibDllLoaded()
	{
		return ZLibHandle != nullptr;
	}

	int compress(void* pdest, ulong& ldest, const void* psrc, ulong lsrc)
	{
		return pcompress(pdest, &ldest, psrc, lsrc);
	}

	int uncompress(void* pdest, ulong& ldest, const void* psrc, ulong lsrc)
	{
		return puncompress(pdest, &ldest, psrc, lsrc);
	}

	pgzFile gzReadOpen(const std::string& fn)
	{
		return pgzReadOpen(fn.c_str(), "rb");
	}

	int gzRead(pgzFile pgz, void *Buf, ulong ldest)
	{
		return pgzRead(pgz, Buf, ldest);
	}

	int gzReadClose(pgzFile& pgz)
	{
		int res{ pgzReadClose(pgz) };
		pgz = nullptr;
		return res;
	}

	void initialization() {
		UnloadZLibLibrary(); //sets entry points to null
	}

	void finalization() {

	}

	//UNIT_INIT_FINI();


}