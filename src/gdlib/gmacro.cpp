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


#include "gmacro.h"
#include <cassert>
#include <cstring>
#include <utility>
#include "utils.h"
#include "gmsstrm.h"

using namespace std::literals::string_literals;


namespace gdlib::gmacro
{

bool strCompCaseInsensitive::operator()( const std::string &lhs, const std::string &rhs ) const
{
#if defined( _WIN32 )
   return _stricmp( lhs.c_str(), rhs.c_str() ) < 0;
#else
   return strcasecmp( lhs.c_str(), rhs.c_str() ) < 0;
#endif
}

/*static bool macroAlreadyThere( const TMacroList &MLUT, const std::string &Id )
{
   return MLUT.find( Id ) != MLUT.end();
}*/

constexpr char CharConcat = '&';
const std::set<char> IDFirst = utils::multiCharSetRanges( { { 'a', 'z' }, { 'A', 'Z' } } ),
                     IDSecond = utils::multiCharSetRanges( { { 'a', 'z' }, { 'A', 'Z' }, { '0', '9' }, { '_', '_' } } );

static bool GoodIdent( const std::string &id )
{
   return !id.empty() && utils::in( id.front(), IDFirst ) && std::all_of( id.begin() + 1, id.end(), [&]( char c ) { return utils::in( c, IDSecond ); } );
}

TGAMSMacroList::TGAMSMacroList( gdlib::gmsstrm::TXStreamDelphi &S )
{
   [[maybe_unused]] const int V{S.ReadInteger()};
   assert(V <= 1 && "Incompatible Macro format");
   int ix{};
   const int numMacros { S.ReadInteger() };
   MacListOrderedPtrs.reserve( numMacros );
   for(int N{numMacros}; N > 0; N--)
   {
      const auto M {new TGAMSMacro { S, *this }};
      MacListOrderedPtrs[ix] = M;
      MacList.emplace( M->GetName(), MacroIndexPair { M, ix++ } );
   }
}

TGAMSMacroList::~TGAMSMacroList() {
   for (const auto* m : MacListOrderedPtrs)
      delete m;
}

bool TGAMSMacroList::AddMacro( const std::string &Id, const std::string &p, int &LUsed, bool allowRedef, std::string &Msg )
{
   LUsed = 0;
   ErrorFree = true;
   Msg.clear();
   const auto it = MacList.find( Id );
   int N {it == MacList.end() ? -1 : it->second.index};
   // known already
   if( N >= 0 )
   {
      if( !allowRedef )
         Msg = "Macro Definition error for Macro: "s + Id + ": Name used already"s;
      else
      {
         delete MacListOrderedPtrs[N];
         MacListOrderedPtrs[N] = nullptr;
         it->second.macro = nullptr;
      }
   }
   else if( !GoodIdent( Id ) )
      Msg = "Macro Definition error for Macro: "s + Id + ": Bad Identifier"s;
   
   TGAMSMacro *M { new TGAMSMacro { Id, *this } };
   const bool res { !p.empty() && M->Define( p, LUsed ) };
   // known already and redef is allowed
   if( N >= 0 )
   {
      MacListOrderedPtrs[N] = M;
      MacList[Id] = MacroIndexPair { M, N };
   }
   else // new macro id
   {
      N = static_cast<int>(MacList.size());
      MacList.emplace( Id, MacroIndexPair { M, N } );
      MacListOrderedPtrs.push_back( M );
   }
   
   if( Msg.empty() && !ErrorFree ) Msg = ErrMsg;
   return res;
}

bool TGAMSMacroList::AddToBody( const std::string &s ) const
{
   return !MacList.empty() && MacListOrderedPtrs.back()->AddToBody( s );
}

bool TGAMSMacroList::GetDefinition( const int N, std::string &Id, int &ArgCount, std::string &P ) const
{
   TPWriter Wrt;
   const bool res { N >= 0 && N < static_cast<int>( MacList.size() ) };
   if( !res )
   {
      Id.clear();
      Wrt.StoreStr( "Bad index "s + std::to_string( N ) );
      ArgCount = 0;
   }
   else
   {
      const TGAMSMacro &M = *MacListOrderedPtrs[N];
      Id = M.GetName();
      ArgCount = M.GetArgCount();
      if( ArgCount > 0 )
      {
         Wrt.StoreCh( '(' );
         for( int k = 0; k < ArgCount; k++ )
         {
            if( k > 0 ) Wrt.StoreCh( ',' );
            Wrt.StoreStr( M.GetParam( k ) );
         }
         Wrt.StoreStr( ")   " );
      }
      Wrt.StoreStr( M.GetBody() );
   }
   Wrt.Release( true, P );
   return res;
}

void TGAMSMacroList::WriteToStream( gmsstrm::TXStreamDelphi &S ) const
{
   S.WriteInteger( 1 );// version
   S.WriteInteger( static_cast<int>( MacList.size() ) );
   for(const TGAMSMacro *m : MacListOrderedPtrs)
      m->WriteToStream( S );
}

int TGAMSMacroList::MsgCount() const
{
   return static_cast<int>( MsgList.size() );
}

int TGAMSMacroList::GetCount() const
{
   return static_cast<int>( MacList.size() );
}

bool TGAMSMacroList::RenameMacro( const std::string &idold, const std::string &idnew )
{
   TGAMSMacro *M = FindMacro( idold );
   if( M ) M->SetName( idnew );
   return M;
}

std::optional<std::string> TGAMSMacroList::GetMessage( int N )
{
   return N < 0 || N >= static_cast<int>( MsgList.size() ) ? std::nullopt : std::make_optional( MsgList[N] );
}

std::string TGAMSMacroList::ExpandMacro( TGAMSMacro &macro, const std::string &src, int &LUsed )
{
   ErrorFree = true;
   CallList.clear();
   MsgList.clear();

   std::string out {};

   if( !macro.GetParameters( src, LUsed ) )
   {
      GetError( src );
      if( FTrace >= 1 ) AddMsgStr( ""s, src );
      goto Done;
   }

   if( !macro.GetMacroBody( out ) )
   {
      GetError( out );
      if( FTrace >= 1 ) AddMsgStr( ""s, out );
      goto Done;
   }
   if( FTrace >= 1 ) AddMsgStr( ": Exit: "s, src );

Done:
   return out;
}

static TGAMSMacro *macroLookup( TMacroList &MLUT, const std::string &id )
{
   if( const auto it { MLUT.find( id ) }; it != MLUT.end() )
      return it->second.macro;
   return nullptr;
}

// Delphi version was with char* str with length info and char array string output parameter, bool return for success
// Now using C++ strings and optional return (nullopt for fail)
std::optional<std::string> TGAMSMacroList::ExpandMacro( const std::string &id, const std::string &src, int &LUsed )
{
   if( TGAMSMacro *m = macroLookup( MacList, id ) )
      return ExpandMacro( *m, src, LUsed );
   Error( "Macro not found: "s + id );
   GetError( id );
   return std::nullopt;
}

int TGAMSMacroList::SetTraceLevel( const int N )
{
   const int oldFTrace = FTrace;
   FTrace = N;
   //MsgList.clear();
   return oldFTrace;
}

void TGAMSMacroList::Error( const std::string &s )
{
   if( ErrorFree )
   {
      ErrMsg = s;
      ErrorFree = false;
   }
}

TGAMSMacro *TGAMSMacroList::FindMacro( const std::string &id )
{
   return macroLookup( MacList, id );
}

bool TGAMSMacroList::GetError( const std::string &s )
{
   TPWriter Wrt;
   bool res { !ErrorFree };
   if( ErrorFree ) Wrt.StoreStr( "No Error(s)"s );
   else
   {
      Wrt.StoreStr( ErrMsg );
      if( !CallList.empty() )
      {
         Wrt.StoreStr( " Call Stack: ["s );
         for( int N {}; N < (int)CallList.size(); N++ )
         {
            if( N > 0 ) Wrt.StoreStr( " | "s );
            Wrt.StoreStr( CallList[N]->GetName() );
         }
      }
   }
   //Wrt.Release(true, s);
   return res;
}

void TGAMSMacroList::AddMsgStr( const std::string &s, const std::string &p )
{
   MsgList.push_back( s + p );
}

TGAMSMacro::TGAMSMacro( std::string id, TGAMSMacroList &ML ) : Name( std::move( id ) ), FML( &ML ) {}

TGAMSMacro::TGAMSMacro( gmsstrm::TXStreamDelphi &S, TGAMSMacroList &ML ) :
   Name( S.ReadString() ), FML( &ML ), Params( S ), Body( S.ReadPChar() )
{
}

bool TGAMSMacro::Define( const std::string &p, int &LUsed )
{
   TPReader Rdr { p };
   char Ch { Rdr.NextCh() };
   if( Ch != '(' ) Rdr.UndoNextCh();
   else
   {
      Ch = Rdr.NextChNoBlank();
      do {
         if( !utils::in( Ch, IDFirst ) )
         {
            Error( errDefinition, "Param ID expected"s );
            break;
         }
         std::string Id { Ch };
         do {
            Ch = Rdr.NextCh();
            if( !utils::in( Ch, IDSecond ) ) break;
            if( Id.length() >= 255 )
            {
               Error( errDefinition, "Param ID too long"s );
               break;
            }
            Id += Ch;
         } while( true );

         const auto it = std::find( Params.begin(), Params.end(), Id );
         if( it != Params.end() )
         {
            Error( errDefinition, "Duplicate Param ID"s );
            break;
         }
         Params.push_back( Id );
         if( Ch == ' ' || Ch == '\t' ) Ch = Rdr.NextChNoBlank();
         if( Ch != ',' ) break;
         Ch = Rdr.NextChNoBlank();
      } while( true );
      if( Ch != ')' ) Error( errDefinition, "Closing parenthesis missing"s );
   }
   Body.clear();
   LUsed = static_cast<int>( p.length() );
   return AddToBody( Rdr.pr );
}

inline int TGAMSMacro::GetArgCount() const { return (int) Params.size(); }

bool TGAMSMacro::AddToBody( const std::string &p )
{
   if( p.empty() ) return false;

   TPWriter Wrt;
   Wrt.StoreStr( Body );
   const auto L = static_cast<int>( p.length() );
   const TPReader Rdr { p };
   const bool res { Rdr.ChFromEnd( 0 ) == '\\' };
   if( !res ) Wrt.StoreTrimStr( p );
   else
   {
      int firstNonBlankIx = 0;
      if( L > 2 )
      {
         const auto nbit = std::find_if( p.begin(), p.end(), []( char c ) { return c != ' '; } );
         if( nbit != p.end() )
            firstNonBlankIx = static_cast<int>( nbit - p.begin() );
      }
      const int numCharsDroppedFromBack = Rdr.ChFromEnd( 1 ) != CharConcat ? 1 : 2;
      auto itslen = L - firstNonBlankIx - numCharsDroppedFromBack;
      // AS: FIXME: Ugly hack. Edge case for empty line in testlib/macro02
      if( !itslen )
      {
         itslen = 1;
         firstNonBlankIx--;
      }
      Wrt.StoreStr( p.substr( firstNonBlankIx, itslen ) );
   }
   Body.clear();
   Wrt.Release( false, Body );
   return res;
}

const std::array ErrHdr = { "Input"s, "Definition"s, "Activation"s };

std::string TGAMSMacro::GetBody() const { return Body; }

bool TGAMSMacro::GetMacroBody( std::string &s )
{
   TPWriterCC Wrt;
   Active++;
   FML->CallList.push_back( this );
   if( !Body.empty() )
   {
      if( FML->FTrace >= 2 ) AddMsgStr( ": Subst before: "s, Body );
      TPReader Rdr { Body };
      SubstituteParams( Rdr, Wrt );
      if( FML->FTrace >= 2 ) AddMsgStr( ": Subst after : "s, Wrt.ps );
   }

   Active--;
   const bool res { FML->ErrorFree };
   if( !res ) s.clear();
   else
   {
      Wrt.Release( true, s );
      FML->CallList.pop_back();
   }
   return res;
}

void TGAMSMacro::Error( TErrTyp ErrTyp, const std::string &s ) const
{
   FML->Error( "Macro "s + ErrHdr[ErrTyp] + " error for Macro "s + Name + ": "s + s );
}

inline std::string TGAMSMacro::GetName() const { return Name; }

void TGAMSMacro::SetName( const std::string &name ) { Name = name; }

bool TGAMSMacro::GetParameters( const std::string &src, int &LUsed )
{
   TPReader Rdr { src };
   if( GetArgCount() > 0 )
   {
      Active++;
      if( char Ch = Rdr.NextChNoBlank(); Ch != '(' ) Error( errActivation, "Opening parenthesis expected"s );
      else
      {
         TPWriterCC Wrt;
         Ch = ',';
         for( int Arg {}; Arg < GetArgCount(); Arg++ )
         {
            if( Ch == ',' ) Ch = Rdr.NextChNoBlank();
            else
            {
               Error( errActivation, "Comma missing in parameter list"s );
               break;
            }
            if( Ch == '\0' || Ch == ')' )
            {
               Error( errActivation, "Too few parameters"s );
               break;
            }
            if( Ch == ',' )
            {
               Error( errActivation, "Empty parameter"s );
               break;
            }

            Wrt.Reset();
            Rdr.UndoNextCh();
            GetParameter( Rdr, Wrt );
            SetValue( Arg, Wrt.ps );
            Ch = Rdr.NextCh();
            if( Ch == ' ' || Ch == '\b' ) Ch = Rdr.NextChNoBlank();
         }
         if( Ch != ')' ) Error( errActivation, "Closing parenthesis missing"s );
      }
      Active--;
   }
   LUsed = static_cast<int>( src.length() - Rdr.pr.length() );
   return FML->ErrorFree;
}

void TGAMSMacro::GetParameter( TPReader &Rdr, TPWriterCC &Wrt ) const
{
   char Stop;
   int Pc;
   bool done {};
   while( !done )
   {
      char Ch = Rdr.NextCh();
      switch( Ch )
      {
         case '\"':
         case '\'':
            Stop = Ch;
            Wrt.StoreCh( Ch );
            do {
               Ch = Rdr.NextCh();
               if( Ch == '\0' )
               {
                  Error( errActivation, "Closing quote missing in parameter"s );
                  return;
               }
               Wrt.StoreCh( Ch );
            } while( Ch != Stop );
            break;
         case '(':
            Pc = 1;
            Wrt.StoreCh( '(' );
            do {
               Ch = Rdr.NextCh();
               if( Ch == '\0' )
               {
                  Error( errActivation, "Closing parenthesis missing in parameter"s );
                  return;
               }
               Wrt.StoreCh( Ch );
               if( Ch == '(' ) Pc++;
               else if( Ch == ')' )
                  Pc--;
            } while( Pc != 0 );
            break;
         case ')':
         case ',':
         case '\0':
            Rdr.UndoNextCh();
            done = true;
            break;
         default:
            Wrt.StoreCh( Ch );
            break;
      }
   }
}

std::string TGAMSMacro::GetParam( int index ) const { return Params[index]; }

void TGAMSMacro::SetValue( const int N, const std::string &P )
{
   if( FML->FTrace >= 2 ) AddMsgStr( ": SetParam-in"s + std::to_string( N + 1 ) + ": ", P );
   TPReader Rdr { P };
   TPWriterCC Wrt {};
   char Ch { ' ' };
   bool IdOk { true };
   bool done {};
   char stop;
   std::string Id;
   while( Ch != '\0' && !done )
   {
      do {
         Ch = Rdr.NextCh();
         switch( Ch )
         {
            case '\0':
               done = true;
               break;
            case '\'':
            case '\"':
               stop = Ch;
               Wrt.StoreCh( Ch );
               do {
                  Ch = Rdr.NextCh();
                  if( Ch == '\0' ) break;
                  Wrt.StoreCh( Ch );
               } while( Ch != stop );
               if( Ch == '\0' ) Error( errActivation, "Closing quote missing in parameter"s );
               break;
            default:
               if( isalpha( Ch ) )
               {
                  if( !IdOk )
                  {
                     Wrt.StoreCh( Ch );
                     continue;
                  }
                  Id.clear();
                  Id += Ch;
                  do {
                     Ch = Rdr.NextCh();
                     if( !utils::in( Ch, IDSecond ) )
                     {
                        if( Ch != '\0' ) Rdr.UndoNextCh();
                        TGAMSMacro *M = !FML->ExpandArguments ? nullptr : FML->FindMacro( Id );
                        if( !M || M->Active > 0 ) Wrt.StoreStr( Id );
                        else
                        {
                           int XU;
                           if( !M->GetParameters( Rdr.pr, XU ) ) break;
                           Rdr.Advance( XU );
                           std::string Xp;
                           if( !M->GetMacroBody( Xp ) ) break;
                           Wrt.StoreStr( Xp );
                        }
                        break;
                     }
                     Id += Ch;
                     if( Id.length() >= 255 )
                     {
                        Wrt.StoreStr( Id );
                        do {
                           Ch = Rdr.NextCh();
                           if( Ch == '\0' ) break;
                           Wrt.StoreCh( Ch );
                        } while( utils::in( Ch, IDSecond ) );
                        break;
                     }
                  } while( true );
               }
               else
               {
                  Wrt.StoreCh( Ch );
                  IdOk = !isdigit( Ch ) && Ch != '_';
               }
         }
      } while( FML->ErrorFree && !done );
   }

   if( FML->FTrace >= 2 ) AddMsgStr( ": SetParam-out"s + std::to_string( N + 1 ) + ": ", Wrt.ps );
   Params.SetValue( N, Wrt.ps );
}

void TGAMSMacro::GetValue( const int N, std::string &P ) const
{
   P = Params.GetValue( N );
}

void TGAMSMacro::AddMsgStr( const std::string &s, const std::string &p ) const
{
   FML->AddMsgStr( Name + "[" + std::to_string( FML->CallList.size() ) + "] " + s, p );
}

void TGAMSMacro::WriteToStream( gmsstrm::TXStreamDelphi &S ) const
{
   S.WriteString( Name );
   Params.WriteToStream( S );
   S.WritePChar( Body.c_str(), static_cast<int>(Body.length()) );
}

void TGAMSMacro::SubstituteParams( TPReader &Rdr, TPWriterCC &Wrt ) const
{
   char Ch { ' ' };
   std::string Id, P;
   int AmpC {};
   bool idOk { true }, done {};
   while( Ch != '\0' && !done )
   {
      Ch = Rdr.NextCh();
      switch( Ch )
      {
         case '\0':
            done = true;
            break;
         case CharConcat:
            AmpC++;
            idOk = true;
            break;
         default:
            if( isalpha( Ch ) )
            {
               bool AmpF { AmpC == 2 };
               AmpC = 0;
               if( !idOk )
               {
                  Wrt.StoreCh( Ch );
                  continue;
               }

               Id.clear();
               Id += Ch;
               do {
                  Ch = Rdr.NextCh();
                  if( !utils::in( Ch, IDSecond ) )
                  {
                     if( Ch != '\0' ) Rdr.UndoNextCh();
                     const int N = Params.IndexOf( Id );
                     if( N < 0 ) Wrt.StoreStr( Id );
                     else
                     {
                        GetValue( N, P );
                        AmpF &= P.length() >= 2 && utils::in( P.front(), '\'', '\"' ) && utils::in( P.back(), '\'', '\"' );
                        if( !AmpF ) Wrt.StoreTrimStr( P );
                        else
                           Wrt.StoreStr( P.substr( 1, P.length() - 2 ) );
                     }
                     break;
                  }
                  Id += Ch;
                  if( Id.length() >= 255 )
                  {
                     Wrt.StoreStr( Id );
                     do {
                        Ch = Rdr.NextCh();
                        if( Ch == '\0' ) break;
                        Wrt.StoreCh( Ch );
                     } while( utils::in( Ch, IDSecond ) );
                     break;
                  }
               } while( true );
            }
            else
            {
               AmpC = 0;
               Wrt.StoreCh( Ch );
               idOk = !isdigit( Ch ) && Ch != '_';
            }
      }
   }
}

TPReader::TPReader( const std::string &_p ) : p( _p ), pr( _p ) {}

TPReader::~TPReader() = default;

char TPReader::NextCh()
{
   if( pr.empty() ) return '\0';
   const char c = pr.front();
   pr.erase( 0, 1 );
   off++;
   return c;
}

char TPReader::NextChNoBlank()
{
   char res;
   for( res = NextCh(); res == ' ' || res == '\t'; res = NextCh() )
      ;
   return res;
}

void TPReader::Advance( int L )
{
   assert( L <= static_cast<int>( pr.length() ) && "TPReader.Advance" );
   off += L;
   pr.erase( 0, L );
}

void TPReader::UndoNextCh()
{
   off--;
   pr = p.substr( off );
}

char TPReader::ChFromEnd( int Ofs ) const
{
   return pr[pr.length() - 1 - Ofs];
}

TPWriter::TPWriter() = default;
TPWriter::~TPWriter() = default;

void TPWriter::StoreCh( const char ch )
{
   pw += ch;
}

void TPWriter::StoreStr( const std::string &s )
{
   pw += s;
}

void TPWriter::StoreTrimStr( const std::string &s )
{
   bool BlHead { true };
   int BlCnt {};
   for( char c: s )
   {
      if( c == ' ' ) BlCnt++;
      else
      {
         if( BlHead )
         {
            BlHead = false;
            BlCnt = 0;
         }
         else
         {
            while( BlCnt > 0 )
            {
               StoreCh( ' ' );
               BlCnt--;
            }
         }
         StoreCh( c );
      }
   }
}

void TPWriter::Reset()
{
   pw.clear();
}

void TPWriter::Release( bool Z, std::string &P )
{
   //if (Z) StoreCh('\0');
   P.assign( this->pw );
   //if (Z) P.pop_back();
   pw.clear();
}

TPWriterCC::TPWriterCC() = default;

void TPWriterCC::StoreCh( char ch )
{
   if( ch != CharConcat )
      TPWriter::StoreCh( ch );
}

TParamList::TParamList() = default;

TParamList::TParamList( gmsstrm::TXStreamDelphi &S )
{
   for( int N = S.ReadInteger(); N > 0; N-- )
      push_back( S.ReadString() );
}

void TParamList::SetValue( const int N, const std::string &S )
{
   objects[( *this )[N]] = S;
}

[[nodiscard]] std::string TParamList::GetValue( const int N ) const
{
   return objects.at( ( *this )[N] );
}

void TParamList::FreeObject( const int Index )
{
   SetValue( Index, "" );
}

void TParamList::WriteToStream( gmsstrm::TXStreamDelphi &S ) const
{
   S.WriteInteger( static_cast<int>( size() ) );
   for( const auto &s: *this )
      S.WriteString( s );
}

int TParamList::IndexOf( const std::string &s ) const
{
   int i {};
   for( const auto &s1: *this )
   {
      if( utils::sameText( s, s1 ) ) return i;
      i++;
   }
   return -1;
}

}// namespace gdlib::gmacro