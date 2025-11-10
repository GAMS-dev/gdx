/*
* GAMS - General Algebraic Modeling System GDX API
 *
 * Copyright (c) 2017-2025 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2025 GAMS Development Corp. <support@gams.com>
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
#include <cstdint>
#include <string>


#include "delphitypes.hpp"

namespace rtl::p3process
{

enum TKillHow : uint8_t
{
   soft,
   hard
};

struct TProcInfo {
   uint32_t pid {};// process ID
   uint32_t tid {};// thread ID
   void *hProcess {};

   void clear() {
      pid = tid = 0;
      hProcess = nullptr;
   }
};

class TExecArgList final
{
   void Put(int Index, const std::string &Item);
   int Split(bool append, const std::string &s);
   std::vector<std::string> FList {};
public:
   [[maybe_unused]] bool fInheritedHandles {};
   TExecArgList();
   ~TExecArgList();
   int Add(const std::string &Item );
   void Clear();
   void Delete(int Index);
   void Insert(int Index, const std::string &Item);
   int SplitAppend(const std::string &s);
   int SplitPrepend(const std::string &s);
   [[nodiscard]] int GetCapacity() const;
   [[nodiscard]] std::string Get(int Index ) const;
   [[nodiscard]] std::string GetLast() const;
   [[nodiscard]] int Count() const;
   std::string operator[]( int Index ) const;
};

bool p3GetCPUInfo( int &nSockets, int &nCores, int &nThreads, int &coresPerSocket, int &threadsPerCore );
int p3GetNumberOfProcessors();

int P3SystemP( const std::string &CmdPtr, int &ProgRC );
int P3ExecP( const std::string &CmdPtr, int &ProgRC );

int P3SystemL( const std::string &ProgName, const TExecArgList &ProgParams, int &ProgRC );

int p3ASyncSystemP( const std::string &cmdPtr, bool newConsole, TProcInfo &procInfo, std::string &msg );
int p3ASyncExecP( const std::string &cmdPtr, bool newConsole, TProcInfo &procInfo, std::string &msg );

int p3ASyncStatus( TProcInfo &procInfo, int &progRC, std::string &msg );

bool p3KillProcGroupTP( const TProcInfo &procInfo, TKillHow how );
bool p3IsPIDValid( uint32_t pid );

using tCtrlHandler = void(*)();

enum CtrlHandlerState : uint8_t
{
   P3CtrlHandlerOK,
   P3CtrlHandlerWasEmpty,
   P3CtrlHandlerSysFail
};

CtrlHandlerState P3InstallCtrlHandler( tCtrlHandler newHandler );
int P3UninstallCtrlHandler();
tCtrlHandler P3GetCtrlHandler();

}// namespace rtl::p3process