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


#include "runner.h"
#include "utils.h"
#include "../rtl/p3process.h"
#include "../rtl/sysutils_p3.h"

using namespace std::literals::string_literals;

namespace gdlib::runner
{

TMsgHandler::TMsgHandler( const std::string &MsgPfx ) : FMsgPfx { MsgPfx }
{
}

void TMsgHandler::ErrorMessage( int ec, const std::string &s )
{
   debugStream << "*** Error: "s + s << '\n';
}

void TMsgHandler::LogMessage( const std::string &s ) const
{
   if( FVerbose >= 1 )
      debugStream << s << std::endl;
}

void TMsgHandler::DebugMessage( const std::string &s ) const
{
   if( FVerbose >= 2 )
      LogMessage( FMsgPfx + ": "s + s );
}

TRunner::TRunner() : FMsgHandler { "Runner"s }
{
}

TRunner::~TRunner() = default;

void TRunner::ParamsAdd( const std::string &v )
{
   if( ErrorWhenRunning( "ParamsA"s ) ) return;
   FParams.push_back( v );
   CommandLineChanged();
}

void TRunner::ParamsClear()
{
   if( ErrorWhenRunning( "ParamsClear"s ) ) return;
   FParams.clear();
   CommandLineChanged();
}

void TRunner::SetExecutable( const std::string &v )
{
   if( ErrorWhenRunning( "Executable"s ) ) return;
   FExecutable = v;
   CommandLineChanged();
}

std::string TRunner::GetExecutable()
{
   return FExecutable;
}

bool TRunner::IsRunning() const
{
   return FIsRunning;
}

int TRunner::ParamsCount()
{
   return static_cast<int>( FParams.size() );
}

std::string TRunner::CommandLine()
{
   if( !FCommandLine.empty() ) return FCommandLine;
   const char Q { '\"' };
   FCommandLine = utils::quoteWhitespace( FExecutable, Q );
   for( const auto &param: FParams )
      FCommandLine += ' ' + utils::quoteWhitespace( param, Q );

   return FCommandLine;
}

int TRunner::StartAndWait()
{
   if( IsRunning() )
   {
      FMsgHandler.ErrorMessage( EC_Process_Active, "Cannot start an active process"s );
      return EC_Process_Active;
   }
   if( CommandLine().empty() )
   {
      FMsgHandler.ErrorMessage( EC_Empty_CMD_Line, "No Command Line specified"s );
      return EC_Empty_CMD_Line;
   }
   if( FUseShell )
   {
      FMsgHandler.DebugMessage( "Use shell: "s + FCommandLine );
      return rtl::p3process::P3SystemP( FCommandLine, FProgRC );
   }
   else
   {
      FMsgHandler.DebugMessage( "Direct call: "s + FCommandLine );
      int res { rtl::p3process::P3ExecP( FCommandLine, FProgRC ) };
      FMsgHandler.DebugMessage( "Return = "s + rtl::sysutils_p3::IntToStr( res ) + " RC = "s + rtl::sysutils_p3::IntToStr( FProgRC ) );
      return res;
   }
}

int TRunner::GetProgRC() const
{
   return 0;
}

bool TRunner::ErrorWhenRunning( const std::string &s )
{
   const bool res { IsRunning() };
   if( res )
      FMsgHandler.ErrorMessage( EC_Cannot_modify, "Cannot modify "s + s + "when process is active"s );
   return res;
}

std::string TRunner::GetWorkDir() const
{
   return FWorkDir;
}

void TRunner::SetWorkDir( const std::string &v )
{
   if( ErrorWhenRunning( "WorkDir"s ) ) return;
   FWorkDir = v;
}

void TRunner::SetInheritHandles( bool v )
{
   if( ErrorWhenRunning( "InheritHandles"s ) ) return;
   FInheritHandles = v;
}

bool TRunner::GetInheritHandles() const
{
   return FInheritHandles;
}

void TRunner::SetUseShell( bool v )
{
   if( ErrorWhenRunning( "UseShell"s ) ) return;
   FUseShell = v;
   CommandLineChanged();
}

bool TRunner::GetUseShell() const
{
   return FUseShell;
}

void TRunner::CommandLineChanged()
{
   FCommandLine.clear();
}

int TRunner::GetVerbose()
{
   return FMsgHandler.FVerbose;
}

void TRunner::SetVerbose( int v )
{
   FMsgHandler.FVerbose = v;
}

void TRunner::SetVisible( TVisible v )
{
   if( ErrorWhenRunning( "Visible"s ) ) return;
   FVisible = v;
}

TVisible TRunner::GetVisible() const
{
   return FVisible;
}
}// namespace gdlib::runner