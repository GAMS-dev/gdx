/*
* GAMS - General Algebraic Modeling System GDX API
 *
 * Copyright (c) 2017-2025 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2025 GAMS Development Corp. <support@gams.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "p3library.hpp"

#if defined( WIN32 ) || defined( _WIN64 ) || defined( __WIN32__ ) || defined( _WIN32 ) || defined( __NT__ )
#include <Windows.h>
#define MY_WINPLATFORM
#else
#include <dlfcn.h>
#endif

#include "sysutils_p3.hpp"
#include "p3platform.hpp"

using namespace rtl::sysutils_p3;
using namespace rtl::p3platform;
using namespace std::literals::string_literals;

namespace rtl::p3library
{
TLibHandle P3LoadLibrary( const std::string &lib, std::string &loadMsg )
{
#ifdef MY_WINPLATFORM
   const auto oldMode = SetErrorMode( SEM_FAILCRITICALERRORS );
   HINSTANCE hinstLib = LoadLibraryA( lib.c_str() );
   const auto lastErr = GetLastError();
   SetErrorMode( oldMode );
   if( !hinstLib )
      loadMsg = "Error message with code = "s + rtl::sysutils_p3::IntToStr( lastErr );
   return hinstLib;
#else
   void *libHandle = dlopen( lib.c_str(), RTLD_NOW | RTLD_GLOBAL );
   if( !libHandle )
      loadMsg = "Unable to load dynamic library named "s + lib + " with error: "s + dlerror();
   return libHandle;
#endif
}

bool P3FreeLibrary( TLibHandle handle )
{
#ifdef MY_WINPLATFORM
   return FreeLibrary( (HMODULE) handle );
#else
   return !dlclose( (void *) handle );
#endif
}

bool P3LibHandleIsNil( const TLibHandle handle )
{
   return !handle;
}

void *P3GetProcAddress( TLibHandle handle, const std::string &name )
{
#ifdef MY_WINPLATFORM
   return reinterpret_cast<void *>(GetProcAddress( (HMODULE) handle, name.c_str() ));
#else
   char *errMsg;
   dlerror();// clear the error state, will not happen on success
   void *res = dlsym( handle, const_cast<char *>(name.c_str()) );
   errMsg = dlerror();
   return errMsg ? nullptr : res;
#endif
}

std::string P3MakeLibName( const std::string &path, const std::string &base )
{
   return path.empty() ? P3LibraryPrefix() + base + P3LibraryExt() : ExcludeTrailingPathDelimiter( path ) + PathDelim + P3LibraryPrefix() + base + P3LibraryExt();
}

TLibHandle P3NilLibHandle()
{
   return nullptr;
}

std::string P3LibraryExt()
{
   return OSDllExtension[OSPlatform()];
}

std::string P3LibraryPrefix()
{
   return OSDllPrefix[OSPlatform()];
}
}// namespace rtl::p3library