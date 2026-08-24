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

#include <stdexcept>
#include <iostream>
#include <filesystem>
#include <cassert>
#include <cstring>
#include <fstream>
#include "p3process.hpp"
#include "p3platform.hpp"
#include "../gdlib/utils.hpp"
#include "sysutils_p3.hpp"

#if __cplusplus >= 202002L
#include <format>
#endif

#if defined( _WIN32 ) || defined( _WIN64 )
#include <windows.h>
#include <tlhelp32.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <csignal>
#include <fcntl.h>

#if defined( __APPLE__ )
#include <sys/sysctl.h>
#endif
#endif

using namespace std::literals::string_literals;

using namespace rtl::p3platform;
using namespace rtl::sysutils_p3;

namespace GDX_NS rtl::p3process
{

constexpr int debug {0};

class SocketInfo
{
public:
   int n;               // current size of storage
   int nProc{};         // count of (logical) processor stanzas
   int iSock{-1};       // socket number of current processor
   int numSockets{};    // socket count
   int numCores{-1};    // core count
   int uniform{};       // are the sockets uniformly-sized?
   int sibs{}, cores{}; // if sockets are uniform

   std::vector<int> cpuCores, siblings, procCount;

   explicit SocketInfo( int n_ ) : n { n_ }, cpuCores( n_, -1 ), siblings( n_, -1 ), procCount( n_, 0 )
   {
      if( debug )
         debugStream << "  SocketInfo: core size = " << cpuCores.size()
                   << "  n = " << n << "  cpuCores[n-1] " << cpuCores[n - 1]
                   << '\n';
   }
   // start of a new processor stanza
   void nextProc( int k )
   {
      if( k < 0 )
         throw std::runtime_error( "processor ID must be positive" );
      nProc++;
      iSock = -1;
   }// nextProc
   // physical ID = socket index: should come early in processor stanza
   void setPhysicalID( int k )
   {
      if( iSock >= 0 )
         throw std::runtime_error( "physical ID already set for this processor" );
      if( k < 0 )
         throw std::runtime_error( "physical ID must be positive" );
      if( k >= n )
      {// must grow
         if( debug )
            debugStream << "Growing: n = " << n << "  physical ID = " << k << '\n';
         n = k + 1;
         cpuCores.resize( n, -1 );
         siblings.resize( n, -1 );
         procCount.resize( n, 0 );
      }
      iSock = k;
      if( 0 == procCount[iSock] )
         numSockets++;
      procCount[iSock]++;
   }// setPhysicalID
   // setCores: how many cores on the socket containing this processor?
   void setCores( int k )
   {
      if( debug )
         debugStream << "  setCores(" << k << ") with iSock = " << iSock << '\n';
      if( iSock < 0 )
         throw std::runtime_error( "physical ID must be set prior to cpu cores" );
      if( k < 0 )
         throw std::runtime_error( "cpu cores must be positive" );
      // std::cout << "    cpuCores curr is " << cpuCores[iSock] << '\n';
      if( cpuCores[iSock] < 0 )
      {// first "cpu cores" line
         cpuCores[iSock] = k;
      }
      else if( cpuCores[iSock] != k )
         throw std::runtime_error( "conflicting cpu cores values detected" );
   }// setCores
   void setSiblings( int k )
   {
      if( debug )
         debugStream << "  setSiblings(" << k << ") with iSock = " << iSock << '\n';
      if( iSock < 0 )
         throw std::runtime_error( "physical ID must be set prior to cpu cores" );
      if( k < 0 )
         throw std::runtime_error( "siblings must be positive" );
      // std::cout << "    siblings curr is " << siblings[iSock] << '\n';
      if( siblings[iSock] < 0 )
      {// first "siblings" line
         siblings[iSock] = k;
      }
      else if( siblings[iSock] != k )
         throw std::runtime_error( "conflicting siblings values detected" );
   }// siblings
   // verify: when all the data is in, check it out and compute some totals
   void verify()
   {
      int s = 0;// socket count
      int p = 0;// processor count

      uniform = 1;
      sibs = cores = 0;
      numCores = 0;
      for( int i = 0; i < n; i++ )
      {
         if( procCount[i] <= 0 )
         {
            if( ( -1 != siblings[i] ) || ( -1 != cpuCores[i] ) )
               throw std::runtime_error( "incomplete/inconsistent cpuinfo detected" );
         }
         else
         {
            if( ( siblings[i] <= 0 ) || ( cpuCores[i] <= 0 ) )
               throw std::runtime_error( "incomplete/inconsistent cpuinfo detected" );
            if( procCount[i] != siblings[i] )
               throw std::runtime_error( "incomplete/inconsistent cpuinfo detected" );
            s++;
            p += procCount[i];
            numCores += cpuCores[i];
            if( 0 == sibs )
            {
               sibs = siblings[i];
               cores = cpuCores[i];
            }
            else
               uniform = uniform && ( sibs == siblings[i] ) && ( cores == cpuCores[i] );
         }
      }// loop over sockets
      if( p != nProc )
         throw std::runtime_error( "incomplete/inconsistent cpuinfo detected" );
      if( ( s != numSockets ) || ( s <= 0 ) )
         throw std::runtime_error( "incomplete/inconsistent cpuinfo detected" );
   }// verify
};  // end SocketInfo

static int getCPUInfo( int *nSockets, int *nCores, int *nThreads,
                       int *coresPerSocket, int *threadsPerCore );

//========================================================================================================

std::string TExecArgList::Get( const int Index ) const
{
   return FList[Index];
}

std::string TExecArgList::GetLast() const
{
   return FList.back();
}

int TExecArgList::Count() const
{
   return static_cast<int>( FList.size() );
}

std::string TExecArgList::operator[]( const int Index ) const
{
   return Get( Index );
}

void TExecArgList::Put( const int Index, const std::string &Item )
{
   FList[Index] = Item;
}

int TExecArgList::GetCapacity() const
{
   return static_cast<int>( FList.capacity() );
}

static const char* GetParamShortStr(const char* p, std::string& param)
{
   static utils::sstring buf;
   int len {};
   char *r { buf.data() };

   auto pushChar = [&]( const char c ) {
      if (len < (int)buf.size() - 1)
      {
         *r = c;
         ++len;
         ++r;
      }
   };

   while (true)
   {
      while( *p != '\0' && *p <= ' ' )
         p++;
      const char *s = p;
      ++s;
      if( *p == '\"' && *s == '\"' )
         p += 2;
      else
         break;
   }

   while (*p > ' ')
   {
      if (*p == '\"')
      {
         ++p;
         while (*p != '\0' && *p != '\"')
         {
            pushChar( *p );
            ++p;
         }
         if( *p != '\0' )
            ++p;
      }
      else
      {
         pushChar( *p );
         ++p;
      }
   }
   buf[len] = '\0';
   param = buf.data();
   return p;
}

int TExecArgList::Split( const bool append, const std::string &s )
{
   std::string param;
   int count {};
   const char *p = s.c_str();
   while (true)
   {
      p = GetParamShortStr( p, param );
      if( param.empty() )
         break;
      if( append )
         Add( param );
      else
         Insert( count, param );
      count++;
   }
   return count;
}

TExecArgList::TExecArgList() = default;
TExecArgList::~TExecArgList() = default;

void TExecArgList::Clear()
{
   FList.clear();
}
void TExecArgList::Delete( const int Index )
{
   FList.erase( FList.begin() + Index );
}

void TExecArgList::Insert( const int Index, const std::string &Item )
{
   FList.insert( FList.begin() + Index, Item );
}

int TExecArgList::SplitAppend( const std::string &s )
{
   return Split( true, s );
}

int TExecArgList::SplitPrepend( const std::string &s )
{
   return Split( false, s );
}

int TExecArgList::Add( const std::string &Item )
{
   FList.emplace_back( Item );
   return static_cast<int>( FList.size() - 1 );
}

//========================================================================================================

// nSockets: count of physical sockets, aka packages
// nCores:   count of cores, aka physical CPUs
// nThreads: count of threads, aka logical CPUs or vCPUs.
//           with Intel's Hyperthreading, each core can run 2 threads
// if the values above are detected OK,
//   return TRUE
// else
//   return FALSE
// If we return true and the number of cores per socket looks good, return it,
// o/w return coresPerSocket=-1
// If we return true and the number of threads per core looks good, return it,
// o/w return ThreadsPerCore=-1
bool p3GetCPUInfo( int &nSockets, int &nCores, int &nThreads, int &coresPerSocket, int &threadsPerCore )
{
   return !getCPUInfo( &nSockets, &nCores, &nThreads, &coresPerSocket, &threadsPerCore );
}

/* on success, return 0: this implies nSockets, nCores, nThreads are all good */
/* coresPerSocket and threadsPerCore are -1 if there is any doubt about them */
static int getCPUInfo( int *nSockets, int *nCores, int *nThreads,
                       int *coresPerSocket, int *threadsPerCore )
{
#if defined( _WIN32 )
   DWORD rc, len, pos;
   PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX proc;

   try
   {
      len = 0;
      LOGICAL_PROCESSOR_RELATIONSHIP relType = RelationAll;
      rc = GetLogicalProcessorInformationEx( relType, nullptr, &len );
      if( rc || ( ERROR_INSUFFICIENT_BUFFER != GetLastError() ) )
         return -1;// failure
      std::unique_ptr<unsigned char[]> uPtr( new unsigned char[len] );
      unsigned char *buf = uPtr.get();
      proc = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) buf;
      rc = GetLogicalProcessorInformationEx( relType, proc, &len );
      if( debug )
         debugStream << "GLPIEx returns " << rc << '\n';
      if( !rc )
         return -1;// failure
      *nSockets = 0;
      *nThreads = 0;
      *nCores = 0;
      for( pos = 0; pos < len; )
      {
         proc = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) ( buf + pos );
         switch( proc->Relationship )
         {
            case RelationProcessorCore:
               ++*nCores;
               if( 1 != proc->Processor.GroupCount )
                  return -1;// should always be 1
               {
                  KAFFINITY mask = proc->Processor.GroupMask[0].Mask;
                  while( 0 != mask )
                  {
                     *nThreads += mask & 1;
                     mask >>= 1;
                  }
               }
               if( debug )
               {
                  debugStream << "  Found processor core relationship:" << '\n';
                  BYTE flags = proc->Processor.Flags;
                  debugStream << "    SMT: "
                            << ( ( 0 == flags ) ? "no" : ( LTP_PC_SMT == flags ) ? "yes"
                                                                                 : "unknown" )
                            << '\n';
               }
               break;
            case RelationProcessorPackage:
               ++*nSockets;
               if( debug )
               {
                  debugStream << "  Found processor package relationship:" << '\n';
                  debugStream << "    GroupCount: " << proc->Processor.GroupCount << '\n';
               }
               break;
            default:
               break;
         }// switch
         pos += proc->Size;
      }// for loop over structures
   }   // try
   catch( ... )
   {
      return -1;
   }

   *coresPerSocket = *nCores / *nSockets;
   *threadsPerCore = *nThreads / *nCores;
   if( ( ( *coresPerSocket * *nSockets ) != *nCores ) || ( ( *threadsPerCore * *nCores ) != *nThreads ) )
   {
      *coresPerSocket = -1;
      *threadsPerCore = -1;
   }
   return 0;
