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
#include <vector>
#include <cstdint>

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace rtl::p3utils
{

void initParamStr( int argc, const char **argv );

int p3Chmod( const std::string &path, int mode );

double RealTrunc( double x );
double ReadRound( double x );

// delphiGetDecDigits is exposed so we can test it again p3GetDecDigits
bool delphiGetDecDigits( double y, int mode, int nDigits, std::string &digits, int &decPos, int &minusCnt );

bool p3GetDecDigits( double y, int mode, int nDigits, std::string &digits, int &decPos, int &minusCnt );
std::string p3FloatToEfmt( double x, int width, int decimals );
std::string getDigits( int64_t i64 );

std::string FloatToE( double y, int decimals );

std::string ParamStrZero();
std::string ParamStr( int index );
int ParamStrCount();
std::string loadPathVarName();
bool PrefixLoadPath( const std::string &dir );
bool PrefixEnv( const std::string &dir, std::string &evName );

constexpr int NLocNames = 8;
using TLocNames = std::vector<std::string>;

enum Tp3Location
{
   p3Config,
   p3AppConfig,
   p3Data,
   p3AppData,
   p3AppLocalData,
   p3Documents
};

bool p3StandardLocations( Tp3Location locType, const std::string &appName, TLocNames &locNames, int &eCount );
bool p3WritableLocation( Tp3Location locType, const std::string &appName, std::string &locName );

std::string ExtractFileExt( const std::string &filename );
std::string ChangeFileExt( const std::string &filename, const std::string &extension );
std::string CompleteFileExt( const std::string &filename, const std::string &extension );
std::string ReplaceFileExt( const std::string &filename, const std::string &extension );

bool PrefixPath( const std::string &s );

bool P3SetEnv( const std::string &name, const std::string &val );
std::string P3GetEnv( const std::string &name );
void P3UnSetEnv( const std::string &name );
bool P3IsSetEnv( const std::string &name );

bool P3SetEnvPC( const std::string &name, char *val );
uint32_t P3GetEnvPC( const std::string &name, char *buf, uint32_t bufSize );

int p3GetExecName( std::string &execName, std::string &msg );
int p3GetLibName( std::string &libName, std::string &msg );

bool p3GetMemoryInfo( int64_t &rss, int64_t &vss );

void p3SetConsoleTitle( const std::string &s );
void p3NoPopups();

std::string p3GetUserName();
std::string p3GetComputerName();

using Tp3File = std::fstream;
using Tp3FileHandle = std::fstream *;
enum Tp3FileOpenAction
{
   p3OpenRead,
   p3OpenWrite,
   p3OpenReadWrite
};

int p3FileOpen( const std::string &fName, Tp3FileOpenAction mode, Tp3FileHandle h );
int p3FileClose( Tp3FileHandle h );
int p3FileRead( Tp3FileHandle h, char *buffer, uint32_t buflen, uint32_t &numRead );
int p3FileWrite( Tp3FileHandle h, const char *buffer, uint32_t buflen, uint32_t &numWritten );
int p3FileGetSize( Tp3FileHandle h, int64_t &fileSize );

bool p3GetFirstMACAddress( std::string &mac );

union T_P3SOCKET {
   uint64_t wsocket;
   int socketfd;
};

T_P3SOCKET p3SockGetInvalid();
bool p3SockIsInvalid(T_P3SOCKET s);
int p3SockClose(T_P3SOCKET &s);
T_P3SOCKET p3SockCreateConnectedClient(int port);

bool p3SockSend(T_P3SOCKET s, const char *buf, int count, int &res);
bool p3SockSendTimeout(T_P3SOCKET s, const char *buf, int count, int &res, int timeOut);

int p3SockStartUp();
void p3SockCleanUp();

bool p3SockRecv(T_P3SOCKET s, char *buf, int count, int &res);
bool p3SockRecvTimeout(T_P3SOCKET s, char *buf, int count, int &res, int timeOut);

T_P3SOCKET p3SockAcceptClientConn(T_P3SOCKET srvSock);
T_P3SOCKET p3SockCreateServerSocket(int port, bool reuse);

// ...

}// namespace rtl::p3utils
