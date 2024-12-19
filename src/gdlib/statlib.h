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



#pragma once
#include "gmsgen.h"
#include "stattypes.h"


/*

Status file operations

We are proceeding in several steps collect all status file operations
in one spot to be shared by everyone. This involves lots of
disentangeling and cleaning up very old stuff.

1. Collect all status file code in one unit for CMEX in gdlib\statlib.pas


2. Make unit compatible with convert. Existing functionality only.
   But complete hiding of implementation.

   A. problems with statusAppend:
   mpec and emp append a subsystem status file. The E/V numbers
   most likely will not be meaningful when sent back to GAMS. Also
   the editsymbols are not managed correctly. We mau need another
   append that drop the e/v reatled stuff.
   B. see fudge with assign(fstat,'.); in the initialization section


3. Make unit compatible with gmo and add required gmo capabilities.

4. moved the cmex log routines into statlib and make everything
   compatible with convert

7. Added  copy files without processing any =xxx
   =Afilename    copy into the final gams listing unconditional
   =Bfilename    copy only whene doing a sysout call. All other =xx
                 are ignored.


ToDo

5. make the log stuff compatible with GMO

6. Look over the code and so some cleanup

There is a testprogram status.dpr in gamscmex that shows some use


This are the old markers

   form =1,..,=9,=C,=E,=V (where the = char may be changed).
   Meaning:
     =0   solver signature
     =1   start to copy    any marker =xxx will stop copy
     =2   stop to copy
     =3   eof  (not longer needed)
     =4   set sysout
     =5   collect: rownumber errornumber message
     =6   collect: colnumber errornumber message
     =7   collect: rownumber colnumber errornumber message
     =8   eject - new page
     =9   new escape character
     =A   copy file unconditional
     =B   copy file when doing a sysout
     =C   copy rest of line
     =E   write equation name   "text" equ text
     =V   write variable name   "text" var equ
     =T   Termination request
     =L   Log anchor

*/

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace gdlib::statlib
{
void StatusSetFilename( const std::string &fn );
bool StatusDummy( std::string &msg );
void statusDumpFirst();
bool statusDumpNext( std::string &msg );
bool statusAppend( const std::string &fn, std::string &msg );
void statusProcessFirst();
bool statusProcessNext( stattypes::tstatusproc &statusproc, std::string &msg1, std::string &msg2, int &num );
void statusErrorFirst();
bool statusErrorNext( int &row, int &col );
bool statusErrorNextVar( int &col );
bool statusErrorNextJac( int &row, int &col );
bool statusErrorDetail( int &cnt, std::string &msg );
void statusErrorFree();

void statusSetRowCol(int rowmax, int colmax);

bool statusFileOpen(gmsgen::tfileaction AAction, std::string &msg);
void statusClose();

void gcstat(const std::string &s);
void gcstatPChar(const char *p);

// ...
// TODO: FIXME: Add missing functions!

// you need to adjust gmoxxx\gmodoorg.pas

void registerWriteCallback( gdlib::stattypes::tgwrite fptr, void *usermem );


void gstatMessage( const std::string &s );
bool gstatReady();
bool gstatOpen( int Astat, gmsgen::tfileaction AAction, const std::string &Afn );
void gstatLineNr( int N );
void gstatTraceLevel( int N );
void gstatFileAnchor( bool err, const std::string &fn, int line, int col );
void gstatAnchor( int N );
void gstatFileName( const std::string &fn, int Lev );
int gstatGetTraceLevel();
void gstatSetIDErun( bool IDErun );
void gstatClose();
void gstatFlush();
void gstatReOpen();

void gstatMemory(double M);
void gstatErrorCnt( int N );

// check if the log is "fresh enough", update if not
void gstatFreshen();
/*
*check if the log is "fresh enough", update if not:
called at regular time intervals, e.g. 200 ticks
*/
void gstatFreshenEx();

void dumpfilename( const std::string &prfx, bool enabled, const std::string &what, const std::string &gs, gmsgen::tfileaction fa, int ioResOrNeg );

bool gstatLogEnabled();

// ...
// TODO: Add missing functions!

std::string GetRedirFileName();
std::string GetRedirLogStr();
std::string GetShortRedirLogStr( const std::string &Dir );

void GcLog( const std::string &s );
void gstatSetLogEnabled( bool enablelog );
void gstatSetShowOSMem( int showOSMemory );
void gstatShowCounts();

void GcLogWritePlain( const std::string &s );
void GcStatWritePlain( const std::string &s );
}// namespace gdlib::statlib