#elif defined( __linux__ )
   SocketInfo sInfo( 4 );
   try
   {
      std::ifstream cpuinfo( "/proc/cpuinfo"s );
      std::string s, f;
      size_t found;
      int k;
      while( cpuinfo.good() )
      {
         (void) std::getline( cpuinfo, s );
         if( debug >= 3 )
            debugStream << "line s : " << s << '\n';
         if( 0 == s.find( "processor"s ) )
         {
            found = s.find( ':' );
            f = s.substr( found + 1 );
            k = std::stoi( f );
            if( debug )
               debugStream << " GOT processor : " << f << "  " << k << '\n';
            sInfo.nextProc( k );
            if( k < 0 )
               return -1;
         }
         else if( 0 == s.find( "physical id"s ) )
         {// socket ID
            found = s.find( ':' );
            f = s.substr( found + 1 );
            k = std::stoi( f );
            if( debug )
               debugStream << " GOT physical id : " << f << "  " << k << '\n';
            sInfo.setPhysicalID( k );
         }
         else if( 0 == s.find( "siblings"s ) )
         {// # of siblings on this socket
            found = s.find( ':' );
            f = s.substr( found + 1 );
            k = std::stoi( f );
            if( debug )
               debugStream << " GOT siblings : " << f << "  " << k << '\n';
            sInfo.setSiblings( k );
         }
         else if( 0 == s.find( "cpu cores"s ) )
         {// # of cpu cores on this socket
            found = s.find( ':' );
            f = s.substr( found + 1 );
            k = std::stoi( f );
            if( debug )
               debugStream << " GOT cpu cores : " << f << "  " << k << '\n';
            sInfo.setCores( k );
         }
         else
            continue;
      }
      if( debug )
      {
         debugStream << "read of /proc/cpuinfo finished: nProc = " << sInfo.nProc << '\n';
      }
      sInfo.verify();
      *nSockets = sInfo.numSockets;
      *nCores = sInfo.numCores;
      *nThreads = sInfo.nProc;
   }
   catch( const std::string &msg )
   {
      if( debug )
         debugStream << "Exception caught: msg = " << msg << '\n';
      return -1;
   }
   catch( ... )
   {
      return -1;
   }

   *coresPerSocket = *nCores / *nSockets;
   *threadsPerCore = *nThreads / *nCores;
   if( ( !sInfo.uniform ) || ( ( *coresPerSocket * *nSockets ) != *nCores ) || ( ( *threadsPerCore * *nCores ) != *nThreads ) )
   {
      *coresPerSocket = -1;
      *threadsPerCore = -1;
   }
   return 0;
#elif defined( __APPLE__ )
   int rc, k;
   size_t sz = sizeof( k );

   rc = sysctlbyname( "hw.packages", &k, &sz, NULL, 0 );
   if( rc || ( k <= 0 ) )
      return -1;
   *nSockets = k;
   rc = sysctlbyname( "hw.physicalcpu", &k, &sz, NULL, 0 );
   if( rc || ( k <= 0 ) )
      return -1;
   *nCores = k;
   rc = sysctlbyname( "hw.logicalcpu", &k, &sz, NULL, 0 );
   if( rc || ( k <= 0 ) )
      return -1;
   *nThreads = k;

   *coresPerSocket = *nCores / *nSockets;
   *threadsPerCore = *nThreads / *nCores;
   if( ( ( *coresPerSocket * *nSockets ) != *nCores ) || ( ( *threadsPerCore * *nCores ) != *nThreads ) )
   {
      *coresPerSocket = -1;
      *threadsPerCore = -1;
   }
   return 0;
