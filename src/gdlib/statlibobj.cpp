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



#include <iostream>
#include <chrono>

#include "../rtl/sysutils_p3.h"
#include "../rtl/p3platform.h"
#include "../rtl/p3utils.h"
#include "../rtl/idglobal_p3.h"

#include "utils.h"

#include "strutilx.h"
#include "statlibobj.h"
#include "gmsgen.h"
#include "global/unit.h"

using namespace gdlib::gmsgen;
using namespace rtl::sysutils_p3;
using namespace std::literals::string_literals;
using namespace global::delphitypes;

namespace gdlib::statlibobj
{
bool isMuxed()
{
   return false;
}

std::string termString;

static uint64_t GetTickCount()
{
   const auto duration = std::chrono::system_clock::now().time_since_epoch();
   return std::chrono::duration_cast<std::chrono::milliseconds>( duration ).count();
}

static bool tmElapsed( uint64_t &thenT, uint64_t minDiffT )
{
   auto nowT = GetTickCount();
   auto diffT = rtl::idglobal_p3::GetTickDiff( thenT, nowT );
   if( diffT > minDiffT )
   {
      thenT = nowT;
      return true;
   }
   return false;
}

static void staterror( const std::string &s )
{
   debugStream << "*** StatLib." << s << '\n';
}

static void statuslerror( const std::string &s )
{
   debugStream << "*** gStatusL." << s << '\n';
}

bool opentextmsg( global::delphitypes::Text f, const std::string &fn, gdlib::gmsgen::tfileaction fa, std::string &msg );

statlibobj::TGMSLogStream::TGMSLogStream( std::string &Msg ) : FLastShowTicks { GetTickCount() }
{
   Msg.clear();
}

void statlibobj::TGMSLogStream::registerWriteCallback( gdlib::stattypes::tgwrite *fptr, void *usermem )
{
   Fgfcb = fptr;
   Fgfusrmem = usermem;
}

void statlibobj::TGMSLogStream::LogClose()
{
   if( FStatus != sl_closed )
   {
      if( FLogEnabled )
      {
         Flush();
#if defined(__IN_CPPMEX__)
         if( FSaveAstat == 4 )
            std::fflush( stdout );
#endif
         //Ffcon->close();
      }
      FStatus = sl_closed;
   }
}

// FIXME: AS: Finish porting this method!
bool statlibobj::TGMSLogStream::LogOpen( int Astat, gdlib::gmsgen::tfileaction AAction, const std::string &Afn )
{
   int ioRes;
   int IDECMDSrun;
   //int IDECMDSrc;
   //int IDECMDSsc;
   if( FStatus != sl_closed )
   {
      if( FLogEnabled )
      {
         Flush();
         if( FStatus == sl_file )
            dynamic_cast<std::ofstream *>( Ffcon )->close();
      }
      FStatus = sl_closed;
   }

   bool resetLo1to3 {};
   Astat = std::max( 0, std::min( 4, Astat ) );
   if( Astat == 1 )
   {
      Astat = 3;
      resetLo1to3 = true;
   }
   //save for gstatReOpen
   FSaveAfn = Afn;
   FSaveAstat = Astat;

   switch( Astat )
   {
      case 0:
         FLogEnabled = false;
         FRedirFileName = rtl::p3platform::OSNullFilename();
         FStatus = sl_output;
         break;

      case 3:// standard output (stdout)
         FLogEnabled = true;
         FRedirFileName.clear();
         //Ffcon = new std::fstream("", std::ios_base::out);
         Ffcon = &debugStream;
         ioRes = Ffcon->good() ? 0 : Ffcon->rdstate();
         if( ioRes ) statuslerror( "LogOpen-2: cannot write to standard output"s );
         FStatus = sl_output;
         break;

      case 2:// file
      case 4:// file+stdout
         FLogEnabled = true;
         FRedirFileName = Afn;
         IDECMDSrun = 0;
         while(IDECMDSrun <= 1)
         {
            IDECMDSrun += 2;
            Ffcon = new std::ofstream(Afn);
            if(AAction == forAppend)
            {
               // ...
            }
         }
         // ...
         STUBWARN();
         break;

      default:
         break;
   }

   if( resetLo1to3 )
      writeln_gf( "--- LogOption value 1 is deprecated, reset to 3"s );

   FLenLast = 0;
   return false;
}

void statlibobj::TGMSLogStream::LogReOpen()
{
   LogOpen( FSaveAstat, forAppend, FSaveAfn );
}

void statlibobj::TGMSLogStream::LogMessage( const std::string &s )
{
   if( !FLogEnabled ) return;
   Flush();
   CheckOpen();
   write_gf( s );
   Ffcon->flush();
   FLenLast = static_cast<int>(s.length());
   FLastIsMsg = true;
}

void statlibobj::TGMSLogStream::LogFileName( const std::string &fn, int Lev )
{
   if( !FLogEnabled ) return;
   if( FhasNewData && FTraceLevel > 0 ) ShowStatLine();
   FFullFileName = fn;
   FFileName = gdlib::strutilx::ExtractFileNameEx( fn );
   int OldLevel = FNestLevel;
   FNestLevel = Lev;
   if( !( FNestLevel < OldLevel || FLastIsMsg || FTraceLevel > 2 ) ) FLastIsMsg = true;
   else
   {
      if( FTraceLevel > 0 && FStatus != sl_closed && FLogEnabled )
      {
         writeln_gf( "" );
         FLenLast = 0;
      }
      FErrorCnt = 0;
      FLastIsMsg = false;
   }

   /*std::string nestpoints(Lev, '.');
		std::cout << "--- " << nestpoints << " " << fn << "(" << Lev << ") 0 Mb" << '\n';*/
}

void statlibobj::TGMSLogStream::LogLineNr( int N )
{
   if( FLogEnabled && N != FLineNr )
   {
      FLineNr = N;
      FhasNewData = true;
      FLineStartTicks = GetTickCount();
      FSpinChar = ' ';
      //FPrevSecs = 0,
      //CndShowStatLine();
      if( FPrevSecs > 0 )
      {
         FLastIsMsg = true;
         FPrevSecs = 0;
         ShowStatLine();
      }
      else
         CndShowStatLine();
   }
}

void statlibobj::TGMSLogStream::LogAnchor( int N )
{
   if( FLogEnabled )
   {
      std::string s = "[LST:"s + std::to_string( N ) + "]"s;
      CheckOpen();
      write_gf( s );
      FLenLast += static_cast<int>(s.length());
   }
}

void statlibobj::TGMSLogStream::LogTitleAnchor( const std::string &Msg )
{
   if( FLogEnabled )
   {
      gstatStartWriting();
      writeln_gf( "[TIT:" + Msg + "]" );
      Ffcon->flush();
   }
}

void statlibobj::TGMSLogStream::LogFileAnchor( bool err, const std::string &fn, int line, int col )
{
   if( !FLogEnabled ) return;
   doFileAnchor( err, fn, line, col );
}

void statlibobj::TGMSLogStream::LogMemory( double M )
{
   if( FLogEnabled && std::abs( M - FMemory ) > 0.5 )
   {
      FMemory = M;
      FhasNewData = true;
      CndShowStatLine();
   }
}

void statlibobj::TGMSLogStream::LogErrrorCnt( int N )
{

   if( FLogEnabled && N != FErrorCnt )
   {
      FErrorCnt = N;
      FhasNewData = true;
      CndShowStatLine();
   }
}

void statlibobj::TGMSLogStream::freshen()
{
   if( FLogEnabled ) CndShowStatLine();
}

void statlibobj::TGMSLogStream::freshenEx()
{
   if( FLogEnabled && FStatus == sl_output && FTraceLevel > 1 )
   {
      auto nowT = GetTickCount();
      auto dt = rtl::idglobal_p3::GetTickDiff( FLineStartTicks, nowT );
      FLastShowTicks = nowT;
      auto secs = dt / 1000;
      if( secs > 2 && secs > FPrevSecs && ( secs <= 30 || secs - 5 >= FPrevSecs ) )
      {
         FPrevSecs = secs;
         FhasNewData = true;
      }
      if( FhasNewData )
      {
         ShowStatLine();
         Ffcon->flush();
      }
   }
}

void statlibobj::TGMSLogStream::LogDumpFilename( const std::string &prfx, bool enabled, const std::string &what, const std::string &gs, gdlib::gmsgen::tfileaction fa, int ioResOrNeg )
{
   if( !enabled || !FLogEnabled ) return;
   std::string s { prfx + " "s + what + " "s };
   switch( fa )
   {
      case forRead:
         s += 'R';
         break;
      case forWrite:
         s += 'W';
         break;
      case forAppend:
         s += 'A';
         break;
   }

   s += "="s + std::to_string( ioResOrNeg ) + " FN=\""s + gs + "\""s;
   LogMessage( s );
   if( ioResOrNeg > 0 )
      LogMessage( prfx + "MSG=" + SysErrorMessage( ioResOrNeg ) );
}

void statlibobj::TGMSLogStream::LogWriteLn( const std::string &s )
{
   if( FLogEnabled )
   {
      if( !Fgfcb )
      {
         gstatStartWriting();
         writeln_gf( s );
         Ffcon->flush();
      }
      else
         writeln_gf( s );
   }
}

void statlibobj::TGMSLogStream::LogWrite( const std::string &p )
{
   const int blockSize = 255;
   if( p.empty() || !FLogEnabled ) return;
   if( !Fgfcb ) gstatStartWriting();
   const int numBlocks = static_cast<int>( std::ceil( static_cast<double>( p.length() ) / static_cast<double>( blockSize ) ) );
   for( int i = 0; i <= numBlocks; i++ )
   {
      std::string s { p.substr( i * blockSize, blockSize ) };
      write_gf( s );
   }
   if( !Fgfcb ) Ffcon->flush();
}

void statlibobj::TGMSLogStream::LogWritePlain( const std::string &s )
{
   if( !FLogEnabled ) return;
   *Ffcon << s;
   if( FSaveAstat == 4 )
      debugStream << s;
   Ffcon->flush();
}

void statlibobj::TGMSLogStream::showCounts()
{
#ifdef METER
   LogMessage( "DEBUG: TGMSLogStream usage counts:" );
   LogMessage( "  method LogLineNr: " + std::to_string( cntLogLineNr ) );
   LogMessage( "  method LogMemory: " + std::to_string( cntLogMemory ) );
   LogMessage( "  method   freshen: " + std::to_string( cntFreshen ) );
   LogMessage( "" );
#endif
}

void statlibobj::TGMSLogStream::setIDErun( bool v )
{
   FIDE = v;
}

bool statlibobj::TGMSLogStream::getLogEnabled() const
{
   return FLogEnabled;
}

void statlibobj::TGMSLogStream::setLogEnabled( bool v )
{
   FLogEnabled = v;
}

int statlibobj::TGMSLogStream::getTraceLevel() const
{
   return FTraceLevel;
}

void statlibobj::TGMSLogStream::setTraceLevel( int v )
{
   FTraceLevel = v;
}

std::string statlibobj::TGMSLogStream::getRedirFilename() const
{
   return FRedirFileName.empty() ? ""s : "\"" + FRedirFileName + "\"";
}

std::string statlibobj::TGMSLogStream::getRedirString() const
{
   if( FRedirFileName.empty() ) return ""s;
   std::string pf = FSaveAstat == 4 ? " | tee -a " : " >> ";
   return pf + getRedirFilename();
}

std::string statlibobj::TGMSLogStream::getShortRedirString( const std::string &Dir )
{
   if( FRedirFileName.empty() ) return ""s;
   auto k { Dir.find( FRedirFileName ) };
   return k == std::string::npos ? getRedirString() : " >> \"." + FRedirFileName.substr( Dir.length() ) + "\"";
}

void statlibobj::TGMSLogStream::SetOSMemory( int v )
{
   FShowOSMem = v;
}

void statlibobj::TGMSLogStream::CheckOpen()
{
   if( FStatus == sl_closed )
   {
      Ffcon = &debugStream;// new std::fstream("", std::ios_base::out);
      debugStream << "*** Log reopened as output\n";
      FStatus = sl_output;
   }
}

void statlibobj::TGMSLogStream::Flush()
{
   if( FhasNewData && FTraceLevel > 0 ) ShowStatLine();
   if( FLenLast > 0 && FStatus != sl_closed )
   {
      writeln_gf( "" );
      FLenLast = 0;
      FErrorCnt = 0;
      Ffcon->flush();
   }
   FLastIsMsg = false;
}

void statlibobj::TGMSLogStream::doFileAnchor( bool err, const std::string &fn, int line, int col )
{
   CheckOpen();
   std::string s { ( err ? "[ERR:"s : "[FIL:"s ) + "\"" + fn + "\"," + std::to_string( line ) + "," + std::to_string( col ) + "]" };
   write_gf( s );
   FLenLast += static_cast<int>(s.length());
}

void statlibobj::TGMSLogStream::CndShowStatLine()
{
   if( FStatus == sl_output && FTraceLevel > 1 )
   {
      if( !tmElapsed( FLastShowTicks, refreshInterval ) ) return;
      ShowStatLine();
      Ffcon->flush();
   }
}

void statlibobj::TGMSLogStream::ShowStatLine()
{
   if( FLineNr > 0 )
   {
      CheckOpen();
      if( FLastIsMsg )
      {
         writeln_gf( "" );
         FLenLast = 0;
         FLastIsMsg = false;
      }
      std::string s = "--- "s;
      if( FNestLevel > 0 )
         s += std::string( FNestLevel, '.' ) + " "s;
      s += FFileName + "("s + std::to_string( FLineNr ) + ")"s;
      uint64_t rss, vss;
      if( FShowOSMem == 1 && rtl::p3utils::p3GetMemoryInfo( rss, vss ) ) FMemory = (double) rss / 1e6;
      else if( FShowOSMem == 2 && rtl::p3utils::p3GetMemoryInfo( rss, vss ) )
         FMemory = (double) vss / 1e6;
      // FIXME: Do actual memory usage computation and do not discard it!
      FMemory = 0;
      s += " "s + std::to_string( (int) std::round( FMemory ) ) + " Mb"s;
      if( FErrorCnt > 0 )
      {
         s += " "s + std::to_string( FErrorCnt ) + " "s;
         s += "Error"s + ( FErrorCnt > 1 ? "s"s : ""s );
      }
      if( FSpinChar != ' ' ) s += " "s + FSpinChar;
      if( FPrevSecs > 0 ) s += " " + std::to_string( FPrevSecs ) + " secs";
      if( FLenLast > 0 ) write_gf( "\r" );
      if( (int) s.length() < FLenLast )
      {
         write_gf( std::string( FLenLast, ' ' ) + "\r" );
      }
      write_gf( s );
      Ffcon->flush();
      FLenLast = static_cast<int>( s.length() );
      if( FIDE ) doFileAnchor( false, FFullFileName, FLineNr, 0 );
   }
   FhasNewData = false;
}

void statlibobj::TGMSLogStream::gstatStartWriting()
{
   CheckOpen();
   Flush();
}

void statlibobj::TGMSLogStream::writeln_gf( const std::string &msg )
{
   write_gf( msg + termString );
}


void statlibobj::TGMSLogStream::write_gf( const std::string &msg )
{
   if( Fgfcb ) ( *Fgfcb )( msg, gdlib::stattypes::doLog, Fgfusrmem );
   else
   {
      *Ffcon << msg;
      if( FSaveAstat == 4 ) debugStream << msg;
   }
}


void statlibobj::TGMSStatusStream::writeln_gf( const std::string &msg )
{
   write_gf( msg + termString );
}

void statlibobj::TGMSStatusStream::write_gf( const std::string &msg )
{
   if( Fgfcb ) ( *Fgfcb )( msg, gdlib::stattypes::doStat, Fgfusrmem );
   else
      *Ffstat << msg;
}

void statlibobj::TGMSStatusStream::SolverErrorInit()
{
   Ffrsteqptr = new rcerrrec {
           std::numeric_limits<int>::max(),
           std::numeric_limits<int>::max(),
   };
   Ffrsterrtxt = nullptr;
}

trcerrtxt statlibobj::TGMSStatusStream::SolverErrorMessage( const std::string &s )
{
   std::string msg = utils::trim( s );
   if( msg.empty() ) msg = "No message";

   trcerrtxt nextmsg;
   for( nextmsg = Ffrsterrtxt; nextmsg; nextmsg = nextmsg->tnxttxt )
   {
      if( utils::sameText( nextmsg->errtxt, msg ) )
      {
         return nextmsg;
      }
   }

   // enter new message
   nextmsg = new rcerrtxt {
           msg,
           Ffrsterrtxt };
   Ffrsterrtxt = nextmsg;
   return nextmsg;
}

void statlibobj::TGMSStatusStream::SolverErrorAdd( int rownum, int columnnum, const std::string &msg )
{
   trcerrrec ehdrptr { Ffrsteqptr }, nextrc { ehdrptr };
   bool firsttime { true };
   trcerrtxt newerr { SolverErrorMessage( msg ) };

   while( rownum > nextrc->seqno )// find row
      nextrc = nextrc->tnxtrc;

   if( rownum == nextrc->seqno )
   {// look for matching col, could be 0
      while( rownum == nextrc->seqno && columnnum > nextrc->colnum )
         nextrc = nextrc->tnxtrc;
      if( rownum == nextrc->seqno && columnnum == nextrc->colnum )
         firsttime = false;
   }

   if( firsttime )
   {// first time on this equ/var
      auto newtyp = new rcerrtyp {
              1,
              nullptr,
              newerr };
      auto newrc = new rcerrrec {
              nextrc->seqno,
              nextrc->colnum,
              nextrc->tnxtrc,
              nextrc->tfrsttyp };
      nextrc->seqno = rownum;
      nextrc->colnum = columnnum;
      nextrc->tfrsttyp = newtyp;
      nextrc->tnxtrc = newrc;
   }
   else
   {
      firsttime = true;
      auto nexttyp = nextrc->tfrsttyp;
      while( nexttyp )
      {
         if( nexttyp->txtindx == newerr )
         {
            nexttyp->errcnt++;
            nexttyp = nullptr;
            firsttime = false;
         }
         else
            nexttyp = nexttyp->tnxttyp;
      }
      if( firsttime )
      {
         auto newtyp = new rcerrtyp {
                 1,
                 nextrc->tfrsttyp,
                 newerr };
         nextrc->tfrsttyp = newtyp;
      }
   }
}

bool statlibobj::TGMSStatusStream::checkfile( std::string &msg )
{
   if( Fstatusopen )
   {
      Ffstat->close();
      Fstatusopen = false;
      msg.clear();
   }
   if( Fnofilename )
   {
      msg = "Status filename has not been assigned";
      return false;
   }
   return true;
}

statlibobj::TGMSStatusStream::TGMSStatusStream( std::string &Msg ) : Ffstat { new std::fstream( "." ) },
                                                                     Ffnstat { "unknown file name"s },
                                                                     Fnofilename { true },
                                                                     Feditsymbol { '=' },
                                                                     Fmodelrows { std::numeric_limits<int>::max() },
                                                                     Fmodelcolumns { std::numeric_limits<int>::max() }
{
   Msg.clear();
}

statlibobj::TGMSStatusStream::~TGMSStatusStream()
{
   StatusErrorFree();
   delete Ffstat;
}

void statlibobj::TGMSStatusStream::registerWriteCallback( gdlib::stattypes::tgwrite *fptr, void *usermem )
{
   Fgfcb = fptr;
   Fgfusrmem = usermem;
}

void statlibobj::TGMSStatusStream::StatusErrorFree()
{
   trcerrrec delmsg;
   for( trcerrrec nextmsg = Ffrsteqptr; nextmsg; nextmsg = nextmsg->tnxtrc, delete delmsg )
   {
      delmsg = nextmsg;
      trcerrtyp deltyp;
      for( trcerrtyp nexttyp = nextmsg->tfrsttyp; nexttyp; nexttyp = nexttyp->tnxttyp, delete deltyp )
      {
         deltyp = nexttyp;
      }
   }

   trcerrtxt delstr;
   for( trcerrtxt nextstr = Ffrsterrtxt; nextstr; nextstr = nextstr->tnxttxt, delete delstr )
   {
      delstr = nextstr;
   }

   Ffrsteqptr = nullptr;
   Ffrsterrtxt = nullptr;
}

void statlibobj::TGMSStatusStream::StatusDumpFirst()
{
   Feditsymbol = '=';
   Fdumpcount = 0;
   Fcopysysout = false;

   std::string msg;
   if( !checkfile( msg ) )
   {
      Fmsgbuf.front() = msg;
      Fmsgbufcount = 1;
      return;
   }

   if( !opentextmsg( Ffstat, Ffnstat, forRead, msg ) )
   {
      Fmsgbuf[1] = msg;
      Fmsgbuf.front() = "filename=\"" + Ffnstat + "\"";
      Fmsgbufcount = 2;
      return;
   }

   Fstatusopen = true;
   Fmsgbufcount = 0;
}

bool statlibobj::TGMSStatusStream::StatusAppend( const std::string &fn, std::string &msg )
{
   if( !Fstatusopen )
   {
      msg = "statusAppend: target file is not open";
      return false;
   }

   std::fstream f { fn, std::ios::app };
   if( !f.good() )
   {
      msg = "statusAppend: " + SysErrorMessage( f.good() ? 0 : 1 );
      return false;
   }

   // process =x stuff
   // this will not always work, because the editsymbol maybe wrong
   // *************************************************************
   while( !f.eof() )
   {
      std::string line = utils::getLineWithSep( f );
      if( utils::sameText( line.substr( 5, 14 ), "SOLVER DID NOT" ) )
      {
         msg = "statusAppend: nothing as written to " + fn;
         f.close();
         return false;
      }
      if( !utils::starts_with( line, "=0"s ) ) StatusWriteLn( line );
   }
   StatusWriteLn( " " );
   f.close();
   msg.clear();
   return true;
}

void statlibobj::TGMSStatusStream::StatusProcessFirst()
{
   std::string msg;

   Feditsymbol = '=';
   Fprocesscount = 0;
   Fwascopy = false;

   if( !checkfile( msg ) )
   {
      Fmsgbuf.front() = msg;
      Fmsgbufcount = 1;
      return;
   }

   if( !opentextmsg( Ffstat, Ffnstat, forRead, msg ) )
   {
      Fmsgbuf[1] = msg;
      Fmsgbuf.front() = "filename=\"" + Ffnstat + "\"";
      Fmsgbufcount = 2;
      return;
   }

   Fstatusopen = true;
   Fmsgbufcount = 0;
   Fcopysysout = false;
}

bool statlibobj::TGMSStatusStream::StatusDumpNext( std::string &msg )
{
   if( Fmsgbufcount > 0 )
   {
      msg = Fmsgbuf[Fmsgbufcount];
      Fdumpcount++;
      Fmsgbufcount++;
      return true;
   }

   if( !Fstatusopen )
   {
      msg.clear();
      return false;
   }

   if( Fcopysysout )
   {
      assert(Ffsysout);
      if( Ffsysout->eof() )
      {
         msg = "*** End of SysOut copy";
         Fcopysysout = false;
         Ffsysout->close();
      }
      else
         msg = utils::getLineWithSep( *Ffsysout );
      Fdumpcount++;
      return true;
   }

   if( Ffstat->eof() )
   {
      Ffstat->close();
      Fstatusopen = false;
      msg.clear();
      return false;
   }

   msg = utils::getLineWithSep( *Ffstat );
   if( msg.length() >= 3 && msg.front() == Feditsymbol )
   {
      switch( msg[1] )
      {
         case 'B':
         {
            std::string fnsys = utils::trim( msg.substr( 2 ) ), s;
            if( !opentextmsg( Ffsysout, fnsys, gmsgen::forRead, s ) )
            {
               Fmsgbuf.front() = "Msg=" + s;
               Fmsgbuf[1] = "Could not process above statement";
               Fmsgbufcount = 2;
            }
            else
            {
               Fmsgbuf.front() = "*** Begin of SysOut copy";
               Fmsgbufcount = 0;
               Fcopysysout = true;
            }
         }
         break;
         case '9':
            Feditsymbol = msg[2];
            break;
         default:
            break;
      }
   }

   Fdumpcount++;
   return true;
}

bool statlibobj::TGMSStatusStream::StatusProcessNext( gdlib::stattypes::tstatusproc &statusproc, std::string &msg1, std::string &msg2, int &num )
{
   int VarNum, EquNum, p;
   std::string line, s;

   if( Fmsgbufcount > 0 )
   {
      statusproc = stattypes::statusLine;
      msg1 = Fmsgbuf[Fmsgbufcount];
      msg2.clear();
      num = 0;
      Fprocesscount++;
      Fmsgbufcount--;
      return true;
   }

   if( !Fstatusopen )
   {
      statusproc = stattypes::statusLine;
      msg1 = "Must be a bad call";
      msg2.clear();
      num = 0;
      return false;
   }

   if( Fcopysysout )
   {
      statusproc = stattypes::statusLine;
      if( !Ffsysout->eof() )
      {
         msg1 = utils::getLineWithSep( *Ffsysout );
         return true;
      }
      else
      {
         Fcopysysout = false;
         Ffsysout->close();
      }
   }

   while( !Ffstat->eof() )
   {
      line = utils::getLineWithSep( *Ffstat );
      msg1.clear();
      msg2.clear();
      num = 0;
      //res = true;
      if( line.length() < 2 || line.front() != Feditsymbol )
      {
         if( Fwascopy )
         {
            statusproc = stattypes::statusLine;
            msg1 = line;
            return true;
         }
      }
      else
      {
         switch( line[1] )
         {
            case '0':
               statusproc = stattypes::statusSignature;
               msg1 = utils::trim( line.substr( 2 ) );
               return true;

            case '1':
               Fwascopy = true;
               break;

            case '2':
               Fwascopy = false;
               break;

            case '3':
               statusproc = stattypes::statusEOF;
               msg1 = "End-of-File request";
               Ffstat->close();
               Fstatusopen = false;
               return false;

            case '4':
               statusproc = stattypes::statusDump;
               msg1 = "Sysout request";
               return true;

            case '5':
               VarNum = 0;
               p = 3;// to decode the rest of the string
               EquNum = strutilx::StrAsInt( strutilx::ExtractToken( line, p ) );
               if( EquNum < 1 || EquNum > Fmodelrows )
               {
                  msg1 = "Equ out of range:" + line;
                  statusproc = stattypes::statusLine;
                  return true;
               }
               SolverErrorAdd( EquNum, VarNum, utils::trim( line.substr( p - 1 ) ) );
               break;

            case '6':
               EquNum = 0;
               p = 3;// to decode the rest of the string
               VarNum = strutilx::StrAsInt( strutilx::ExtractToken( line, p ) );
               if( VarNum < 1 || VarNum > Fmodelcolumns )
               {
                  msg1 = "Var out of range:" + line;
                  statusproc = stattypes::statusLine;
                  return true;
               }
               SolverErrorAdd( EquNum, VarNum, utils::trim( line.substr( p - 1 ) ) );
               break;

            case '7':
               p = 3;
               EquNum = strutilx::StrAsInt( strutilx::ExtractToken( line, p ) );
               VarNum = strutilx::StrAsInt( strutilx::ExtractToken( line, p ) );
               if( VarNum < 1 || VarNum > Fmodelcolumns || EquNum < 1 || EquNum > Fmodelrows )
               {
                  msg1 = "Equ/Var out of range:" + line;
                  statusproc = stattypes::statusLine;
                  return true;
               }
               SolverErrorAdd( EquNum, VarNum, utils::trim( line.substr( p - 1 ) ) );
               break;

            case '8':
               statusproc = stattypes::statusEject;
               msg1 = "eject request";
               return true;

            case '9':
               Feditsymbol = line[2];
               break;

            case 'A':
               statusproc = stattypes::statusLine;
               s = utils::trim( line.substr( 2 ) );
               if( !opentextmsg( Ffsysout, s, forRead, msg2 ) )
               {
                  msg1 = line;
                  Fmsgbuf.front() = "*** Msg=" + msg2;
                  Fmsgbuf[1] = "*** Above File could not be opened";
                  Fmsgbufcount = 2;
               }
               else if( !Ffsysout->eof() )
               {
                  msg1 = utils::getLineWithSep( *Ffsysout );
                  Fcopysysout = true;
               }
               else
               {
                  msg1 = line;
                  Fmsgbuf.front() = "*** above file is empty";
                  Fmsgbufcount = 1;
                  Ffsysout->close();
               }
               return true;

            case 'B':
               // ignore
               break;

            case 'C':
               statusproc = stattypes::statusLine;
               msg1 = line.substr( 2 );
               return true;

            case 'L':
               statusproc = stattypes::statusAnchor;
               msg1 = line.substr( 2 );
               return true;

            case 'E':
            case 'V':
               p = 3;
               msg1 = strutilx::ExtractToken( line, p );
               num = strutilx::StrAsInt( strutilx::ExtractToken( line, p ) );
               msg2 = strutilx::ExtractToken( line, p );
               if( line[1] == 'E' )
               {
                  if( num < 1 || num > Fmodelrows )
                  {
                     msg1 = "Equ out of range:" + line;
                     statusproc = stattypes::statusLine;
                  }
                  else
                     statusproc = stattypes::statusEqu;
               }
               else if( num < 1 || num > Fmodelcolumns )
               {
                  msg1 = "Var out of range:" + line;
                  statusproc = stattypes::statusLine;
               }
               else
                  statusproc = stattypes::statusVar;
               return true;

            case 'T':
               msg1 = utils::trim( line.substr( 1 ) );
               statusproc = msg1 == "TerminationRequestfromSolver" ? stattypes::statusTerminate : stattypes::statusLine;
               return true;

            default:
               statusproc = stattypes::statusLine;
               msg1 = "unknown status file edit character: " + line;
               return true;
         }
      }
   }

   statusproc = stattypes::statusEOF;
   msg1 = "End-of-File reached";
   msg2.clear();
   num = 0;
   Ffstat->close();
   Fstatusopen = false;
   return false;
}

void statlibobj::TGMSStatusStream::StatusSetFilename( const std::string &fn )
{
   if( Fstatusopen )
   {
      Ffstat->close();
      Fstatusopen = false;
      StatusErrorFree();
   }

   StatusErrorFree();

   SolverErrorInit();
   Ffnstat = fn;
   Fnofilename = false;
}

bool statlibobj::TGMSStatusStream::StatusDummy( std::string &msg )
{
   if( !checkfile( msg ) || !opentextmsg( Ffstat, Ffnstat, forWrite, msg ) ) return false;
   writeln_gf( "= 1" );
   writeln_gf( "" );
   writeln_gf( "**** SOLVER DID NOT WRITE A STATUS FILE ****" );
   writeln_gf( "" );
   writeln_gf( "=3" );
   Ffstat->close();
   Fstatusopen = false;
   msg.clear();
   return true;
}

void statlibobj::TGMSStatusStream::StatusErrorFirst()
{
   Fnextheader = Ffrsteqptr;
   Fnexterror = nullptr;
}

bool statlibobj::TGMSStatusStream::StatusErrorNext( int &row, int &col )
{
   // there is always a maxint record
   if( !Fnextheader || Fnextheader->seqno == std::numeric_limits<int>::max() )
   {
      row = 0;
      col = 0;
      Fnexterror = nullptr;
      return false;
   }
   row = Fnextheader->seqno;
   col = Fnextheader->colnum;
   Fnexterror = Fnextheader->tfrsttyp;
   Fnextheader = Fnextheader->tnxtrc;
   return true;
}

bool statlibobj::TGMSStatusStream::StatusErrorNextEqu( int &row )
{
   row = 0;
   Fnexterror = nullptr;
   while( true )
   {
      if( !Fnextheader || Fnextheader->seqno == std::numeric_limits<int>::max() ) return false;
      row = Fnextheader->seqno;
      int col = Fnextheader->colnum;
      if( !row || col ) Fnextheader = Fnextheader->tnxtrc;
      else
      {
         Fnexterror = Fnextheader->tfrsttyp;
         Fnextheader = Fnextheader->tnxtrc;
         return true;
      }
   }
   return false;
}

bool statlibobj::TGMSStatusStream::StatusErrorNextVar( int &col )
{
   // there is always a maxint record
   col = 0;
   Fnexterror = nullptr;
   while( true )
   {
      if( !Fnextheader || Fnextheader->seqno == std::numeric_limits<int>::max() ) return false;
      int row = Fnextheader->seqno;
      col = Fnextheader->colnum;
      if( !col || row ) Fnextheader = Fnextheader->tnxtrc;
      else
      {
         Fnexterror = Fnextheader->tfrsttyp;
         Fnextheader = Fnextheader->tnxtrc;
         return true;
      }
   }
   return false;
}

bool statlibobj::TGMSStatusStream::StatusErrorNextJac( int &row, int &col )
{
   row = col = 0;
   Fnexterror = nullptr;
   while( true )
   {
      if( !Fnextheader || Fnextheader->seqno == std::numeric_limits<int>::max() ) return false;
      row = Fnextheader->seqno;
      col = Fnextheader->colnum;
      if( !col || !row ) Fnextheader = Fnextheader->tnxtrc;
      else
      {
         Fnexterror = Fnextheader->tfrsttyp;
         Fnextheader = Fnextheader->tnxtrc;
         return true;
      }
   }
   return false;
}

bool statlibobj::TGMSStatusStream::StatusErrorDetail( int &cnt, std::string &msg )
{
   if( !Fnexterror )
   {
      cnt = 0;
      msg.clear();
      return false;
   }
   cnt = Fnexterror->errcnt;
   msg = Fnexterror->txtindx->errtxt;
   Fnexterror = Fnexterror->tnxttyp;
   return true;
}

void statlibobj::TGMSStatusStream::StatusSetRowCol( int rowmax, int colmax )
{
   Fmodelrows = rowmax;
   Fmodelcolumns = colmax;
}

void statlibobj::TGMSStatusStream::StatusClose()
{
   if( Fstatusopen )
   {
      Ffstat->close();
      Fstatusopen = false;
   }
}

bool statlibobj::TGMSStatusStream::StatusFileOpen( gdlib::gmsgen::tfileaction AAction, std::string &msg )
{
   if( checkfile(msg) )
      msg.clear();
   else return false;
   Ffstat = new std::fstream{Ffnstat, std::ios::in};
   bool newstat;
   if(Ffstat->is_open()) {
      std::string s;
      if(!Ffstat->eof()) {
         std::getline(*Ffstat, s);
         if(!Ffstat->eof()) {
            std::getline(*Ffstat, s);
            if(!Ffstat->eof())
               std::getline(*Ffstat, s);
         }
      }
      // stars,' SOLVER DID NOT WRITE A STATUS FILE ', stars
      //  012345
      // '**** SOLVER DID NOT WRITE A STATUS FILE'
      newstat = s.substr(5, 14) == "SOLVER DID NOT"s;
      Ffstat->close();
   } else newstat = true;
   delete Ffstat;

   // FIXME: Finish porting!

   if(!newstat && AAction == forAppend) {
   }
   return false;

   // ...
   STUBWARN();
}

void statlibobj::TGMSStatusStream::StatusWriteLn( const std::string &s )
{
   try
   {
      writeln_gf( s );
   }
   catch( ... )
   {
      staterror( "StatusWriteLn "s + s );
   }
}

void statlibobj::TGMSStatusStream::StatusWrite( const std::string &p )
{
   if( p.empty() ) return;
   write_gf( p );
}

void statlibobj::TGMSStatusStream::StatusWritePlain( const std::string &s )
{
   *Ffstat << s;
}

void statlibobj::TGMSStatusStream::StatusTerminationRequestfromSolver()
{
   commonStatusFunc( "TerminationRequestfromSolver", "StatusTerminationRequestfromSolver" )( "" );
}

void statlibobj::TGMSStatusStream::StatusCopyOn()
{
   commonStatusFunc( "1", "StatusCopyOn" )( "" );
}

void statlibobj::TGMSStatusStream::StatusCopyOff()
{
   commonStatusFunc( "2", "StatusCopyOff" )( "" );
}

void statlibobj::TGMSStatusStream::StatusLSTAnchor( const std::string &s )
{
   commonStatusFunc( "L", "StatusLSTAnchor" )( s );
}

void statlibobj::TGMSStatusStream::StatusCopyLine( const std::string &s )
{
   commonStatusFunc( "C", "StatusCopyLine" )( s );
}

void statlibobj::TGMSStatusStream::StatusCopyFile( const std::string &s )
{
   commonStatusFunc( "A", "StatusCopyFile" )( s );
}

void statlibobj::TGMSStatusStream::StatusCopyFileOnSysOut( const std::string &s )
{
   commonStatusFunc( "B", "StatusCopyFileOnSysOut" )( s );
}

void statlibobj::TGMSStatusStream::StatusEndOfFile()
{
   commonStatusFunc( "3", "StatusEndOfFile" )( "" );
}

void statlibobj::TGMSStatusStream::StatusSysOut()
{
   commonStatusFunc( "4", "StatusSysOut" )( "" );
}

void statlibobj::TGMSStatusStream::StatusPageEject()
{
   commonStatusFunc( "8", "StatusPageEject" )( "" );
}

void statlibobj::TGMSStatusStream::StatusSetEditSymbol( char c )
{
   try
   {
      writeln_gf( ""s + Feditsymbol + "9"s + c );
      Feditsymbol = c;
   }
   catch( ... )
   {
      staterror( "StatusSetEditSymbol" );
   }
}

void statlibobj::TGMSStatusStream::StatusAuditLine( const std::string &s )
{
   try
   {
      writeln_gf( ""s + Feditsymbol + "0"s + s );
   }
   catch( std::exception &e )
   {
      staterror( "StatusAuditLine"s + s );
      staterror( "StatusAuditLine "s + e.what() );
   }
}

void statlibobj::TGMSStatusStream::StatusEquationName( const std::string &s1, int num, std::string &s2 )
{
   try
   {
      writeln_gf( ""s + Feditsymbol + "E \""s + s1 + "\" "s + std::to_string( num ) + " \""s + s2 + "\""s );
   }
   catch( ... )
   {
      staterror( "StatusEquationName "s + s1 );
   }
}

void statlibobj::TGMSStatusStream::StatusVariableName( const std::string &s1, int num, const std::string &s2 )
{
   try
   {
      writeln_gf( ""s + Feditsymbol + "V \""s + s1 + "\" "s + std::to_string( num ) + " \""s + s2 + "\""s );
   }
   catch( ... )
   {
      staterror( "StatusVariableName "s + s1 );
   }
}

void statlibobj::TGMSStatusStream::StatusEquationError( int num, const std::string &s )
{
   commonStatusFunc( "5 "s + std::to_string( num ) + " "s, "StatusEquationError"s )( s );
}

void statlibobj::TGMSStatusStream::StatusVariableError( int num, const std::string &s )
{
   commonStatusFunc( "6 " + std::to_string( num ) + " "s, "StatusVariableError" )( s );
}

void statlibobj::TGMSStatusStream::StatusJacobianError( int num1, int num2, const std::string &s )
{
   try
   {
      writeln_gf( ""s + Feditsymbol + "7 " + std::to_string( num1 ) + " " + std::to_string( num2 ) + " " + s );
   }
   catch( std::exception &e )
   {
      staterror( "StatusJacobianError "s + e.what() );
   }
}

std::function<void( const std::string & )> TGMSStatusStream::commonStatusFunc( const std::string &s1, const std::string &s2 )
{
   return [&]( const std::string &s3 ) {
      try
      {
         writeln_gf( ""s + Feditsymbol + s1 );
      }
      catch( ... )
      {
         staterror( s2 + s3 );
      }
   };
}

bool opentextmsg( global::delphitypes::Text f, const std::string &fn, gdlib::gmsgen::tfileaction fa, std::string &msg )
{
   f = new std::fstream();
   switch( fa )
   {
      case forRead:
         f->open( fn, std::ios_base::in );
         break;
      case forWrite:
         f->open( fn, std::ios_base::out );
         break;
      case forAppend:
         f->open( fn, std::ios_base::app );
         break;
   }
   unsigned int ioRes { f->good() ? 0 : static_cast<unsigned int>( f->rdstate() ) };
   msg = !ioRes ? ""s : SysErrorMessage( ioRes );
   return !ioRes;
}

static void initialization()
{
   bool isntUnix = rtl::p3platform::OSFileType() != rtl::p3platform::tOSFileType::OSFileUNIX;
   termString = isntUnix ? "\r\n"s : "\n"s;
}

static void finalization()
{
}

UNIT_INIT_FINI();
}// namespace gdlib::statlibobj