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
namespace rtl::p3platform
{
enum tOSFileType : uint8_t
{
   OSFileWIN,
   OSFileUNIX,
   OSFileMissing
};

const std::array<std::string, 3> OSFileTypeText { "WIN", "UNIX", "XXX" };

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

const std::array<std::string, OSPlatformCount> OSPlatformText {
        "WinNT",
        "Win64EMT",
        "Linux86_64",
        "Linux_arm64",
        "Darwin-x64",
        "Darwin-arm64",
        "Missing" };

const std::array<std::string, OSPlatformCount> OSDllExtension {
        ".dll",
        ".dll",
        ".so",
        ".so",
        ".dylib",
        ".dylib",
        ".XXX" };

const std::array<std::string, OSPlatformCount> OSDllPrefix {
        "",
        "",
        "lib",
        "lib",
        "lib",
        "lib",
        "lib" };

tOSFileType OSFileType();
tOSPlatform OSPlatform();
std::string OSNullFilename();
std::string OSConsoleName();
std::string OSLanguagePascal();
std::string OSLanguageC();
bool nativeIsLittleEndian();
}// namespace rtl::p3platform