#else
   *nSockets = *nCores = *nThreads = *coresPerSocket = *threadsPerCore = 1;
   throw std::runtime_error( "Unknown platform for getCPUInfo!" );
#endif
}

#if defined( _WIN32 )
/* on success, return TRUE */
BOOL winProcInfo( int *coreCount, int *logicalCount )
{
   DWORD rc, len, pos, group;
   PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX buf, proc;
   unsigned char *ucPtr;
   SYSTEM_INFO siSysInfo;

   *coreCount = -1;
   *logicalCount = -1;

   len = 0;
   rc = GetLogicalProcessorInformationEx( RelationProcessorCore, nullptr, &len );
   if( rc || ( ERROR_INSUFFICIENT_BUFFER != GetLastError() ) )
      return FALSE; /* failure */
   buf = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) malloc( len );
   if( !buf )
      return FALSE; /* failure */
   rc = GetLogicalProcessorInformationEx( RelationProcessorCore, buf, &len );
   if( !rc )
   {
      free( buf );
      return FALSE; /* failure */
   }
   *coreCount = 0;
   *logicalCount = 0;
   ucPtr = (unsigned char *) buf;
   for( pos = 0; pos < len; )
   {
      proc = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX) ( ucPtr + pos );
      if( RelationProcessorCore == proc->Relationship )
      {
         ++*coreCount;
         for( group = 0; group < proc->Processor.GroupCount; group++ )
         {
            KAFFINITY mask = proc->Processor.GroupMask[group].Mask;
            while( 0 != mask )
            {
               *logicalCount += mask & 1;
               mask >>= 1;
            }
         }
      }
      pos += proc->Size;
   }
   free( buf );
   /* only if logicalCount <= 64 can we
		 * verify what we computed is consistent with numberOfProcessors */
   if( *logicalCount <= 64 )
   {
      GetSystemInfo( &siSysInfo );
      if( *logicalCount != (int) siSysInfo.dwNumberOfProcessors )
      {
         *coreCount = -1;
         *logicalCount = -1;
         return FALSE; /* failure */
      }
   }

   return TRUE; /* success */
} /* winProcInfo */
#endif

int p3GetNumberOfProcessors()
{
#if defined( _WIN32 )
   int coreCount, logicalCount;
   SYSTEM_INFO siSysInfo;
   if( winProcInfo( &coreCount, &logicalCount ) )
      return logicalCount;
   /* my assumption: better to use GetSystemInfo than return -1, */
   /*                even if GetSystemInfo may undercount        */
   GetSystemInfo( &siSysInfo );
   return (int) siSysInfo.dwNumberOfProcessors;
#elif defined( __linux__ ) || defined( __APPLE__ )
   return (int) sysconf( _SC_NPROCESSORS_ONLN );
#else
   throw std::runtime_error( "Unknown platform for getNumberOfProcessors!" );
#endif
}

constexpr auto CMD_WIN7 = R"(C:\windows\system32\cmd.exe)";
constexpr auto CMD_WINNT = R"(C:\winnt\system32\cmd.exe)";

#if defined(_WIN32)

#if __cplusplus >= 202002L
constexpr auto CMD_WIN7_W = LR"(C:\windows\system32\cmd.exe)";
constexpr auto CMD_WINNT_W = LR"(C:\winnt\system32\cmd.exe)";
#endif

static int wShowWindow {SW_SHOWNA};
#endif

static int CppCreateProc( const std::string &exeName, const std::string &cmdLine, bool allowStdHandles, int &exeRC )
{
#if defined(_WIN32)
   PROCESS_INFORMATION ProcessInformation;
   ZeroMemory( &ProcessInformation, sizeof( PROCESS_INFORMATION ) );
   // Initialise the startup information to be the same as that of the
   // calling application.  This is easier than initialising the many
   // individual startup information fields and should be fine in most
   // cases.
   STARTUPINFOA StartupInfo;
   GetStartupInfoA( &StartupInfo );
   StartupInfo.cbReserved2 = 0;
   StartupInfo.lpReserved = nullptr;
   StartupInfo.lpReserved2 = nullptr;

   // StartupInfo.wShowWindow determines whether the called application
   // will be initially displayed normal, maximises, minimised or some
   // other subtle variations
   StartupInfo.wShowWindow = wShowWindow;

   // call with STARTF_USESTDHANDLES set only if inheritHandles is TRUE
   if( !allowStdHandles )
      StartupInfo.dwFlags &= ~STARTF_USESTDHANDLES;

   std::vector<char> cmdLineBuf( cmdLine.length() + 1 );
   std::memcpy( cmdLineBuf.data(), cmdLine.c_str(), cmdLine.length()+1 );

   if( !CreateProcessA( exeName.empty() ? nullptr : exeName.c_str(), cmdLineBuf.data(), nullptr, nullptr, true, 0, nullptr, nullptr, &StartupInfo, &ProcessInformation ) )
   {
      exeRC = 0;
      return static_cast<int>(GetLastError());
   }
   WaitForSingleObject( ProcessInformation.hProcess, INFINITE );
   DWORD exitcode;
   BOOL brc = GetExitCodeProcess( ProcessInformation.hProcess, &exitcode );
   CloseHandle( ProcessInformation.hThread );
   CloseHandle( ProcessInformation.hProcess );
   if (!brc || exitcode == 255) // failed call to GetExitCodeProcess
   {
      exeRC = 0;
      return 1;
   }
   exeRC = static_cast<int>( exitcode );
   return 0;
#else
   exeRC = !exeName.empty() + !cmdLine.empty();
   return 1;
#endif
}

static int System4Win( const std::string &CmdPtr, bool inheritedHandles, int &ProgRC )
{
   // check if we have a command starting with "" and ending with " (and more than 2 chars)
   static auto checkAddQuotes = []( const std::string &cp ) {
      return cp.length() <= 2 || cp[0] != '\"' || cp[1] != '\"' || cp.back() != '\"';
   };

   std::string cs = QueryEnvironmentVariable( "COMSPEC" );
   if( cs.empty() )
   {
      if( FileExists( CMD_WIN7 ) )
         cs = CMD_WIN7;
      else if( FileExists( CMD_WINNT ) )
         cs = CMD_WINNT;
      else
         return 1;
   }
   std::string arg = checkAddQuotes( CmdPtr ) ? cs + " /C \""s + CmdPtr + '\"' : cs + " /C "s + CmdPtr;
   return CppCreateProc( cs, arg, inheritedHandles, ProgRC );
}

