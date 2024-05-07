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

#include <map>
#include <optional>
#include <vector>
#include <string>
#include <list>
#include <cstdint>

namespace gdlib::gmsstrm
{
class TXStream;
}

namespace gdlib::gmacro
{

struct strCompCaseInsensitive {
   bool operator()( const std::string &lhs, const std::string &rhs ) const;
};

class TParamList : public std::vector<std::string>
{
   std::map<std::string, std::string, strCompCaseInsensitive> objects;

public:
   TParamList();
   explicit TParamList( gdlib::gmsstrm::TXStream &S );

   void SetValue( int N, const std::string &S );
   [[nodiscard]] std::string GetValue( int N ) const;
   void FreeObject( int Index );
   void WriteToStream( gdlib::gmsstrm::TXStream &S ) const;
   [[nodiscard]] int IndexOf( const std::string &s ) const;
};

enum TErrTyp : uint8_t
{
   errInput,
   errDefinition,
   errActivation
};

class TPWriter
{
public:
   std::string pw;
   std::string &ps = pw;

   TPWriter();
   virtual ~TPWriter();

   virtual void StoreCh( char ch );
   void StoreStr( const std::string &s );
   void StoreTrimStr( const std::string &s );
   void Reset();

   void Release( bool Z, std::string &P );
};

class TPWriterCC final : public TPWriter
{
public:
   TPWriterCC();
   ~TPWriterCC() override = default;
   void StoreCh( char ch ) override;
};

class TPReader
{
   const std::string &p;

public:
   std::string pr;
   int off {};

   explicit TPReader( const std::string &_p );
   ~TPReader();

   char NextCh();
   char NextChNoBlank();
   void Advance( int L );
   void UndoNextCh();
   [[nodiscard]] char ChFromEnd( int Ofs ) const;
};

class TGAMSMacroList;

class TGAMSMacro final
{
   std::string Name{};
   TGAMSMacroList *FML {};
   TParamList Params{};
   std::string Body {};
   int Active {};

public:
   TGAMSMacro() = default;
   TGAMSMacro( const TGAMSMacro &other ) = default;
   TGAMSMacro( std::string id, TGAMSMacroList &ML );
   TGAMSMacro( gmsstrm::TXStream &S, TGAMSMacroList &ML );
   ~TGAMSMacro() = default;

   bool Define( const std::string &p, int &LUsed );
   [[nodiscard]] int GetArgCount() const;
   [[nodiscard]] std::string GetName() const;
   void SetName( const std::string &name );
   bool GetParameters( const std::string &src, int &LUsed );
   void GetParameter( TPReader &Rdr, TPWriterCC &Wrt ) const;
   [[nodiscard]] std::string GetParam( int index ) const;
   [[nodiscard]] std::string GetBody() const;
   bool GetMacroBody( std::string &s );
   void Error( TErrTyp ErrTyp, const std::string &s ) const;
   void WriteToStream( gmsstrm::TXStream &S ) const;
   void SubstituteParams( TPReader &Rdr, TPWriterCC &Wrt ) const;
   void SetValue( int N, const std::string &P );
   void GetValue( int N, std::string &P ) const;
   void AddMsgStr( const std::string &s, const std::string &p ) const;
   bool AddToBody( const std::string &p );
};

// TODO: FIXME: Evalute using a TXStrHashList for performance instead of std::map!!!

struct MacroIndexPair {
   TGAMSMacro *macro;
   int index;
};

using TMacroList = std::map<std::string, MacroIndexPair, strCompCaseInsensitive>;

class TGAMSMacroList final
{
   bool ErrorFree {true};
   std::string ErrMsg;
   int FTrace {};
   //char *FResult {};
   TMacroList MacList;
   std::vector<TGAMSMacro *> MacListOrderedPtrs;
   std::vector<std::string> MsgList;
   std::vector<TGAMSMacro *> CallList;

   void Error( const std::string &s );
   TGAMSMacro *FindMacro( const std::string &id );
   bool GetError( const std::string &s );
   void AddMsgStr( const std::string &s, const std::string &p );

public:
   bool ExpandArguments {true};

   friend TGAMSMacro;

   TGAMSMacroList() = default;
   explicit TGAMSMacroList( gmsstrm::TXStream &S );
   ~TGAMSMacroList();

   bool AddMacro( const std::string &Id, const std::string &p, int &LUsed, bool allowRedef, std::string &Msg );
   [[nodiscard]] bool AddToBody( const std::string &s ) const;
   std::string ExpandMacro( TGAMSMacro &macro, const std::string &src, int &LUsed );
   // Delphi version was with char* str with length info and char array string output parameter, bool return for success
   // Now using C++ strings and optional return (nullopt for fail)
   std::optional<std::string> ExpandMacro( const std::string &id, const std::string &src, int &LUsed );
   bool GetDefinition( int N, std::string &Id, int &ArgCount, std::string &P ) const;
   void WriteToStream( gmsstrm::TXStream &S ) const;
   int SetTraceLevel( int N );
   std::optional<std::string> GetMessage( int N );
   [[nodiscard]] int MsgCount() const;
   [[nodiscard]] int GetCount() const;
   bool RenameMacro( const std::string &idold, const std::string &idnew );
};
}// namespace gdlib::gmacro