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



#include "statlib.hpp"
#include <string>
#include <memory>
#include "gmsgen.hpp"
#include "statlibobj.hpp"
#include "global/unit.h"

using namespace gdlib::gmsgen;
using namespace std::literals::string_literals;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace gdlib::statlib
{

static gdlib::statlibobj::TGMSLogStream *GMSLogObj {};
static gdlib::statlibobj::TGMSStatusStream *GMSStatusObj {};
static std::string msg;

void registerWriteCallback( stattypes::tgwrite *fptr, void *usermem );
void initialization();
void finalization();

bool statusFileOpen( const tfileaction AAction, std::string &msg )
{
   return GMSStatusObj->StatusFileOpen( AAction, msg );
}

void statusClose()
{
   GMSStatusObj->StatusClose();
}

void gcstat( const std::string &s )
{
   GMSStatusObj->StatusWriteLn( s );
}

void gcstatPChar( const char *p )
{
   GMSStatusObj->StatusWrite( p );
}

void registerWriteCallback( gdlib::stattypes::tgwrite *fptr, void *usermem )
{
   GMSStatusObj->registerWriteCallback( fptr, usermem );
   GMSLogObj->registerWriteCallback( fptr, usermem );
}

void gstatMessage( const std::string &s )
{
   if( GMSLogObj )
      GMSLogObj->LogMessage( s );
}


bool gstatReady()
{
   return GMSLogObj && GMSLogObj->getLogEnabled();
}

void dumpfilename( const std::string &prfx, const bool enabled, const std::string &what, const std::string &gs, const tfileaction fa, const int ioResOrNeg )
{
   GMSLogObj->LogDumpFilename( prfx, enabled, what, gs, fa, ioResOrNeg );
}

std::string GetRedirFileName()
{
   return GMSLogObj->getRedirFilename();
}

std::string GetRedirLogStr()
{
   return GMSLogObj->getRedirString();
}

std::string GetShortRedirLogStr( const std::string &Dir )
{
   return GMSLogObj->getShortRedirString( Dir );
}

void GcLog( const std::string &s )
{
   GMSLogObj->LogWriteLn( s );
}

void gstatSetLogEnabled( const bool enablelog )
{
   GMSLogObj->setLogEnabled( enablelog );
}

void gstatSetShowOSMem( const int showOSMemory )
{
   GMSLogObj->SetOSMemory( showOSMemory );
}

void gstatShowCounts()
{
   GMSLogObj->showCounts();
}

void GcLogWritePlain( const std::string &s )
{
   GMSLogObj->LogWritePlain( s );
}

void GcStatWritePlain( const std::string &s )
{
   GMSStatusObj->StatusWritePlain( s );
}

bool gstatOpen( const int Astat, const tfileaction AAction, const std::string &Afn )
{
   return GMSLogObj->LogOpen( Astat, AAction, Afn );
}

void gstatLineNr( const int N )
{
   GMSLogObj->LogLineNr( N );
}

void gstatTraceLevel( const int N )
{
   GMSLogObj->setTraceLevel( N );
}

void gstatFileAnchor( const bool err, const std::string &fn, const int line, const int col )
{
   GMSLogObj->LogFileAnchor( err, fn, line, col );
}

void gstatAnchor( const int N )
{
   GMSLogObj->LogAnchor( N );
}

void gstatFileName( const std::string &fn, const int Lev )
{
   GMSLogObj->LogFileName( fn, Lev );
}

int gstatGetTraceLevel()
{
   return GMSLogObj->getTraceLevel();
}

void gstatSetIDErun( const bool IDErun )
{
   GMSLogObj->setIDErun( IDErun );
}

void gstatClose()
{
   GMSLogObj->LogClose();
}

void gstatFlush()
{
   GMSLogObj->Flush();
}

void gstatReOpen()
{
   return GMSLogObj->LogReOpen();
}

void gstatMemory( const double M )
{
   if(GMSLogObj)
      GMSLogObj->LogMemory( M );
}

void gstatErrorCnt( const int N )
{
   return GMSLogObj->LogErrrorCnt( N );
}

void gstatFreshen()
{
   GMSLogObj->freshen();
}

void gstatFreshenEx()
{
   GMSLogObj->freshenEx();
}

bool gstatLogEnabled()
{
   return GMSLogObj->getLogEnabled();
}

void StatusSetFilename( const std::string &fn )
{
   GMSStatusObj->StatusSetFilename( fn );
}

bool StatusDummy( std::string &msg )
{
   return GMSStatusObj->StatusDummy( msg );
}

void statusDumpFirst()
{
   GMSStatusObj->StatusDumpFirst();
}

bool statusDumpNext( std::string &msg )
{
   return GMSStatusObj->StatusDumpNext( msg );
}

bool statusAppend( const std::string &fn, std::string &msg )
{
   return GMSStatusObj->StatusAppend( fn, msg );
}

void statusProcessFirst()
{
   GMSStatusObj->StatusProcessFirst();
}

bool statusProcessNext( stattypes::tstatusproc &statusproc, std::string &msg1, std::string &msg2, int &num )
{
   return GMSStatusObj->StatusProcessNext( statusproc, msg1, msg2, num );
}

void statusErrorFirst()
{
   return GMSStatusObj->StatusErrorFirst();
}

bool statusErrorNext( int &row, int &col )
{
   return GMSStatusObj->StatusErrorNext( row, col );
}

bool statusErrorNextVar( int &col )
{
   return GMSStatusObj->StatusErrorNextVar( col );
}

bool statusErrorNextJac( int &row, int &col )
{
   return GMSStatusObj->StatusErrorNextJac( row, col );
}

bool statusErrorDetail( int &cnt, std::string &msg )
{
   return GMSStatusObj->StatusErrorDetail( cnt, msg );
}

void statusErrorFree()
{
   GMSStatusObj->StatusErrorFree();
}

void initialization()
{
   // Maybe put on stack?
   GMSLogObj = new gdlib::statlibobj::TGMSLogStream { msg };
   GMSStatusObj = new gdlib::statlibobj::TGMSStatusStream { msg };
}

void finalization()
{
   delete GMSStatusObj;
   delete GMSLogObj;
   GMSStatusObj = nullptr;
   GMSLogObj = nullptr;
}

UNIT_INIT_FINI();

}// namespace gdlib::statlib