#ifdef __IN_CPPMEX__
#if defined(_WIN32)
static int CppCreateProc( const std::wstring &exeName, const std::wstring &cmdLine,
                         bool allowStdHandles, int &exeRC )
{
   PROCESS_INFORMATION ProcessInformation;
   ZeroMemory( &ProcessInformation, sizeof( PROCESS_INFORMATION ) );
   // Initialise the startup information to be the same as that of the
   // calling application.  This is easier than initialising the many
   // individual startup information fields and should be fine in most
   // cases.
   STARTUPINFOW StartupInfo;
   GetStartupInfoW( &StartupInfo );
   StartupInfo.cbReserved2 = 0;
   StartupInfo.lpReserved = nullptr;
   StartupInfo.lpReserved2 = nullptr;

   // StartupInfo.wShowWindow determines whether the called application
   // will be initially displayed normal, maximises, minimised or some
   // other subtle variations
   StartupInfo.wShowWindow = wShowWindow;

   // call with STARTF_USESTDHANDLES set only if inheritHandles is TRUE
   if( !allowStdHandles )
      StartupInfo.dwFlags &= ~STARTF_USESTDHANDLES;

   std::vector<wchar_t> cmdLineBuf( cmdLine.length() + 1 );
   std::memcpy( cmdLineBuf.data(), cmdLine.c_str(), (cmdLine.length()+1)*sizeof(wchar_t) );

   if( !CreateProcessW(
      exeName.empty() ? nullptr : exeName.c_str(),
      cmdLineBuf.data(),
      nullptr,
      nullptr,
      true,
      0,
      nullptr,
      nullptr,
      &StartupInfo,
      &ProcessInformation
   ) )
   {
      exeRC = 0;
      return static_cast<int>(GetLastError());
   }

   WaitForSingleObject( ProcessInformation.hProcess, INFINITE );
   DWORD exitcode;
   BOOL brc = GetExitCodeProcess( ProcessInformation.hProcess, &exitcode );
   CloseHandle( ProcessInformation.hThread );
   CloseHandle( ProcessInformation.hProcess );
   if (!brc || exitcode == 255) // failed call to GetExitCodeProcess
   {
      exeRC = 0;
      return 1;
   }
   exeRC = static_cast<int>( exitcode );
   return 0;
}

static int System4Win( const std::wstring &cmdPtr, bool inheritedHandles, int &progRC )
{
   // check if we have a command starting with "" and ending with " (and more than 2 chars)
   static auto needQuote = []( const std::wstring &cp ) {
      return cp.length() <= 2 || cp[0] != L'\"' || cp[1] != L'\"' || cp.back() != L'\"';
   };

   std::wstring cs = QueryEnvironmentVariable( L"COMSPEC" );
   if( cs.empty() )
   {
      if( FileExists( CMD_WIN7 ) )
         cs = CMD_WIN7_W;
      else if( FileExists( CMD_WINNT ) )
         cs = CMD_WINNT_W;
      else
         return 1;
   }

   std::wstring q = needQuote(cmdPtr) ? L"\"" : L"";
   std::wstring arg = std::format(L"{} /C {}{}{}", cs, q, cmdPtr, q);
   return CppCreateProc( cs, arg, inheritedHandles, progRC );
}
#endif

#endif

static int System4Unix( const std::string &CmdPtr, int &ProgRC )
{
#if defined( _WIN32 ) || defined( _WIN64 )
   return 1;
#else
   const std::string newPtr = CmdPtr.empty() ? "sh"s : CmdPtr;
   int rCode = system( newPtr.c_str() );
   if( WIFEXITED( rCode ) )
   {// success!
      ProgRC = WEXITSTATUS( rCode );
      if( ProgRC == 127 )
      {// cmd was not run (e.g. file not found)
         ProgRC = 0;
         return 127;
      }
      else if( ProgRC == 126 )
      {// cmd was not run (e.g. permission denied)
         ProgRC = 0;
         return 126;
      }
      return 0;
   }
   else if( WIFSIGNALED( rCode ) )
   {// child stopped via signal
      ProgRC = WTERMSIG( rCode );
      return 1;
   }
   else
   {// shell failed
      ProgRC = 0;
      return 2;
   }
#endif
}

int P3SystemP( const std::string &CmdPtr, int &ProgRC )
{
   int res {};
   switch( OSFileType() )
   {
      case OSFileWIN:
         res = System4Win( CmdPtr, true, ProgRC );
         break;
      case OSFileUNIX:
         res = System4Unix( CmdPtr, ProgRC );
         break;
      default:
         throw std::runtime_error( "Unimplemented P3SysstemP for OSFileType" );
   }
   return res;
}

#if defined(__IN_CPPMEX__)
int P3SystemP( const std::wstring &CmdPtr, int &ProgRC )
{

#ifdef _WIN32
   return System4Win(CmdPtr, true, ProgRC);
#else

   throw std::runtime_error( "P3SystemP with wide cmdline is only usable on windows" );

#endif
}

#define myexit _exit
#else
#define myexit(x) while(false);
#endif

#ifndef _WIN32
static int LibcForkExec( int argc, char *const argv[], int *exeRC )
{
   int result = 1;
   int pid, pid2;
   int wstat;
   pid = fork();
   if( pid < 0 )
   { /* could not fork */
      *exeRC = 0;
      result = 1;
   }
   else if( !pid )
   { /* I am the child */
      if( execvp( argv[0], argv ) == -1 )
         debugStream << "Failure to execute because: " << strerror( errno ) << '\n';
      execl( "/bin/sh", "/bin/sh", "-c", "exit 255", nullptr );
      /* if we are here, it is trouble */
      /* _exit() is a more immediate termination, less likely to flush stdio */
      myexit( 255 ); /* -1 tells parent we could not exec */
   }
   else
   { /* I am the parent */
      for( ;; )
      {
         wstat = 0;
         pid2 = waitpid( pid, &wstat, 0 );
         if( -1 == pid2 )
            continue;
         if( pid != pid2 )
         {
            *exeRC = 0;
            return 1; /* failed wait so fail the entire fork/exec */
         }
         else
            break;
      }
      if( WIFEXITED( wstat ) )
      { /* normal exit from child */
         if( 255 == WEXITSTATUS( wstat ) )
         { /* because it couldn't exec */
            *exeRC = 0;
            result = 1;
         }
         else
         {
            *exeRC = WEXITSTATUS( wstat );
            result = 0;
         }
      }
      else
      { /* abnormal return from child */
         *exeRC = 0;
         result = 1;
      }
   } /* end parent code */
   return result;
} /* LibcForkExec */


/* libcASyncForkExec does a fork/exec to start a process,
	* but it does not wait for it.  Instead it returns the PID.
	* also, it sets up a new process group for the child
	* result: 0 on success, ~0 on failure
	*/
static int libcASyncForkExec( int argc, char *const argv[], int *pid )
{
   int result = 1; /* failure */
   int lPid;       /* local pid, typed appropriately */

   *pid = ~0;
   lPid = fork();
   if( lPid < 0 )
   { /* could not fork */
      result = 1;
   }
   else if( 0 == lPid )
   {                          /* I am the child */
      (void) setpgid( 0, 0 ); /* make this process a new process group */
      if( execvp( argv[0], argv ) == -1 )
         debugStream << "Failure to execute because: " << strerror( errno ) << '\n';

      /* if we are here, it is trouble */
      execl( "/bin/sh", "/bin/sh", "-c", "exit 127", NULL );
      myexit( 127 ); /* consistent with & usage in bash */
   }
   else
   {                             /* I am the parent */
      (void) setpgid( lPid, 0 ); /* make the child its own, new process group */
      /* we call setpgid for both parent and child to avoid a race condition */
      result = 0;
      *pid = (int) lPid;
   }
   return result;
} /* libcASyncForkExec */


