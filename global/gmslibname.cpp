#include "gmslibname.h"

namespace global::gmslibname {
	std::string gamslibnamep3(const std::string& s)
	{
		std::string libPrefix{}, x64Infix{ "64" }, dllSuffix{};
#if defined(WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(_WIN32) || defined(__NT__)
		dllSuffix = ".dll";
#elif defined(__APPLE__)
		dllSuffix = ".dylib";
		libPrefix = "lib";
#else
		dllSuffix = ".so";
		libPrefix = "lib";
#endif
		return libPrefix + s + x64Infix + dllSuffix;
	}
}