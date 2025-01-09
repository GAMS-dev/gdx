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

#include "../rtl/sysutils_p3.hpp"
#include "../rtl/p3platform.hpp"
#include "../rtl/p3utils.hpp"
#include "../rtl/idglobal_p3.hpp"

#include "utils.hpp"

#include "strutilx.hpp"
#include "statlibobj.hpp"
#include "gmsgen.hpp"
#include "global/unit.h"
#include "rtl/p3process.hpp"

using namespace gdlib::gmsgen;
using namespace rtl::sysutils_p3;
using namespace std::literals::string_literals;


namespace gdlib::statlibobj
{
bool isMuxed()
{
   return false;
}

static std::string termString;

static bool tmElapsed( uint32_t &thenT, uint32_t minDiffT )
{
   const auto nowT = rtl::idglobal_p3::GetTickCount();
   if( const auto diffT = rtl::idglobal_p3::GetTickDiff( thenT, nowT ); diffT > minDiffT )
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

bool opentextmsg( FILE *&f, const std::string &fn, tfileaction fa, std::string &msg );

TGMSLogStream::TGMSLogStream( std::string &Msg ) : FLastShowTicks { rtl::idglobal_p3::GetTickCount() }
{
   Msg.clear();
}

void TGMSLogStream::registerWriteCallback( stattypes::tgwrite *fptr, void *usermem )
{
   Fgfcb = fptr;
   Fgfusrmem = usermem;
}

void TGMSLogStream::LogClose()
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
         if( FStatus == sl_file || FSaveAstat == 1 )
         {
            std::fclose( Ffcon );
            Ffcon = nullptr;
         }
      }
      FStatus = sl_closed;
   }
}