/* unixPidStatus returns:
 *   0: valid process but not a zombie
 *   1: zombie process
 *   2: process does not exist
 *   3: not implemented or other error
 */
static int unixPidStatus( int p )
{
   char sbuf[1024]; /* buffer for content of stat file */
   char *tmp, *t2;
   constexpr int fnBufSiz { 80 };
   char filename[fnBufSiz];
   int fd, p2;
   ssize_t numRead;
   char state;
   struct stat sb {}; /* stat() buffer */

   std::snprintf( filename, fnBufSiz, "/proc/%d", p );
   if( -1 == stat( filename, &sb ) )
      return 2;

   std::snprintf( filename, fnBufSiz, "/proc/%d/stat", p );
   fd = open( filename, O_RDONLY, 0 );
   if( -1 == fd )
      return 3;
   numRead = read( fd, sbuf, sizeof( sbuf ) - 1 );
   close( fd );
   if( numRead <= 0 )
      return 3;

   /* start of file looks like
   * 2796 (firefox) S  where we have
   *  pid  cmdline  status */
   sbuf[numRead] = '\0';
   numRead = std::sscanf( sbuf, "%d", &p2 );
   if( 1 != numRead )
      return 3;
   tmp = strchr( sbuf, '(' ) + 1;
   t2 = strrchr( sbuf, ')' );
   if( !tmp || !t2 )
      return 3;
   *t2 = '\0';
   t2 = t2 + 2;// skip ") "
   numRead = std::sscanf( t2, "%c", &state );
   if( 1 != numRead )
      return 3;
   // printf ("DEBUG: in isZombie: read %d (%s) %c\n", p2, tmp, state);
   switch( state )
   {
      case 'D':    /* uninterruptible sleep */
      case 'R':    /* running */
      case 'S':    /* sleeping */
      case 'T':    /* traced or stopped */
         return 0; /* valid, non-zombie process */
      case 'Z': /* zombie */
         return 1;
      default:
         return 3;
   }
}

/* killProcGroupUnix
 * return:  true if the process is(was) running
 *              and our termination attempt succeeded
 *          false o/w
 */
static bool killProcGroupUnix( pid_t p, TKillHow how )
{
   bool result {};
   if( p > 0 )
   { /* PIDs are positive */
      int rc = kill( -p, ( TKillHow::soft == how ) ? SIGINT : SIGKILL );
      result = ( 0 == rc );
      /* clean up the zombie.  If this is our child the PID is still valid
     * until we wait on the process
     * remaining issue: will this wait if this was NOT a child process? */
      if( 0 == rc )
      { /* signal sent successfully */
         for( int i = 0; i < 2; i++ )
         {
            rc = unixPidStatus( p );
            if( rc > 1 ) /* not running */
               return result;
            else if( rc < 1 )
            {                   /* running, not a zombie */
               usleep( 20000 ); /* 20 milliseconds */
                                /* printf ("waiting for zombie\n"); */
            }
            else
            { /* a zombie */
               int wstat;
               (void) waitpid( p, &wstat, 0 );
               return result;
            }
         } /* sleep loop */
      }    /* if signal sent OK */
   }
   return result;
} /* killProcGroupUnix */

static void uglyGmsUnzipHack( std::list<std::string> &args )
{
   // AS: Very ugly hack to circumvent gmsunzip issue in gamslib/dplytest on macOS
   //     Does not like '-d"/some/path"' but it works with '-d "/some/path"'
   //     Solved by finding '-d...' part, removing stuff after -d and putting it as additional item afterwards
   if( utils::sameText( args.front(), "gmsunzip" ) )
   {
      std::list<std::string>::iterator it;
      std::string rest {};
      for( it = std::next( std::begin( args ) ); it != std::end( args ); ++it )
      {
         if( utils::starts_with( *it, "-d"s ) )
         {
            rest = it->substr( 2 );
            *it = "-d";
            ++it;
            break;
         }
      }
      if( !rest.empty() )
      {
         args.insert( it, rest );
      }
   }
}

static int ForkWithSplitArgs( const std::function<int( int, char *const[], int * )> &forkExecFunc, const std::string &s, int &ProgRC )
{
   std::list<std::string> args = utils::splitWithQuotedItems( s );
   uglyGmsUnzipHack( args );
   const auto argc { static_cast<int>( args.size() ) };
   std::vector<char *> argv( argc + 1 );
   std::vector<std::vector<char>> argBufs( argc );
   int i {};
   // remove single and double quotes from result part
   const std::array<char, 2> quoteChars = { '\'', '\"' };
   for( const std::string &arg: args )
   {
      bool quoted = std::any_of( quoteChars.begin(), quoteChars.end(), [&arg]( char qc ) { return arg.front() == qc && arg.back() == qc; } );
      argBufs[i].resize( arg.length() + 1 );
      strcpy( argBufs[i].data(), arg.c_str() + ( quoted ? 1 : 0 ) );
      if( quoted ) argBufs[i][arg.length() - 2] = '\0';
      argv[i] = argBufs[i].data();
      i++;
   }
   // by convention exec expects last argv entry to point to zero
   argv.back() = nullptr;
   return forkExecFunc( argc, argv.data(), &ProgRC );
}
#endif

int P3ExecP( const std::string &CmdPtr, int &ProgRC )
{
   int res { 1 };
   switch( OSFileType() )
   {
      case OSFileWIN:
         res = CppCreateProc( "", CmdPtr, true, ProgRC );
         break;
      case OSFileUNIX:
#ifndef _WIN32
         res = ForkWithSplitArgs( LibcForkExec, CmdPtr, ProgRC );
#endif
         break;
      default:
         assert( 0 && "unimplemented P3ExecP for OSFileType" );
         break;
   }
   return res;
}

// FIXME: this is a bit naive, as a path containing " would need case
// a first improvement is to use ' as a quote in that case.
// TODO: need proper fix
template<typename T = std::string_view>
static auto whatQuote(T arg)
{
   using Char = typename T::value_type;
   Char c[2] = {0};
   if( const T targ { utils::trim( arg ) };
      targ.length() > 1 && targ.front() == '"' && targ.back() == '"')
      c[0] = 0; // already quoted, do not quote it again
   if(arg.empty()) //FIXME: why not targ.empty()?
      c[0] = '"';
   for(int i{}; i<(int)arg.length(); i++)
      if(arg[i] <= ' ') {// found a space, quote the whole thing
         c[0] = '"';
         break;
      }

   return std::basic_string<Char>(c);
}

// NOTE: this is verbatim from P3process.pas, but rewired for the wide character
#if __cplusplus >= 202002L
#ifdef _WIN32
static void getWinErrMsg256 (int errCode, const char prefix[], char buf256[256])
{
  DWORD n;
  size_t k;
  char msgBuf[1024];

  n = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        errCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        msgBuf,
        sizeof(msgBuf),
        NULL
        );
  strcpy (buf256, prefix);
  if (0 == n) {
    strcat (buf256, "error message too long or unavailable");
  }
  else {
    k = 256 - 1 - strlen(prefix);
    strncat (buf256, msgBuf, k);
  }
} /* getWinErrMsg256 */

