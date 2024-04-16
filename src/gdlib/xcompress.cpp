/*
* GAMS - General Algebraic Modeling System GDX API
 *
 * Copyright (c) 2017-2024 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2024 GAMS Development Corp. <support@gams.com>
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



#include "../rtl/p3library.h"
#include "../global/gmslibname.h"

#include "utils.h"

#include "xcompress.h"
#include "strutilx.h"
#include "global/unit.h"

using namespace rtl::p3library;
using namespace gdlib::strutilx;
using namespace std::literals::string_literals;

namespace gdlib::xcompress
{
using Tcompress = int ( * )( void *, ulong *, const void *, ulong );
using Tuncompress = int ( * )( void *, ulong *, const void *, ulong );
using TgzReadOpen = pgzFile ( * )( const char *, const char *mode );
using TgzRead = int ( * )( pgzFile, void *, ulong );
using TgzReadClose = int ( * )( pgzFile );

static TLibHandle ZLibHandle {};
static Tcompress pcompress {};
static Tuncompress puncompress {};
static TgzReadOpen pgzReadOpen {};
static TgzRead pgzRead {};
static TgzReadClose pgzReadClose {};

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
bool LoadZLibLibrary( const std::string &fn, std::string &LoadMsg )
{
   auto LoadEntry = [&LoadMsg]( const std::string &n, const std::string &wfn ) {
      if( !LoadMsg.empty() ) return (void *) nullptr;
      void *res { P3GetProcAddress( ZLibHandle, utils::lowercase( n ) ) };
      if( !res )
         LoadMsg = "Entry not found: " + n + " in " + wfn;
      return res;
   };

   LoadMsg.clear();
   if( !ZLibHandle )
   {
      const std::string Path = ExtractFilePathEx( fn );
      std::string baseName = ExtractFileNameEx( fn );
      if( baseName.empty() ) baseName = "gmszlib1";
      const std::string wfn = Path + global::gmslibname::gamslibnamep3( baseName );
      ZLibHandle = P3LoadLibrary( wfn, LoadMsg );
      if( ZLibHandle && LoadMsg.empty() )
      {
         static std::array<std::pair<std::string, void **>, 5> fncNamesAndPtrs = {
                 std::pair<std::string, void **> { "compress"s, reinterpret_cast<void **>( &pcompress) },
                 { "uncompress"s, reinterpret_cast<void **>(&puncompress) },
                 { "gzopen", reinterpret_cast<void **>(&pgzReadOpen) },
                 { "gzread", reinterpret_cast<void **>(&pgzRead) },
                 { "gzclose", reinterpret_cast<void **>(&pgzReadClose) } };
         for( std::pair<std::string, void **> &pair: fncNamesAndPtrs )
            *pair.second = LoadEntry( pair.first, wfn );
      }
   }

   if( !LoadMsg.empty() )
   {
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
   if( ZLibHandle )
   {
      P3FreeLibrary( ZLibHandle );
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

int compress( void *pdest, ulong &ldest, const void *psrc, ulong lsrc )
{
   return pcompress( pdest, &ldest, psrc, lsrc );
}

int uncompress( void *pdest, ulong &ldest, const void *psrc, ulong lsrc )
{
   return puncompress( pdest, &ldest, psrc, lsrc );
}

pgzFile gzReadOpen( const std::string &fn )
{
   return pgzReadOpen( fn.c_str(), "rb" );
}

int gzRead( pgzFile pgz, void *Buf, ulong ldest )
{
   return pgzRead( pgz, Buf, ldest );
}

int gzReadClose( pgzFile &pgz )
{
   int res { pgzReadClose( pgz ) };
   pgz = nullptr;
   return res;
}

static void initialization()
{
   UnloadZLibLibrary();//sets entry points to null
}

static void finalization()
{
}

UNIT_INIT_FINI();


}// namespace gdlib::xcompress