bool TGMSLogStream::LogOpen( int Astat, const tfileaction AAction, const std::string &Afn )
{
   if( FStatus != sl_closed )
   {
      if( FLogEnabled )
      {
         Flush();
         if( FStatus == sl_file )
            fclose( Ffcon );
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

   int ioRes{};
   bool res{};

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
         //Ffcon = &debugStream;
         Ffcon = stdout;
         ioRes = std::ferror(stdout) ? 1 : 0;
         if( ioRes ) statuslerror( "LogOpen-2: cannot write to standard output"s );
         FStatus = sl_output;
         break;

      case 2:// file
      case 4:// file+stdout
      {
         FLogEnabled = true;
         FRedirFileName = Afn;
         int IDECMDSrun = 0;
         while(IDECMDSrun <= 1)
         {
            IDECMDSrun += 2;
            const bool appendMode { AAction == forAppend };
            #ifndef _WIN32
            Ffcon = std::fopen( Afn.c_str(), appendMode ? "a" : "w" );
            #else
            Ffcon = std::fopen( Afn.c_str(), appendMode ? "ab" : "wb" );
            #endif
            if( !Ffcon )
            {
               if( appendMode )
               {
                  #ifdef _WIN32
                  Ffcon = std::fopen( Afn.c_str(), "wb" );
                  #else
                  Ffcon = std::fopen( Afn.c_str(), "w" );
                  #endif
               }
               ioRes = !Ffcon ? 1 : 0;
            }
            else
               ioRes = 0;

            // Try to close file in IDE
            if( ioRes && rtl::p3platform::OSFileType() == rtl::p3platform::tOSFileType::OSFileWIN && IDECMDSrun <= 2 )
            {
               int IDECMDSsc;
               if( const int IDECMDSrc = rtl::p3process::P3ExecP( "IDECmds View Close \""s + Afn + "\""s, IDECMDSsc ) )
                  debugStream << "*** ExecRC="s << IntToStr(IDECMDSrc) << " Could not start ViewClose request on "s << Afn << '\n';
               else if(IDECMDSsc)
                  debugStream << "*** IDECmdsRC="s << IntToStr(IDECMDSrc) << " Viewclose request failed on "s << Afn << '\n';
               else
               {
                  debugStream << "*** Issued ViewClose request on "s << Afn << '\n';
                  utils::sleep(500);
                  IDECMDSrun--;
               }
            }
         }
         if(!ioRes) FStatus = sl_file;
         else
         {
            FStatus = sl_closed;
            CheckOpen(); // switch to standard output
            FSaveAstat = 3;
            FSaveAfn.clear();
            FRedirFileName.clear();
            write_gf( "*** "s + (AAction == forAppend ? "Append"s : "Open"s) );
            writeln_gf( " logfile error: (R="s + IntToStr( ioRes ) + ") FN=\""s + Afn + '\"' );
            writeln_gf("*** Msg="s + SysErrorMessage( ioRes ));
            writeln_gf( "*** Logging to standard output"s );
            res = true;
         }
      }
         break;

      default:
         break;
   }

   if( resetLo1to3 )
      writeln_gf( "--- LogOption value 1 is deprecated, reset to 3"s );

   FLenLast = 0;
   return res;
}

void TGMSLogStream::LogReOpen()
{
   LogOpen( FSaveAstat, forAppend, FSaveAfn );
}

void TGMSLogStream::LogMessage( const std::string &s )
{
   if( !FLogEnabled ) return;
   Flush();
   CheckOpen();
   write_gf( s );
   std::fflush(Ffcon);
   FLenLast = static_cast<int>(s.length());
   FLastIsMsg = true;
}

void TGMSLogStream::LogFileName( const std::string &fn, const int Lev )
{
   if( !FLogEnabled ) return;
   if( FhasNewData && FTraceLevel > 0 ) ShowStatLine();
   FFullFileName = fn;
   FFileName = strutilx::ExtractFileNameEx( fn );
   const int OldLevel = FNestLevel;
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

void TGMSLogStream::LogLineNr( const int N )
{
   if( FLogEnabled && N != FLineNr )
   {
      FLineNr = N;
      FhasNewData = true;
      FLineStartTicks = rtl::idglobal_p3::GetTickCount();
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

void TGMSLogStream::LogAnchor( const int N )
{
   if( FLogEnabled )
   {
      const std::string s = "[LST:"s + IntToStr( N ) + "]"s;
      CheckOpen();
      write_gf( s );
      FLenLast += static_cast<int>(s.length());
   }
}

void TGMSLogStream::LogTitleAnchor( const std::string &Msg )
{
   if( FLogEnabled )
   {
      gstatStartWriting();
      writeln_gf( "[TIT:" + Msg + "]" );
      std::fflush(Ffcon);
   }
}

void TGMSLogStream::LogFileAnchor( const bool err, const std::string &fn, const int line, const int col )
{
   if( !FLogEnabled ) return;
   doFileAnchor( err, fn, line, col );
}

void TGMSLogStream::LogMemory( const double M )
{
   if( FLogEnabled && std::abs( M - FMemory ) > 0.5 )
   {
      FMemory = M;
      FhasNewData = true;
      CndShowStatLine();
   }
}

void TGMSLogStream::LogErrrorCnt( const int N )
{

   if( FLogEnabled && N != FErrorCnt )
   {
      FErrorCnt = N;
      FhasNewData = true;
      CndShowStatLine();
   }
}

void TGMSLogStream::freshen()
{
   if( FLogEnabled ) CndShowStatLine();
}

void TGMSLogStream::freshenEx()
{
   if( FLogEnabled && FStatus == sl_output && FTraceLevel > 1 )
   {
      const auto nowT = rtl::idglobal_p3::GetTickCount();
      const auto dt = rtl::idglobal_p3::GetTickDiff( FLineStartTicks, nowT );
      FLastShowTicks = nowT;
      if( const auto secs = dt / 1000; secs > 2 && secs > FPrevSecs && ( secs <= 30 || secs - 5 >= FPrevSecs ) )
      {
         FPrevSecs = secs;
         FhasNewData = true;
      }
      if( FhasNewData )
      {
         ShowStatLine();
         std::fflush(Ffcon);
      }
   }
}

void TGMSLogStream::LogDumpFilename( const std::string &prfx, const bool enabled, const std::string &what, const std::string &gs, const tfileaction fa, const int ioResOrNeg )
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

   s += "="s + IntToStr( ioResOrNeg ) + " FN=\""s + gs + "\""s;
   LogMessage( s );
   if( ioResOrNeg > 0 )
      LogMessage( prfx + "MSG=" + SysErrorMessage( ioResOrNeg ) );
}

void TGMSLogStream::LogWriteLn( const std::string &s )
{
   if( FLogEnabled )
   {
      if( !Fgfcb )
      {
         gstatStartWriting();
         writeln_gf( s );
         std::fflush(Ffcon);
      }
      else
         writeln_gf( s );
   }
}

void TGMSLogStream::LogWrite( const std::string &p )
{
   constexpr int blockSize = 255;
   if( p.empty() || !FLogEnabled ) return;
   if( !Fgfcb ) gstatStartWriting();
   const int numBlocks = static_cast<int>( std::ceil( static_cast<double>( p.length() ) / static_cast<double>( blockSize ) ) );
   for( int i = 0; i <= numBlocks; i++ )
   {
      std::string s { p.substr( i * blockSize, blockSize ) };
      write_gf( s );
   }
   if( !Fgfcb ) std::fflush(Ffcon);;
}

void TGMSLogStream::LogWritePlain( const std::string &s ) const
{
   if( !FLogEnabled ) return;
   std::fwrite( s.c_str(), sizeof( char ), s.size(), Ffcon );
   if( FSaveAstat == 4 )
      debugStream << s;
   std::fflush( Ffcon );
}

void TGMSLogStream::showCounts()
{
#ifdef METER
   LogMessage( "DEBUG: TGMSLogStream usage counts:" );
   LogMessage( "  method LogLineNr: " + IntToStr( cntLogLineNr ) );
   LogMessage( "  method LogMemory: " + IntToStr( cntLogMemory ) );
   LogMessage( "  method   freshen: " + IntToStr( cntFreshen ) );
   LogMessage( "" );
#endif
}

void TGMSLogStream::setIDErun( const bool v )
{
   FIDE = v;
}

bool TGMSLogStream::getLogEnabled() const
{
   return FLogEnabled;
}

void TGMSLogStream::setLogEnabled( const bool v )
{
   FLogEnabled = v;
}

int TGMSLogStream::getTraceLevel() const
{
   return FTraceLevel;
}

void TGMSLogStream::setTraceLevel( const int v )
{
   FTraceLevel = v;
}

std::string TGMSLogStream::getRedirFilename() const
{
   return FRedirFileName.empty() ? ""s : "\"" + FRedirFileName + "\"";
}

std::string TGMSLogStream::getRedirString() const
{
   if( FRedirFileName.empty() ) return ""s;
   const std::string pf = FSaveAstat == 4 ? " | tee -a " : " >> ";
   return pf + getRedirFilename();
}

std::string TGMSLogStream::getShortRedirString( const std::string &Dir ) const
{
   if( FRedirFileName.empty() ) return ""s;
   const auto k { Dir.find( FRedirFileName ) };
   return k == std::string::npos ? getRedirString() : " >> \"." + FRedirFileName.substr( Dir.length() ) + "\"";
}

void TGMSLogStream::SetOSMemory( const int v )
{
   FShowOSMem = v;
}

void TGMSLogStream::CheckOpen()
{
   if( FStatus == sl_closed )
   {
      Ffcon = stdout;//&debugStream;// new std::fstream("", std::ios_base::out);
      //debugStream << "*** Log reopened as output\n";
      const auto msg {"*** Log reopened as output\n"};
      std::fwrite( msg, sizeof( char ), 27, Ffcon );
      FStatus = sl_output;
   }
}

void TGMSLogStream::Flush()
{
   if( FhasNewData && FTraceLevel > 0 ) ShowStatLine();
   if( FLenLast > 0 && FStatus != sl_closed )
   {
      writeln_gf( "" );
      FLenLast = 0;
      FErrorCnt = 0;
      std::fflush(Ffcon);
   }
   FLastIsMsg = false;
}

void TGMSLogStream::doFileAnchor( const bool err, const std::string &fn, const int line, const int col )
{
   CheckOpen();
   const std::string s { ( err ? "[ERR:"s : "[FIL:"s ) + "\""s + fn + "\","s + IntToStr( line ) + ","s + IntToStr( col ) + "]"s };
   write_gf( s );
   FLenLast += static_cast<int>(s.length());
}

void TGMSLogStream::CndShowStatLine()
{
   if( FStatus == sl_output && FTraceLevel > 1 )
   {
      if( !tmElapsed( FLastShowTicks, refreshInterval ) ) return;
      ShowStatLine();
      std::fflush(Ffcon);;
   }
}

void TGMSLogStream::ShowStatLine()
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
      static std::string s;
      s.reserve( 256 );
      s = "--- "s;
      if( FNestLevel > 0 )
         s += std::string( FNestLevel, '.' ) + " "s;
      s += FFileName + "("s + IntToStr( FLineNr ) + ")"s;
      uint64_t vss;
      if( uint64_t rss; FShowOSMem == 1 && rtl::p3utils::p3GetMemoryInfo( rss, vss ) )
         FMemory = static_cast<double>( rss ) / 1e6;
      else if( FShowOSMem == 2 && rtl::p3utils::p3GetMemoryInfo( rss, vss ) )
         FMemory = static_cast<double>( vss ) / 1e6;
      s += " "s + IntToStr( utils::round<int>( FMemory ) ) + " Mb"s;
      if( FErrorCnt > 0 )
      {
         s += " "s + IntToStr( FErrorCnt ) + " "s;
         s += "Error"s + ( FErrorCnt > 1 ? "s"s : ""s );
      }
      if( FSpinChar != ' ' ) s += " "s + FSpinChar;
      if( FPrevSecs > 0 ) s += " " + IntToStr( FPrevSecs ) + " secs";
      if( FLenLast > 0 ) write_gf( "\r" );
      if( static_cast<int>( s.length() ) < FLenLast )
      {
         write_gf( std::string( FLenLast, ' ' ) + "\r" );
      }
      write_gf( s );
      std::fflush( Ffcon );
      FLenLast = static_cast<int>( s.length() );
      if( FIDE ) doFileAnchor( false, FFullFileName, FLineNr, 0 );
   }
   FhasNewData = false;
}

void TGMSLogStream::gstatStartWriting()
{
   CheckOpen();
   Flush();
}

void TGMSLogStream::writeln_gf( const std::string &msg ) const
{
   write_gf( msg + termString);
}


void TGMSLogStream::write_gf( const std::string &msg ) const
{
   if( Fgfcb ) ( *Fgfcb )( msg, stattypes::doLog, Fgfusrmem );
   else
   {
      std::fwrite( msg.c_str(), sizeof( char ), msg.size(), Ffcon );
      if( FSaveAstat == 4 ) debugStream << msg;
   }
}

void TGMSLogStream::writeln_gf(const char* msg ) const
{
   static std::array<char, 256> ss;
   const auto l { std::strlen( msg ) };
   std::memcpy( ss.data(), msg, sizeof( char ) * l );
   std::memcpy( &ss[l], termString.data(), sizeof( char ) * termString.size() );
   write_gf( ss.data() );
}

void TGMSLogStream::write_gf(const char* msg ) const
{
   if( Fgfcb ) ( *Fgfcb )( msg, stattypes::doLog, Fgfusrmem );
   else
   {
      fwrite( msg, sizeof( char ), std::strlen(msg), Ffcon );
      if( FSaveAstat == 4 ) debugStream << msg;
   }
}

void TGMSStatusStream::writeln_gf( const std::string &msg ) const
{
   write_gf( msg + termString );
}

void TGMSStatusStream::write_gf( const std::string &msg ) const
{
   if( Fgfcb ) ( *Fgfcb )( msg, gdlib::stattypes::doStat, Fgfusrmem );
   else fwrite( msg.c_str(), sizeof( char ), msg.size(), Ffstat );
}

void TGMSStatusStream::SolverErrorInit()
{
   Ffrsteqptr = new rcerrrec {
           std::numeric_limits<int>::max(),
           std::numeric_limits<int>::max(),
   };
   Ffrsterrtxt = nullptr;
}

trcerrtxt TGMSStatusStream::SolverErrorMessage( const std::string &s )
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

void TGMSStatusStream::SolverErrorAdd( int rownum, int columnnum, const std::string &msg )
{
   const trcerrrec ehdrptr { Ffrsteqptr };
   trcerrrec nextrc { ehdrptr };
   bool firsttime { true };
   const trcerrtxt newerr { SolverErrorMessage( msg ) };

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
      const auto newtyp = new rcerrtyp {
              1,
              nullptr,
              newerr };
      const auto newrc = new rcerrrec {
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
         const auto newtyp = new rcerrtyp {
                 1,
                 nextrc->tfrsttyp,
                 newerr };
         nextrc->tfrsttyp = newtyp;
      }
   }
}