static int Win32CreateProcEx(const wchar_t *exeName, wchar_t *cmdLine,
                             bool inheritedHandles, int *exeRC, int *errCode, char errMsg[256])
{
   PROCESS_INFORMATION processinformation;
   STARTUPINFOW        startupinfo;
   DWORD               exitcode;
   int                 result;
   BOOL brc;

   *errMsg = '\0';
   *errCode = 0;
   /* Initialise the startup information to be the same as that of the
   * calling application.  This is easier than initialising the many
   * individual startup information fields and should be fine in most
   * cases. */
   GetStartupInfoW(&startupinfo);
   startupinfo.cbReserved2 = 0;
   startupinfo.lpReserved  = NULL;
   startupinfo.lpReserved2 = NULL;

   /* StartupInfo.wShowWindow determines whether the called application
   * will be initially displayed normal, maximises, minimised or some
   * other subtle variations */
   startupinfo.wShowWindow = wShowWindow;

   /* call with STARTF_USESTDHANDLES set only if inheritHandles is TRUE */
   if (!inheritedHandles)
      startupinfo.dwFlags &= ~STARTF_USESTDHANDLES;

   if (! CreateProcessW(
      exeName,               /* ApplicationName */
      cmdLine,               /* lpCommandLine */
      nullptr,               /* lpProcessAttributes */
      nullptr,               /* lpThreadAttribute */
      inheritedHandles,      /* bInheritedHandles */
      0,                     /* dwCreationFlags */
      nullptr,               /* lpEnvironment */
      nullptr,               /* lpCurrentDirectory */
      &startupinfo,          /* lpStartupInfo */
      &processinformation    /* lpProcessInformation */
   )) {

      *exeRC = 0;
      *errCode = GetLastError();  /* failed to execute */
      getWinErrMsg256 (*errCode, "CreateProcess() call failed: ", errMsg);
      result = 1;

   } else {

      WaitForSingleObject(processinformation.hProcess,INFINITE);
      brc = GetExitCodeProcess(processinformation.hProcess,&exitcode);
      CloseHandle(processinformation.hThread);
      CloseHandle(processinformation.hProcess);
      if (255 == exitcode) { /* abnormal child termination */
         *exeRC = 0;
         return 2;
      }
      if (! brc) {        /* failed call to GetExitCodeProcess */
         *exeRC = 0;
         *errCode = GetLastError();  /* failed to execute */
         getWinErrMsg256 (*errCode, "GetExitCodeProcess() call failed: ", errMsg);
         return 3;
      }
      *exeRC = exitcode;
      result = 0;
   }
   return result;
} /* Win32CreateProcEx */
#endif

#ifndef _WIN32
static int LibcForkExecEx( int argc, const char * argv[], int &exeRC, int &errCode,
                          std::string &errMsg )
{
   int result = 1;
   int pid, pid2;
   int wstat;
   errMsg = "";
   errCode = 0;
   pid = fork();
   if( pid < 0 )
   { /* could not fork */
      exeRC = 0;
      errCode = errno;
      errMsg = "fork() called failed: " + SysErrorMessage( errno );
      result = 1;
   }
   else if( !pid ) { /* I am the child */
      if( execvp( argv[0], (char *const *)argv ) == -1 )
         debugStream << "Failure to execute because: " << SysErrorMessage(errno) << '\n';
      execl( "/bin/sh", "/bin/sh", "-c", "exit 255", nullptr );
      /* if we are here, it is trouble */
      /* _exit() is a more immediate termination, less likely to flush stdio */
      myexit( 255 ); /* -1 tells parent we could not exec */
   }
   else
   { /* I am the parent */
      for( ;; )
      {
         wstat = 0;
         pid2 = waitpid( pid, &wstat, 0 );
         if( pid == pid2 ) break;

         if ((-1 == pid2) && (EINTR == errno))
            continue;
         exeRC = 0;
         errCode = errno;
         errMsg = "waitpid() call failed: " + SysErrorMessage(errno);
         return 3;    /* failed waitpid */

      }
      if( WIFEXITED( wstat ) )
      { /* normal exit from child */
         if( 255 == WEXITSTATUS( wstat ) )
         { /* because it couldn't exec */
            exeRC = 0;
            result = 2;
         }
         else
         {
            exeRC = WEXITSTATUS( wstat );
            result = 0;
         }
      }
      else
      { /* abnormal return from child */
         exeRC = 0;
         result = 4;
      }
   } /* end parent code */
   return result;
}/* LibcForkExecEx */
#endif // _WIN32


// LLM code, to be replace with std::views::join_with in C++-23
// This template accepts ANY range (vector, view, list, etc.)
template <std::ranges::input_range R>
std::wstring join_range(R&& range, std::wstring_view delimiter) {
   std::wstring result;
    auto it = std::ranges::begin(range);
    auto end = std::ranges::end(range);

    // If the range is completely empty, return early
    if (it == end) return result;

    // Append the first element
    result += *it;
    ++it;

    // Append the delimiter and the rest of the elements
    for (; it != end; ++it) {
        result += delimiter;
        result += *it;
    }

    return result;
}


/**
 * @brief Start a new process using execve semantics. On windows, serialize to a cmdline
 *
 * @warning On windows, the callee is responsible for quoting the argv args that need it
 *
 * @param argv    the sequence of arguments, MUST be NULL-terminated
 * @param progRC  the return code of the program
 * @param errCode the error code
 * @param errMsg  the error message
 *
 * @return        some undocumented error number
 */
int P3ExecArgv(std::vector<const NativePathChar *> &argv, int &progRC, int &errCode,
               std::string &errMsg)
{
   errCode = 0;
   errMsg = "";

   if (argv.size() < 2) {
      errMsg.assign("ERROR: argv size must be at least 2");
      return 5;
   }

   if (argv.back() != nullptr) {
      errMsg.assign("ERROR: the last element of argv must be NULL");
      return 5;
   }

#ifdef _WIN32

   char errMsgBuf[256];
   std::wstring_view exeName = std::wstring_view(argv[0]);
   std::wstring quote = whatQuote<std::wstring_view> (exeName);
   auto args = std::ranges::subrange(argv.begin() + 1, argv.end() - 1);

   std::wstring cmdline = std::format(L"{}{}{} {}", quote, exeName, quote, join_range(args, L" "));
   int result = Win32CreateProcEx (nullptr, cmdline.data(), true, &progRC, &errCode, errMsgBuf);
   errMsg.assign(errMsgBuf);
   return result;

#else
   return LibcForkExecEx(-1, argv.data(), progRC, errCode, errMsg);
#endif

}

int P3SystemL( const std::string &ProgName, const TExecArgList &ProgParams, int &ProgRC )
{
   const bool allowStdHandles { ProgParams.fInheritedHandles };
   std::string cmd { ProgName };
   for(int i {}; i<ProgParams.Count(); i++)
   {
      const std::string quote = whatQuote(ProgParams[i]);
      cmd += ' ';
      cmd += quote;
      cmd += ProgParams[i];
      cmd += quote;
   }
   switch(OSFileType())
   {
      case OSFileWIN:
         return System4Win( cmd, allowStdHandles, ProgRC );
      case OSFileUNIX:
         return System4Unix( cmd, ProgRC );
      default:
         break;
   }
   assert(false && "unimplemented P3systemL for OSFileType");
   return 0;
}
#endif

