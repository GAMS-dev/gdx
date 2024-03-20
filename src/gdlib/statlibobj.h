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

#include <string>
#include "stattypes.h"
#include "gmsgen.h"

namespace gdlib::statlibobj
{
const int refreshInterval = 200, maxbuf = 2, maxerrchar = 150;

bool isMuxed();

enum tsl_status
{
   sl_closed,
   sl_file,
   sl_output
};

class TGMSLogStream
{
   void *Fgfusrmem;
   gdlib::stattypes::tgwrite *Fgfcb;
   bool FIDE, FLogEnabled;
   int FNestLevel;
   std::string FFullFileName, FFileName;
   int FLineNr;
   double FMemory;
   int FShowOSMem;
   int FErrorCnt;
   tsl_status FStatus;
   std::ostream *Ffcon;
   int FLenLast;
   bool FhasNewData;
   uint64_t FLastShowTicks, FLineStartTicks, FPrevSecs;
   char FSpinChar;
   bool FLastIsMsg;
   int FTraceLevel;
   std::string FSaveAfn;
   int FSaveAstat;
   std::string FRedirFileName;

#ifdef METER
   // measure the calls made to this object
   int cntLogLineNr, cntLogMemory, cntFreshen;
#endif

   void CheckOpen();
   void doFileAnchor( bool err, const std::string &fn, int line, int col );
   void CndShowStatLine();
   void ShowStatLine();
   void gstatStartWriting();
   void writeln_gf( const std::string &msg );
   void write_gf( const std::string &msg );

public:
   explicit TGMSLogStream( std::string &Msg );
   virtual ~TGMSLogStream();

   void registerWriteCallback( gdlib::stattypes::tgwrite *fptr, void *usermem );
   void LogClose();
   bool LogOpen( int Astat, gdlib::gmsgen::tfileaction AAction, const std::string &Afn );
   void LogReOpen();
   void LogMessage( const std::string &s );
   void LogFileName( const std::string &fn, int Lev );
   void LogLineNr( int N );
   void LogAnchor( int N );
   void LogTitleAnchor( const std::string &Msg );
   void LogFileAnchor( bool err, const std::string &fn, int line, int col );
   void LogMemory( double M );
   void LogErrrorCnt( int N );
   void freshen();
   void freshenEx();
   void LogDumpFilename( const std::string &prfx, bool enabled, const std::string &what, const std::string &gs, gdlib::gmsgen::tfileaction fa, int ioResOrNeg );
   void LogWriteLn( const std::string &s );
   void LogWrite( const std::string &p );
   void LogWritePlain( const std::string &s );
   void showCounts();

   // Properties
   void setIDErun( bool v );

   bool getLogEnabled() const;
   void setLogEnabled( bool v );

   int getTraceLevel() const;
   void setTraceLevel( int v );

   std::string getRedirFilename() const;
   std::string getRedirString() const;
   std::string getShortRedirString( const std::string &Dir );

   void SetOSMemory( int v );

   void Flush();
};

struct rcerrtxt {
   std::string errtxt;
   rcerrtxt *tnxttxt;
};
using trcerrtxt = rcerrtxt *;

struct rcerrtyp {
   int errcnt;
   rcerrtyp *tnxttyp;
   trcerrtxt txtindx;
};
using trcerrtyp = rcerrtyp *;

struct rcerrrec {
   int seqno, colnum;
   rcerrrec *tnxtrc;
   trcerrtyp tfrsttyp;
};
using trcerrrec = rcerrrec *;

class TGMSStatusStream
{
   void *Fgfusrmem {};
   gdlib::stattypes::tgwrite *Fgfcb {};
   global::delphitypes::Text Ffsysout {}, Ffstat;
   std::string Ffnstat;
   bool Fcopysysout {};
   bool Fwascopy {}, Fnofilename, Fstatusopen {};
   int Fprocesscount {}, Fdumpcount {};
   char Feditsymbol;
   int Fmodelrows, Fmodelcolumns;
   std::array<std::string, maxbuf> Fmsgbuf;
   int Fmsgbufcount {};
   trcerrrec Ffrsteqptr {};
   trcerrtxt Ffrsterrtxt {};
   trcerrrec Fnextheader {};
   trcerrtyp Fnexterror {};

   void writeln_gf( const std::string &msg );
   void write_gf( const std::string &msg );
   void SolverErrorInit();
   trcerrtxt SolverErrorMessage( const std::string &s );
   void SolverErrorAdd( int rownum, int columnnum, const std::string &msg );
   bool checkfile( std::string &msg );

   std::function<void( const std::string & )> commonStatusFunc( const std::string &s1, const std::string &s2 );

public:
   TGMSStatusStream( std::string &Msg );
   virtual ~TGMSStatusStream();

   void registerWriteCallback( gdlib::stattypes::tgwrite *fptr, void *usermem );
   void StatusErrorFree();
   void StatusDumpFirst();
   bool StatusAppend( const std::string &fn, std::string &msg );
   void StatusProcessFirst();
   bool StatusDumpNext( std::string &msg );
   bool StatusProcessNext( gdlib::stattypes::tstatusproc &statusproc, std::string &msg1, std::string &msg2, int &num );
   void StatusSetFilename( const std::string &fn );
   bool StatusDummy( std::string &msg );
   void StatusErrorFirst();
   bool StatusErrorNext( int &row, int &col );
   bool StatusErrorNextEqu( int &row );
   bool StatusErrorNextVar( int &col );
   bool StatusErrorNextJac( int &row, int &col );
   bool StatusErrorDetail( int &cnt, std::string &msg );
   void StatusSetRowCol( int rowmax, int colmax );
   void StatusClose();
   bool StatusFileOpen( gdlib::gmsgen::tfileaction AAction, std::string &msg );
   void StatusWriteLn( const std::string &s );
   void StatusWrite( const std::string &p );
   void StatusWritePlain( const std::string &s );
   void StatusTerminationRequestfromSolver();
   void StatusCopyOn();
   void StatusCopyOff();
   void StatusLSTAnchor( const std::string &s );
   void StatusCopyLine( const std::string &s );
   void StatusCopyFile( const std::string &s );
   void StatusCopyFileOnSysOut( const std::string &s );
   void StatusEndOfFile();
   void StatusSysOut();
   void StatusPageEject();
   void StatusSetEditSymbol( char c );
   void StatusAuditLine( const std::string &s );
   void StatusEquationName( const std::string &s1, int num, std::string &s2 );
   void StatusVariableName( const std::string &s1, int num, const std::string &s2 );
   void StatusEquationError( int num, const std::string &s );
   void StatusVariableError( int num, const std::string &s );
   void StatusJacobianError( int num1, int num2, const std::string &s );
};


};// namespace gdlib::statlibobj