bool TGMSStatusStream::checkfile( std::string &msg )
{
   if( Fstatusopen )
   {
      fclose(Ffstat);
      Ffstat = nullptr;
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

TGMSStatusStream::TGMSStatusStream( std::string &Msg ) : Ffstat { std::fopen( ".", "r" ) },
                                                                     Ffnstat { "unknown file name"s },
                                                                     Fnofilename { true },
                                                                     Feditsymbol { '=' },
                                                                     Fmodelrows { std::numeric_limits<int>::max() },
                                                                     Fmodelcolumns { std::numeric_limits<int>::max() }
{
   Msg.clear();
}

TGMSStatusStream::~TGMSStatusStream()
{
   StatusErrorFree();
   if(Ffstat) {
      std::fclose(Ffstat);
      Ffstat = nullptr;
   }
}

void TGMSStatusStream::registerWriteCallback( stattypes::tgwrite *fptr, void *usermem )
{
   Fgfcb = fptr;
   Fgfusrmem = usermem;
}

void TGMSStatusStream::StatusErrorFree()
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

void TGMSStatusStream::StatusDumpFirst()
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

bool TGMSStatusStream::StatusAppend( const std::string &fn, std::string &msg ) const
{
   if( !Fstatusopen )
   {
      msg = "statusAppend: target file is not open";
      return false;
   }

   // shouldn't these be binary mode on windows?
   FILE *f { fopen( fn.c_str(), "a" ) };
   if( !f )
   {
      msg = "statusAppend: " + SysErrorMessage( errno );
      return false;
   }

   // process =x stuff
   // this will not always work, because the editsymbol maybe wrong
   // *************************************************************
   while( !feof(f) )
   {
      std::string line = utils::getline( f );
      if( utils::sameText( line.substr( 5, 14 ), "SOLVER DID NOT" ) )
      {
         msg = "statusAppend: nothing as written to " + fn;
         fclose(f);
         return false;
      }
      if( !utils::starts_with( line, "=0"s ) ) StatusWriteLn( line );
   }
   StatusWriteLn( " " );
   fclose(f);
   msg.clear();
   return true;
}

void TGMSStatusStream::StatusProcessFirst()
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

bool TGMSStatusStream::StatusDumpNext( std::string &msg )
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
      if( feof(Ffsysout) )
      {
         msg = "*** End of SysOut copy";
         Fcopysysout = false;
         std::fclose(Ffsysout);
      }
      else
         msg = utils::getline( Ffsysout );
      Fdumpcount++;
      return true;
   }

   if( feof(Ffstat) )
   {
      fclose(Ffstat);
      Fstatusopen = false;
      msg.clear();
      return false;
   }

   msg = utils::getline( Ffstat );
   if( msg.length() >= 3 && msg.front() == Feditsymbol )
   {
      switch( msg[1] )
      {
         case 'B':
         {
            const std::string fnsys = utils::trim( msg.substr( 2 ) );
            if( std::string s; !opentextmsg( Ffsysout, fnsys, forRead, s ) )
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

bool TGMSStatusStream::StatusProcessNext( stattypes::tstatusproc &statusproc, std::string &msg1, std::string &msg2, int &num )
{
   int VarNum, EquNum, p;
   std::string s;

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
      assert(Ffsysout);
      if( !feof( Ffsysout ) )
      {
         msg1 = utils::getline( Ffsysout );
         return true;
      }
      Fcopysysout = false;
      std::fclose( Ffsysout );
   }

   while( !feof(Ffstat) )
   {
      std::string line = utils::getline( Ffstat );
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
               fclose( Ffstat );
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
               else if( !feof(Ffsysout) )
               {
                  msg1 = utils::getline( Ffsysout );
                  Fcopysysout = true;
               }
               else
               {
                  msg1 = line;
                  Fmsgbuf.front() = "*** above file is empty";
                  Fmsgbufcount = 1;
                  std::fclose(Ffsysout);
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
   fclose( Ffstat );
   Fstatusopen = false;
   return false;
}

void TGMSStatusStream::StatusSetFilename( const std::string &fn )
{
   if( Fstatusopen )
   {
      fclose( Ffstat );
      Fstatusopen = false;
      StatusErrorFree();
   }

   StatusErrorFree();

   SolverErrorInit();
   Ffnstat = fn;
   Fnofilename = false;
}

bool TGMSStatusStream::StatusDummy( std::string &msg )
{
   if( !checkfile( msg ) || !opentextmsg( Ffstat, Ffnstat, forWrite, msg ) ) return false;
   writeln_gf( "= 1" );
   writeln_gf( "" );
   writeln_gf( "**** SOLVER DID NOT WRITE A STATUS FILE ****" );
   writeln_gf( "" );
   writeln_gf( "=3" );
   fclose( Ffstat );
   Fstatusopen = false;
   msg.clear();
   return true;
}

void TGMSStatusStream::StatusErrorFirst()
{
   Fnextheader = Ffrsteqptr;
   Fnexterror = nullptr;
}

bool TGMSStatusStream::StatusErrorNext( int &row, int &col )
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

bool TGMSStatusStream::StatusErrorNextEqu( int &row )
{
   // there is always a maxint record
   row = 0;
   Fnexterror = nullptr;
   while( true )
   {
      if( !Fnextheader || Fnextheader->seqno == std::numeric_limits<int>::max() )
         return false;
      row = Fnextheader->seqno;
      if( const int col = Fnextheader->colnum; !row || col ) Fnextheader = Fnextheader->tnxtrc;
      else
      {
         Fnexterror = Fnextheader->tfrsttyp;
         Fnextheader = Fnextheader->tnxtrc;
         return true;
      }
   }
}

bool TGMSStatusStream::StatusErrorNextVar( int &col )
{
   // there is always a maxint record
   col = 0;
   Fnexterror = nullptr;
   while( true )
   {
      if( !Fnextheader || Fnextheader->seqno == std::numeric_limits<int>::max() ) return false;
      const int row = Fnextheader->seqno;
      col = Fnextheader->colnum;
      if( !col || row ) Fnextheader = Fnextheader->tnxtrc;
      else
      {
         Fnexterror = Fnextheader->tfrsttyp;
         Fnextheader = Fnextheader->tnxtrc;
         return true;
      }
   }
}

bool TGMSStatusStream::StatusErrorNextJac( int &row, int &col )
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
}

bool TGMSStatusStream::StatusErrorDetail( int &cnt, std::string &msg )
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

void TGMSStatusStream::StatusSetRowCol( int rowmax, int colmax )
{
   Fmodelrows = rowmax;
   Fmodelcolumns = colmax;
}

void TGMSStatusStream::StatusClose()
{
   if( Fstatusopen )
   {
      fclose( Ffstat );
      Fstatusopen = false;
   }
}

bool TGMSStatusStream::StatusFileOpen( const tfileaction AAction, std::string &msg )
{
   if( checkfile(msg) )
      msg.clear();
   else return false;
   // shouldn't this be binary mode on windows?
   Ffstat = fopen( Ffnstat.c_str(), "r" );
   bool newstat;
   if(Ffstat) {
      std::string s;
      if(!feof(Ffstat)) {
         utils::getline(Ffstat, s);
         if(!feof(Ffstat)) {
            utils::getline(Ffstat, s);
            if(!feof(Ffstat))
               utils::getline(Ffstat, s);
         }
      }
      // stars,' SOLVER DID NOT WRITE A STATUS FILE ', stars
      //  012345
      // '**** SOLVER DID NOT WRITE A STATUS FILE'
      newstat = s.substr(5, 14) == "SOLVER DID NOT"s;
      fclose( Ffstat );
   } else newstat = true;
   //delete Ffstat;

   // FIXME: Finish porting!

   if(!newstat && AAction == forAppend) {
   }
   return false;

   // ...
   STUBWARN();
   // TODO: Implement me!
}

void TGMSStatusStream::StatusWriteLn( const std::string &s ) const
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

void TGMSStatusStream::StatusWrite( const std::string &p ) const
{
   if( p.empty() ) return;
   write_gf( p );
}

void TGMSStatusStream::StatusWritePlain( const std::string &s ) const
{
   fwrite( s.c_str(), sizeof( char ), s.size(), Ffstat );
}

void TGMSStatusStream::StatusTerminationRequestfromSolver() const
{
   commonStatusFunc( "TerminationRequestfromSolver", "StatusTerminationRequestfromSolver" )( "" );
}

void TGMSStatusStream::StatusCopyOn() const
{
   commonStatusFunc( "1", "StatusCopyOn" )( "" );
}

void TGMSStatusStream::StatusCopyOff() const
{
   commonStatusFunc( "2", "StatusCopyOff" )( "" );
}

void TGMSStatusStream::StatusLSTAnchor( const std::string &s ) const
{
   commonStatusFunc( "L", "StatusLSTAnchor" )( s );
}

void TGMSStatusStream::StatusCopyLine( const std::string &s ) const
{
   commonStatusFunc( "C", "StatusCopyLine" )( s );
}

void TGMSStatusStream::StatusCopyFile( const std::string &s ) const
{
   commonStatusFunc( "A", "StatusCopyFile" )( s );
}

void TGMSStatusStream::StatusCopyFileOnSysOut( const std::string &s ) const
{
   commonStatusFunc( "B", "StatusCopyFileOnSysOut" )( s );
}

void TGMSStatusStream::StatusEndOfFile() const
{
   commonStatusFunc( "3", "StatusEndOfFile" )( "" );
}

void TGMSStatusStream::StatusSysOut() const
{
   commonStatusFunc( "4", "StatusSysOut" )( "" );
}

void TGMSStatusStream::StatusPageEject() const
{
   commonStatusFunc( "8", "StatusPageEject" )( "" );
}

void TGMSStatusStream::StatusSetEditSymbol( char c )
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

void TGMSStatusStream::StatusAuditLine( const std::string &s ) const
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

void TGMSStatusStream::StatusEquationName( const std::string &s1, const int num, const std::string &s2 ) const
{
   try
   {
      writeln_gf( ""s + Feditsymbol + "E \""s + s1 + "\" "s + IntToStr( num ) + " \""s + s2 + "\""s );
   }
   catch( ... )
   {
      staterror( "StatusEquationName "s + s1 );
   }
}

void TGMSStatusStream::StatusVariableName( const std::string &s1, const int num, const std::string &s2 ) const
{
   try
   {
      writeln_gf( ""s + Feditsymbol + "V \""s + s1 + "\" "s + IntToStr( num ) + " \""s + s2 + "\""s );
   }
   catch( ... )
   {
      staterror( "StatusVariableName "s + s1 );
   }
}

void TGMSStatusStream::StatusEquationError( const int num, const std::string &s ) const
{
   commonStatusFunc( "5 "s + IntToStr( num ) + " "s, "StatusEquationError"s )( s );
}

void TGMSStatusStream::StatusVariableError( const int num, const std::string &s ) const
{
   commonStatusFunc( "6 " + IntToStr( num ) + " "s, "StatusVariableError" )( s );
}

void TGMSStatusStream::StatusJacobianError( const int num1, const int num2, const std::string &s ) const
{
   try
   {
      writeln_gf( ""s + Feditsymbol + "7 "s + IntToStr( num1 ) + ' ' + IntToStr( num2 ) + ' ' + s );
   }
   catch( std::exception &e )
   {
      staterror( "StatusJacobianError "s + e.what() );
   }
}

std::function<void( const std::string & )> TGMSStatusStream::commonStatusFunc( const std::string &s1, const std::string &s2 ) const
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

bool opentextmsg( FILE *&f, const std::string &fn, const tfileaction fa, std::string &msg )
{
   const char *mode {};
   // shouldn't these be binary mode on Windows to avoid weird \r\n behavior?
   switch( fa )
   {
      case forRead:
         mode = "r";
         break;
      case forWrite:
         mode = "w";
         break;
      case forAppend:
         mode = "a";
         break;
      default:
         throw std::runtime_error( "Unknown file action provided!"s );
   }
   f = fopen( fn.c_str(), mode );
   const int ioRes { f ? 0 : errno };
   msg = !ioRes ? ""s : SysErrorMessage( ioRes );
   return !ioRes;
}

static void initialization()
{
   const bool isntUnix = rtl::p3platform::OSFileType() != rtl::p3platform::tOSFileType::OSFileUNIX;
   termString = isntUnix ? "\r\n"s : "\n"s;
}

static void finalization()
{
}

UNIT_INIT_FINI();
}// namespace gdlib::statlibobj