int win32ASyncCreateProc( const char *exeName, char *cmdLine, int newConsole, int inheritedHandles, TProcInfo &procInfo );

/*
	* * execute program given in cmdPtr using a shell
	 *   procInfo: pid-style info for the shell running the child program
	 *   msg:      empty on success, nonempty on error
	 *   result:   error code for starting the program - 0 on success, ~0 otherwise
	 *
	*/
static int asyncSystem4Win( const std::string &cmdPtr, bool newConsole, bool inheritedHandles, TProcInfo &procInfo, std::string &msg )
{
#ifdef _WIN32
   msg.clear();
   std::string cs = QueryEnvironmentVariable( "COMSPEC" );
   if( cs.empty() )
   {
      if( FileExists( CMD_WIN7 ) ) cs = CMD_WIN7;
      else if( FileExists( CMD_WINNT ) )
         cs = CMD_WINNT;
      else
      {
         msg = "COMSPEC not set and cmd.exe not found";
         return 1;
      }
   }
   std::string arg = cs + ( !cmdPtr.empty() ? " /C " : "" ) + cmdPtr;
   std::vector<char> argBuf( arg.size() + 1 );
   strcpy( argBuf.data(), arg.c_str() );
   return win32ASyncCreateProc( cs.c_str(), argBuf.data(), newConsole, inheritedHandles, procInfo );
#else
   throw std::runtime_error( "Should never be called on UNIX!" );
#endif
}

static int asyncSystem4Unix( const std::string &cmdPtr, TProcInfo &procInfo, std::string &msg )
{
#ifdef _WIN32
   throw std::runtime_error( "Should never be called on Windows!" );
#else
   int pid, rc = ForkWithSplitArgs( libcASyncForkExec, cmdPtr, pid );
   procInfo.pid = pid;
   return rc;
#endif
}

int p3ASyncSystemP( const std::string &cmdPtr, bool newConsole, TProcInfo &procInfo, std::string &msg )
{
   msg.clear();
   procInfo.clear();
   switch( OSFileType() )
   {
      case OSFileWIN:
         return asyncSystem4Win( cmdPtr, newConsole, true, procInfo, msg );
      case OSFileUNIX:
         return asyncSystem4Unix( cmdPtr, procInfo, msg );
      default:
         assert( 0 && "p3AsyncSystemP not implemented for OSFileType yet!" );
         break;
   }
   return 1;
}

int win32ASyncCreateProc( const char *exeName, char *cmdLine, int newConsole, int inheritedHandles, TProcInfo &procInfo )
{
#ifdef _WIN32
   int nc;
   PROCESS_INFORMATION processInformation;
   STARTUPINFOA startupInfo;

   procInfo.pid = ~0;

   /* Initialize the startup information to be the same as that of the
		 * parent.  This is easier than initializing the many
		 * individual startup information fields and should be fine in most
		 * cases. */
   GetStartupInfoA( &startupInfo );

   if( newConsole )
   {

      /* This new settings allows us to send a CtrlC via the new "GAMS Message Interrupt" to a
			   particular GAMS job without disturbing the parent and vice versa. Along this, we
			   allowed the new async processes to gets their own stdin/out/err. */

      startupInfo.dwFlags |= STARTF_USESHOWWINDOW;
      startupInfo.dwFlags &= ~STARTF_USESTDHANDLES;
      startupInfo.wShowWindow = SW_MINIMIZE;
      inheritedHandles = FALSE;
      nc = CREATE_NEW_CONSOLE;
   }
   else
   {
      startupInfo.wShowWindow = wShowWindow; /* get the locally stored showWindow preference */
      nc = 0;
   }

   if( !CreateProcessA(
               exeName,            /* ApplicationName */
               cmdLine,            /* lpCommandLine */
               nullptr,               /* lpProcessAttributes */
               nullptr,               /* lpThreadAttribute */
               inheritedHandles,   /* bInheritedHandles */
               nc,                 /* dwCreationFlags */
               nullptr,               /* lpEnvironment */
               nullptr,               /* lpCurrentDirectory */
               &startupInfo,       /* lpStartupInfo */
               &processInformation /* lpProcessInformation */
               ) )
   {
      return GetLastError(); /* failed to execute */
   }

   /* child is running now - just clean up and return the process info */
   procInfo.pid = processInformation.dwProcessId;
   procInfo.tid = processInformation.dwThreadId;
   procInfo.hProcess = processInformation.hProcess;

   CloseHandle( processInformation.hThread );
   /* CloseHandle (processInformation.hProcess); */
   return 0;
#else
   throw std::runtime_error( "Function should never be called on UNIX!" );
#endif
}

int p3ASyncExecP( const std::string &cmdPtr, bool newConsole, TProcInfo &procInfo, std::string &msg )
{
   //int argc, i;
   //const char **pargv;
   //std::string s, param;

   procInfo.clear();
   //int res { 1 };
   msg.clear();

   switch( OSFileType() )
   {
      case OSFileWIN:
      {
         std::vector<char> cmdPtrBuf( cmdPtr.size() + 1 );
         strcpy( cmdPtrBuf.data(), cmdPtr.c_str() );
         return win32ASyncCreateProc( nullptr, cmdPtrBuf.data(), newConsole, 1, procInfo );
      }
      case OSFileUNIX:
      {
#ifndef _WIN32
         int pid, rc = ForkWithSplitArgs( libcASyncForkExec, cmdPtr, pid );
         procInfo.pid = pid;
         return rc;
#endif
      }
      default:
         assert( 0 && "p3ASyncExecP not implemented for OSFileType yet" );
   }
   return 1;
}

/*
	* p3ASyncStatus returns information about the process in procInfo
	 * The procInfo.pid should be good on input.  Typically (say, on return from p3ASyncExec? )
	 * the other values in procInfo are good too.  If you only have the PID (e.g. if
	 * you are re-initializing after a solve) set the other values to 0 and
	 * the wait structures will be reinitialized as required.
	 * After you reap a child, a subsequent call on that PID will return 4 (no process)
	 * There are several possible returns:
	 *   0: error (impossible PID, other failures)
	 *   1: process is still running
	 *   2: process is finished with return code in procRC
	 *   3: process is finished but no return code available
	 *   4: no such process
	 * 127: child forked but never exec'ed
	 *
	*/
