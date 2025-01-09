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

namespace gdlib::runner
{

const int EC_Cannot_modify = 1;
const int EC_Process_Active = 2;
const int EC_Empty_CMD_Line = 3;

enum TVisible : uint8_t
{
   vis_hide,
   vis_minimized,
   vis_normal
};

class TRunner;

class TMsgHandler
{
   int FVerbose {1};
   std::string FMsgPfx;

public:
   friend class TRunner;

   explicit TMsgHandler( const std::string &MsgPfx );
   static void ErrorMessage( int ec, const std::string &s );
   void LogMessage( const std::string &s ) const;
   void DebugMessage( const std::string &s ) const;
};


class TRunner
{
   TMsgHandler FMsgHandler;
   std::string FExecutable{}, FWorkDir{}, FCommandLine{};
   std::vector<std::string> FParams{};
   bool FIsRunning{}, FInheritHandles{}, FUseShell{};
   TVisible FVisible{};
   int FProgRC{};

   bool ErrorWhenRunning( const std::string &s );
   void CommandLineChanged();

public:
   explicit TRunner();
   ~TRunner();

   void ParamsAdd( const std::string &v );
   void ParamsClear();
   int ParamsCount();
   std::string CommandLine();
   int StartAndWait();

   void SetExecutable( const std::string &v );
   std::string GetExecutable();

   [[nodiscard]] bool IsRunning() const;

   [[nodiscard]] std::string GetWorkDir() const;
   void SetWorkDir( const std::string &v );

   void SetInheritHandles( bool v );
   [[nodiscard]] bool GetInheritHandles() const;

   void SetUseShell( bool v );
   [[nodiscard]] bool GetUseShell() const;

   int GetVerbose();
   void SetVerbose( int v );

   void SetVisible( TVisible v );
   [[nodiscard]] TVisible GetVisible() const;

   [[nodiscard]] int GetProgRC() const;
};

}// namespace gdlib::runner
