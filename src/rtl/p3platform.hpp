/*
* GAMS - General Algebraic Modeling System GDX API
 *
 * Copyright (c) 2017-2026 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2026 GAMS Development Corp. <support@gams.com>
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

#pragma once
#include <string>
#include <array>
#include <cstdint>

#if defined( _WIN32 )
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif


// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace GDX_NS rtl::p3platform
{
enum tOSFileType : uint8_t
{
   OSFileWIN,
   OSFileUNIX,
   OSFileMissing
};

constexpr std::array OSFileTypeText { "WIN", "UNIX", "XXX" };

enum tOSPlatform : uint8_t
{
   OSWindowsNT,
   OSWindows64EMT,
   OSLinux86_64,
   OSLinux_arm64,
   OSDarwin_x64,
   OSDarwin_arm64,
   OSMissing,
   OSPlatformCount
};

constexpr std::array<std::string_view, OSPlatformCount> OSPlatformText {
        "WinNT",
        "Win64EMT",
        "Linux86_64",
        "Linux_arm64",
        "Darwin-x64",
        "Darwin-arm64",
        "Missing" };

constexpr std::array<std::string_view, OSPlatformCount> OSDllExtension {
        ".dll",
        ".dll",
        ".so",
        ".so",
        ".dylib",
        ".dylib",
        ".XXX" };

constexpr std::array<std::string_view, OSPlatformCount> OSDllPrefix {
        "",
        "",
        "lib",
        "lib",
        "lib",
        "lib",
        "lib" };

constexpr tOSFileType OSFileType()
{
#if defined( WIN32 ) || defined( _WIN64 ) || defined( __WIN32__ ) || defined( _WIN32 ) || defined( __NT__ )
   return OSFileWIN;
#elif defined( __APPLE__ ) || defined( __linux__ ) || defined( __unix__ )
   return OSFileUNIX;
#else
   return OSFileMissing;
#endif
}

constexpr tOSPlatform OSPlatform()
{
#if defined( _WIN64 ) || defined( WIN32 ) || defined( __WIN32__ )
   return OSWindows64EMT;
#elif defined( __APPLE__ )
   #if defined( __x86_64__ ) || defined( _M_X64 )
      return OSDarwin_x64;
   #else
      return OSDarwin_arm64;
   #endif
#elif defined( __linux__ )
   #if defined( __x86_64__ ) || defined( _M_X64 )
      return OSLinux86_64;
   #else
      return OSLinux_arm64;
   #endif
#else
   return OSMissing;
#endif
}

constexpr auto OSNullFilename()
{

   switch( OSFileType() )
   {
   case OSFileWIN:
      return "nul";
   case OSFileUNIX:
      return "/dev/null";
   case OSFileMissing:
      return "";
   }

   // this is needed to compile with GCC in debug mode
   return "";
}

}// namespace rtl::p3platform

namespace rtl {
namespace p3platform = GDX_NS rtl::p3platform;
}
