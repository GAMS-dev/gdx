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

#include "p3platform.hpp"
#include "unit.h"

using namespace std::literals::string_literals;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace rtl::p3platform
{

static tOSFileType localOSFileType;
static tOSPlatform localOSPlatform;

static std::string
        localOSNullFileName,
        localOSConsoleName,
        localOSLanguagePascal,
        localOSLanguageC;
static bool localIsLittleEndian;

tOSFileType OSFileType()
{
#if defined( WIN32 ) || defined( _WIN64 ) || defined( __WIN32__ ) || defined( _WIN32 ) || defined( __NT__ )
   return OSFileWIN;
#elif defined( __APPLE__ ) || defined( __linux__ ) || defined( __unix__ )
   return OSFileUNIX;
#else
   return OSFileMissing;
#endif
}

tOSPlatform OSPlatform()
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

static void initialization()
{
   localOSFileType = OSFileType();
   localOSPlatform = OSPlatform();

   switch( localOSFileType )
   {
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

static void finalization()
{
}

UNIT_INIT_FINI();

}// namespace rtl::p3platform
