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


#include <string>
#include <filesystem>
#include <iostream>

#include "../global/delphitypes.h"
#include "../gdlib/utils.h"
#include "../rtl/sysutils_p3.h"
#include "gfileopen.h"

using namespace global::delphitypes;
using namespace std::literals::string_literals;
using namespace rtl::p3utils;
using namespace rtl::sysutils_p3;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace gdlib::gfileopen
{

enum TfoAction : uint8_t
{
   foRewrite,
   foReset,
   foAppend
};

const int MaxTry { 3 };

const std::array sAction { "ReWrite"s, "Reset"s, "Append"s };

static void ReportRetry( std::string_view Func, std::string_view fn, TfoAction Action, int TryCount, int IORes1 )
{
   debugStream << "*** Retry count succeeded in function "s << Func << "\n";
   debugStream << "*** Count = "s << rtl::sysutils_p3::IntToStr( TryCount ) << " Action = "s << sAction[Action] << " IORes = "s << IORes1 << " File = "s << fn << "\n";
}

// TODO: Reduce usage of std::fstream for performance reasons!
static int TextFileOpenRetry( const std::string &fn, bool ReTry, TfoAction Action, std::fstream &FileHandle, int &IORes )
{
   uint32_t Delay {};
   int res {};
   IORes = 0;
   int IORes1;
   while( true )
   {
      IORes1 = IORes;
#if defined(_WIN32)
      std::ios_base::openmode flags {};
#else
      std::ios_base::openmode flags {std::fstream::binary};
#endif
      switch( Action )
      {
         case foRewrite:
            flags = std::fstream::out;
            break;
         case foReset:
            flags = std::fstream::in;// | std::fstream::trunc;
            break;
         case foAppend:
            flags = std::fstream::out | std::fstream::app;
            break;
      }
      if( Action != foReset || ( FileExists( fn ) && !DirectoryExists( fn ) ) )
      {
         FileHandle.open( fn, flags );
         if( !FileHandle.is_open() )
         {
#if !defined(NDEBUG)
            debugStream << "Unable to open file "s << fn << '\n';
#endif
            IORes = 1;
         }
      }
      else
      {
#if !defined(NDEBUG)
         debugStream << "*** INFO: Tried opening file:\n*** "s << fn << " in TextFileOpenRetry.\n"s
                   << "*** File does not exist. Maybe test, maybe problem."s << '\n';
#endif
         IORes = 1;
      }
      if( !IORes || !ReTry || res >= MaxTry ) break;
      res++;
      rtl::sysutils_p3::Sleep( Delay );
      Delay *= 10;
   }
   if( !IORes && res > 0 )
      ReportRetry( "TextFileOpenRetry"s, fn, Action, res, IORes1 );
   return res;
}

static int TextFileOpenRetry( const std::string &fn, bool ReTry, TfoAction Action, FILE *&FileHandle, int &IORes )
{
   uint32_t Delay {};
   int res {};
   IORes = 0;
   int IORes1;
   std::string flags;
   switch( Action )
   {
      case foRewrite:
         flags = "w";
         break;
      case foReset:
         flags = "r";
         break;
      case foAppend:
         flags = "a";
         break;
   }
#if !defined(_WIN32)
   flags += 'b';
#endif
   while( true )
   {
      IORes1 = IORes;
      if( Action != foReset || ( FileExists( fn ) && !DirectoryExists( fn ) ) )
      {
         FileHandle = fopen( fn.c_str(), flags.c_str() );
         if( !FileHandle )
         {
#if !defined( NDEBUG )
            debugStream << "Unable to open file "s << fn << '\n';
#endif
            assert( errno > 0 );
            IORes = errno;
         }
      }
      else
      {
#if !defined( NDEBUG )
         debugStream << "*** INFO: Tried opening file:\n*** "s << fn << " in TextFileOpenRetry.\n"s
                     << "*** File does not exist. Maybe test, maybe problem."s << '\n';
#endif
         IORes = 1;
      }
      if( !IORes || !ReTry || res >= MaxTry ) break;
      res++;
      rtl::sysutils_p3::Sleep( Delay );
      Delay *= 10;
   }
   if( !IORes && res > 0 )
      ReportRetry( "TextFileOpenRetry"s, fn, Action, res, IORes1 );
   return res;
}

// FIXME: How are untyped files with arbitrary record size for reading/writing treated correctly in C++?
// Default record size should be 128 bytes
// FIXME: Take record size into account in seek, block r/w, typed r/w , filesize, filepos. See: p3/base/rtl/p3io.c
static int UntypedFileOpenRetry( const std::string &fn, int RecSize, bool ReTry, TfoAction Action, std::fstream &FileHandle, int &IORes )
{
   uint32_t Delay { 10 };
   int res {}, IORes1;
   IORes = 0;
   while( true )
   {
      IORes1 = IORes;
#if defined(_WIN32)
      std::ios_base::openmode flags {std::fstream::binary};
#else
      std::ios_base::openmode flags {};
#endif
      switch( Action )
      {
         case foRewrite:
            flags = std::fstream::out;
            break;
         case foReset:
            flags = std::fstream::in;// | std::fstream::trunc;
            break;
         case foAppend:
            flags = std::fstream::out | std::fstream::app;
            break;
      }
      FileHandle.open( fn, flags );
      if( !FileHandle.is_open() )
         IORes = 1;
      if( !IORes || !ReTry || res >= MaxTry ) break;
      res++;
      rtl::sysutils_p3::Sleep( Delay );
      Delay *= 10;
   }
   if( !IORes && res > 0 )
      ReportRetry( "UntypedFileOpenRetry"s, fn, Action, res, IORes1 );
   return res;
}

// FIXME: Respect record size in seek, block r/w, typed r/w , filesize, filepos. See: p3/base/rtl/p3io.c
static int UntypedFileOpenRetry( const std::string &fn, int RecSize, bool ReTry, TfoAction Action, Tp3FileHandle &FileHandle, int &IORes )
{
   uint32_t Delay { 10 };
   int res {}, IORes1;
   IORes = 0;
   while( true )
   {
      IORes1 = IORes;
      Tp3FileOpenAction mode;
      switch( Action )
      {
         case foRewrite:
            mode = p3OpenWrite;
            break;
         case foReset:
            mode = p3OpenRead;
            break;
         default:
            throw std::runtime_error("Other cases than read and write not properly handled in FileOpenRetry!"s);
      }
      if( p3FileOpen( fn, mode, FileHandle ) )
         IORes = 1;
      if( !IORes || !ReTry || res >= MaxTry ) break;
      res++;
      rtl::sysutils_p3::Sleep( Delay );
      Delay *= 10;
   }
   if( !IORes && res > 0 )
      ReportRetry( "UntypedFileOpenRetry"s, fn, Action, res, IORes1 );
   return res;
}

// FIXME: Use RecSize in fread/write ops!
static int UntypedFileOpenRetry( const std::string &fn, int RecSize, bool ReTry, TfoAction Action, FILE *&FileHandle, int &IORes )
{
   uint32_t Delay { 10 };
   int res {}, IORes1;
   IORes = 0;
   while( true )
   {
      IORes1 = IORes;
      std::string mode;
      switch( Action )
      {
         case foRewrite:
            mode = "w"s;
            break;
         case foReset:
            mode = "r"s;
            break;
         default:
            throw std::runtime_error("Other cases than read and write not properly handled in FileOpenRetry!"s);
      }
#if defined(_WIN32)
      mode += "b"s;
#endif
      FileHandle = std::fopen( fn.c_str(), mode.c_str() );
      if( !FileHandle )
         IORes = 1;
      if( !IORes || !ReTry || res >= MaxTry ) break;
      res++;
      rtl::sysutils_p3::Sleep( Delay );
      Delay *= 10;
   }
   if( !IORes && res > 0 )
      ReportRetry( "UntypedFileOpenRetry"s, fn, Action, res, IORes1 );
   return res;
}

int grRewrite( const std::string &fn, bool ReTry, std::fstream &FileHandle, int &IORes )
{
   return TextFileOpenRetry( fn, ReTry, foRewrite, FileHandle, IORes );
}

int grReset( const std::string &fn, bool ReTry, std::fstream &FileHandle, int &IORes )
{
   return TextFileOpenRetry( fn, ReTry, foReset, FileHandle, IORes );
}

int grAppend( const std::string &fn, bool ReTry, std::fstream &FileHandle, int &IORes )
{
   return TextFileOpenRetry( fn, ReTry, foAppend, FileHandle, IORes );
}

int grRewrite( const std::string &fn, bool ReTry, FILE *&FileHandle, int &IORes )
{
   return TextFileOpenRetry( fn, ReTry, foRewrite, FileHandle, IORes );
}

int grReset( const std::string &fn, bool ReTry, FILE *&FileHandle, int &IORes )
{
   return TextFileOpenRetry( fn, ReTry, foReset, FileHandle, IORes );
}

int grAppend( const std::string &fn, bool ReTry, FILE *&FileHandle, int &IORes )
{
   return TextFileOpenRetry( fn, ReTry, foAppend, FileHandle, IORes );
}

int grResetUntyped( const std::string &fn, int RecSize, bool ReTry, std::fstream &FileHandle, int &IORes )
{
   return UntypedFileOpenRetry( fn, RecSize, ReTry, foReset, FileHandle, IORes );
}

int grRewriteUntyped( const std::string &fn, int RecSize, bool ReTry, std::fstream &FileHandle, int &IORes )
{
   return UntypedFileOpenRetry( fn, RecSize, ReTry, foRewrite, FileHandle, IORes );
}

int grResetUntyped( const std::string &fn, int RecSize, bool ReTry, Tp3FileHandle &FileHandle, int &IORes )
{
   return UntypedFileOpenRetry( fn, RecSize, ReTry, foReset, FileHandle, IORes );
}

int grRewriteUntyped( const std::string &fn, int RecSize, bool ReTry, Tp3FileHandle &FileHandle, int &IORes )
{
   return UntypedFileOpenRetry( fn, RecSize, ReTry, foRewrite, FileHandle, IORes );
}

int grResetUntyped( const std::string &fn, int RecSize, bool ReTry, FILE *&FileHandle, int &IORes )
{
   return UntypedFileOpenRetry( fn, RecSize, ReTry, foReset, FileHandle, IORes );
}

int grRewriteUntyped( const std::string &fn, int RecSize, bool ReTry, FILE *&FileHandle, int &IORes )
{
   return UntypedFileOpenRetry( fn, RecSize, ReTry, foRewrite, FileHandle, IORes );
}

}// namespace gdlib::gfileopen