int p3ASyncStatus( TProcInfo &procInfo, int &progRC, std::string &msg )
{
#ifdef _WIN32
   int res {};
   DWORD rc, exitCode;
   const auto p = static_cast<DWORD>( procInfo.pid );
   auto h = static_cast<HANDLE>( procInfo.hProcess );
   if( !h )
   {
      h = OpenProcess( PROCESS_ALL_ACCESS, FALSE, p );
      if( !h )
      {
         rc = GetLastError();
         switch( rc )
         {
            case ERROR_INVALID_PARAMETER:
               /* system process but that is pid=0, we checked for that already */
               /* or expired or invalid PID */
               msg.clear();
               return 4; /* no such process */
               break;

            case ERROR_ACCESS_DENIED:
            default:
               /*(void)winErrMsg(rc, ebuf, sizeof(ebuf));
					_P3_pchar2str(msg, 255, (SYSTEM_char*)ebuf);*/
               break;

         } /* end switch */

         return 0;
      }
      procInfo.hProcess = h;
      /* we have no way to get a thread ID, given a process ID */
      /* procinfo->tid = 0; */
   }

   /* assume we have a PID and handle now */
   rc = WaitForSingleObject( h, 0 );
   switch( rc )
   {
      case WAIT_OBJECT_0: /* signalled/completed */
         rc = GetExitCodeProcess( h, &exitCode );
         if( rc && ( 0xffffffff != exitCode ) )
         {
            progRC = exitCode;
            res = 2;
         }
         else
         {
            res = 3;
         }
         CloseHandle( h );
         procInfo.hProcess = nullptr;
         return res;
      case WAIT_TIMEOUT: /* still running normally */
         return 1;
      default:
         msg = "Unexpected return from wait";
         return 0;
   }           /* end switch */
   return res; /* should never get here */
#else
   if( !procInfo.pid )
      return 0;

   const auto pid = static_cast<pid_t>( procInfo.pid );

   // PIDs are positive
   if (pid <= 0) {
		msg = "Invalid PID"s;
		return 0;
	}

   // we only use/set the pid on non-windows
   if( procInfo.tid || procInfo.hProcess )
   {
      msg = "Corrupt or bogus procInfo"s;
      return 0;
   }

   int wstat;

   pid_t p2 = waitpid( pid, &wstat, WNOHANG );
   if( pid == p2 )
   {
      // process p has changed state - assume it was to exit
      // consider using waitid() instead of waitpid() to get
		// "more precise control over which child state changes to wait for"
      if( !WIFEXITED( wstat ) ) // no exit code is available
         return 3;
      progRC = WEXITSTATUS( wstat );
      if( 127 == progRC )// return for fork & failed exec
         return 127;
      else // we really have something to return
         return 2;
   }
   // error, e.g. no such process or not a child
   else if( -1 == p2 )
   {
      msg = "No such process or not a child"s;
      return 4;
   }
   // child exists but has not exited
   else if( !p2 )
      return 1;

   msg = "Unexpected return from wait"s;
   return 0;
#endif
}

p3_pid_t p3GetPID(void)
{
#ifdef _WIN32
   return GetCurrentProcessId();
#else
   return getpid();
#endif
}

#ifdef _WIN32
bool killProcessTree( DWORD myprocID )
{
   PROCESSENTRY32 pe;
   HANDLE hSnap;
   HANDLE hChildProc, hProc;

   std::memset( &pe, 0, sizeof( PROCESSENTRY32 ) );
   pe.dwSize = sizeof( PROCESSENTRY32 );

   hSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
   if( INVALID_HANDLE_VALUE == hSnap )
   {
      return FALSE; /* failure */
   }
   if( !Process32First( hSnap, &pe ) )
   {
      CloseHandle( hSnap ); /* clean up the snapshot object */
      return FALSE;         /* failure */
   }

   /* kill the main process */
   hProc = OpenProcess( PROCESS_ALL_ACCESS, FALSE, myprocID );
   if( hProc )
   {
      TerminateProcess( hProc, 1 );
      CloseHandle( hProc );
   }

   for( ;; )
   {
      if( pe.th32ParentProcessID == myprocID )
      {
         /* Recursion */
         killProcessTree( pe.th32ProcessID );

         hChildProc = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID );
         if( hChildProc )
         {
            TerminateProcess( hChildProc, 0 );
            CloseHandle( hChildProc );
         }
      }
      if( !Process32Next( hSnap, &pe ) )
         break;
   }

   /* kill the main process */
   hProc = OpenProcess( PROCESS_ALL_ACCESS, FALSE, myprocID );
   if( hProc )
   {
      TerminateProcess( hProc, 1 );
      CloseHandle( hProc );
   }
   return true;
}
#endif

bool p3KillProcGroupTP( const TProcInfo &procInfo, TKillHow how )
{
#ifdef _WIN32
   return killProcessTree( procInfo.pid );
#else
   return killProcGroupUnix( procInfo.pid, how );
#endif
}

bool p3IsPIDValid( uint32_t pid )
{
#ifdef _WIN32
   DWORD p = pid;
   HANDLE hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, p );
   if( hProcess )
   {
      CloseHandle( hProcess );
      return true;
   }
   return false;
#else
   int rc = unixPidStatus( pid );
   return rc == 1 || !rc;
#endif
}

static p3_global_storage tCtrlHandler CtrlHandler {};

#ifdef _WIN32
static BOOL WINAPI P3Handler( DWORD s )
{
   if( CTRL_C_EVENT == s )
   {
      if( CtrlHandler )
      {
         ( *CtrlHandler )();
         return TRUE;
      }
   }
   return FALSE;
}
#else

static p3_global_storage sigset_t sigSet;
static p3_global_storage struct sigaction newAction, oldAction;

extern "C" {

static p3_global_storage void ( *oldHandler )( int );

static void p3CtrlCHandler( int sig );
}
static void p3CtrlCHandler( int sig )
{
   if( CtrlHandler ) ( *CtrlHandler )();
}

#endif

CtrlHandlerState P3InstallCtrlHandler( tCtrlHandler newHandler )
{
   if( CtrlHandler )
   {
      CtrlHandler = newHandler;
      return P3CtrlHandlerOK;
   }
#ifdef _WIN32
   CtrlHandler = newHandler;
   bool rc = SetConsoleCtrlHandler( &P3Handler, true );
   if( !rc ) CtrlHandler = nullptr;
   return rc ? P3CtrlHandlerOK : P3CtrlHandlerSysFail;
#else
   int rc = sigemptyset( &sigSet );
   if( rc ) return P3CtrlHandlerSysFail;
   newAction.sa_handler = p3CtrlCHandler;
   newAction.sa_mask = sigSet;
   newAction.sa_flags = 0;
   CtrlHandler = newHandler;
   rc = sigaction( SIGINT, &newAction, &oldAction );
   if( rc )
   {
      CtrlHandler = nullptr;
      return P3CtrlHandlerSysFail;
   }
   else
   {
      oldHandler = oldAction.sa_handler;
      return P3CtrlHandlerOK;
   }
#endif
}

int P3UninstallCtrlHandler()
{
#ifdef _WIN32
   CtrlHandler = nullptr;
   return SetConsoleCtrlHandler( &P3Handler, false ) ? P3CtrlHandlerOK : P3CtrlHandlerSysFail;
#else
   int rc = sigemptyset( &sigSet );
   if( rc ) return P3CtrlHandlerSysFail;
   newAction.sa_handler = oldHandler;
   newAction.sa_mask = sigSet;
   newAction.sa_flags = 0;
   rc = sigaction( SIGINT, &newAction, &oldAction );
   CtrlHandler = nullptr;
   return rc || oldAction.sa_handler != p3CtrlCHandler ? P3CtrlHandlerSysFail : P3CtrlHandlerOK;
#endif
}

tCtrlHandler P3GetCtrlHandler()
{
   return CtrlHandler;
}
}// namespace rtl::p3process
