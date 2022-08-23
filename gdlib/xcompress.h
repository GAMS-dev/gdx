#pragma once

#include <string>

// Description:
//  This unit provides the necessary function pointer variables
//  used for dynamic loading of the ZLib1.DLL
//  Note that we rename this DLL to gmszlib1.dll to make sure we
//  know which dll we are using; other components use this dll too!

namespace gdlib::xcompress {
	using pgzFile = void*;
    using ulong = unsigned long;

	bool LoadZLibLibrary(const std::string& fn, std::string& LoadMsg);
	void UnloadZLibLibrary();

	bool ZLibDllLoaded();

	int compress(void* pdest, ulong& ldest, const void* psrc, ulong lsrc);
	int uncompress(void* pdest, ulong& ldest, const void* psrc, ulong lsrc);
	pgzFile gzReadOpen(const std::string& fn);
	int gzRead(pgzFile pgz, void *Buf, ulong ldest);
	int gzReadClose(pgzFile& pgz);
}