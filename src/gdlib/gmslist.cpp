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
#include <cassert>

#include "rtl/sysutils_p3.h"

#include "gmslist.h"
#include "strutilx.h"

#include "utils.h"

using namespace std::literals::string_literals;

namespace gdlib::gmslist
{

constexpr int NR_BEFORE_HEADER = 1,
              NR_AFTER_HEADER = 1,
              NR_EXTRA_HEADER = 3,
              MIN_PAGELENGTH = 20,
              MIN_WIDTH_LINENR = 4;

std::array<int, inxcMessage + 1> IndxCodeLev { 0, 1, 1, 2, 2, 2, 2, 3, 2, 2 };

/*static TIndxCode indxcFindCode( const std::string &s, int &Lev )
{
   for( int c { 1 }; c <= inxcMessage; c++ )
   {
      if( s == IndxCodeText[c] )
      {
         Lev = IndxCodeLev[c];
         return static_cast<TIndxCode>( c );
      }
   }
   Lev = IndxCodeLev[0];
   return static_cast<TIndxCode>( 0 );
}*/

void TGmsList::SetTitle( std::string_view s )
{
   FTitle = s;
   FDoPageHeader = s != FTitleLastWritten;
}

std::string TGmsList::GetTitle() const { return FTitle; }

void TGmsList::WrStrLn( std::string_view s )
{
   if( FsuppressOutput ) return;
   WrStr( s );
   WrLn();
}

// Brief:
//   Write a string in one or more lines between the Indent
//   and the right margin
// Arguments:
//   S: The string to be written
// See Also:
//   StrBlockLength
void TGmsList::WrStrBlock( std::string_view s )
{
   if( FsuppressOutput ) return;
   if( s.empty() ) WrLn();
   else
   {
      // FIXME: AS: This looks slow!
      for( std::string x { s }; !x.empty(); )
      {
         int k { FCharsLeft };
         WrStrLn( x.substr( 0, k ) );
         x = k >= static_cast<int>( x.length() ) ? ""s : x.substr( k, x.length() - k );
         if( !x.empty() && x.front() == ' ' )
            utils::trimLeft(x);
         if( !x.empty() )
            WrBl( FCharsLeft - k );
      }
   }
}

void TGmsList::WrInt( int n )
{
   if( FsuppressOutput ) return;
   WrStr( rtl::sysutils_p3::IntToStr( n ) );
}

void TGmsList::WrStr( std::string_view s )
{
   if( FsuppressOutput ) return;
   if( (int) s.length() <= FCharsLeft )
   {
      UsrWrite( s );
      return;
   }

   if( !LineIsEmpty() )
   {
      LWrite( true );
      if( (int) s.length() <= FCharsLeft )
      {
         UsrWrite( s );
         return;
      }
   }

   const std::string_view &x { s };
   for( int k { FCharsLeft }; (int) x.length() > k && k > 0; k = FCharsLeft )
   {
      UsrWrite( x.substr( 1, k ) );
      LWrite( true );
   }
   if( !x.empty() )
   {
      UsrWrite( gdlib::strutilx::BlankStr( FCharsLeft - static_cast<int>(x.length()) ) );
      UsrWrite( x );
   }
}

void TGmsList::WrStrInt( std::string_view s, int N )
{
   if( FsuppressOutput ) return;
   std::string acc {s};
   acc += rtl::sysutils_p3::IntToStr(N);
   WrStr( acc );
}

// Brief:
//   Write a double in a field of M characters wide using N decimals.
//   If both M and N are zero, the double is written in the default
//   format for doubles.
// Arguments:
//   M: The width of the field
//   N: The number decimals to be used
void TGmsList::WrDblFmt( double d, int m, int n )
{
   if( FsuppressOutput ) return;
   std::ostringstream oss;

   if( !m && !n )
   {
      oss.precision( 14 );
      oss << std::scientific << d;
      std::string s = oss.str();
      if( utils::strContains( s, 'e' ) )
      {
         const auto parts = utils::split( s, 'e' );
         const std::string sign = utils::strContains( s, '-' ) ? "-"s : "+"s,
                     exp = utils::replaceSubstrs( parts.back(), sign, ""s );
         const std::string &base = parts.front();
         s = base + 'E' + sign + std::string( 4 - exp.length(), '0' ) + exp;
      }
      WrStr( s );
      return;
   }

   oss.precision( n );
   oss << d;
   WrStr( PadLeft( oss.str(), m, ' ' ) );
}

void TGmsList::WrIntFmt( int N, int D )
{
   if( FsuppressOutput ) return;
   WrStr( PadLeft( rtl::sysutils_p3::IntToStr( N ), D, ' ' ) );
}

// Brief:
//   Check if the current page can have at least N more lines.
//   If N lines do not fit, signal that the text that follows
//   should be written on a new page.
// Arguments:
//   N: The number of lines requested
// Returns:
//   True if the number of lines requested fit on the current page,
//   False otherwise.
// See Also:
//   ReqLinesSkip
bool TGmsList::ReqLines( int N )
{
   if( FsuppressOutput ) return true;
   if( N <= 0 || FDoPageHeader || FLinesOnPage + N * ( DoubleSpace ? 2 : 1 ) <= FLinesPerPage ) return true;
   FDoPageHeader = true;
   FTitleLastWritten.clear();
   return false;
}

// Brief:
//   Check if the current page can have at least L more lines.
//   If L lines do not fit, signal that the text that follows
//   should be written on a new page. If the lines do fit, skip
//   S lines.
// Arguments:
//   L: The number of lines requested
//   S: The Extra number of lines on the same page
// See Also:
//   ReqLines
void TGmsList::ReqLinesSkip( int L, int S )
{
   if( FsuppressOutput ) return;
   if( !FDoPageHeader )
   {
      if( ReqLines( L ) )
      {
         while( S > 0 )
         {
            WrLn();
            S--;
         }
      }
   }
}

// Brief:
//   Cancel the current Indent position. The current position
//   is popped from the Indent stack.
void TGmsList::IndentDone()
{
   if( FsuppressOutput ) return;
   if( FIndentSp > 0 )
   {
      FIndentColumn = FIndentStack[FIndentSp];
      FIndentSp--;
   }
}

void TGmsList::PushHeader( std::string_view debugstr )
{
   if( DebugHeader ) { debugStream << "\nPushHeader: " << debugstr << ", Lev = " << FNrHdrLines << " Sp = " << FHdrSp; }
   if( FNrHdrLines >= MAX_HDR )
   {
      FNrHdrLines = 0;
      FWrHeader = false;
      WrLn();
      throw std::runtime_error( "Number of header lines limit reached" );
   }
   FNrHdrLines++;
   FHdrStrStack[FNrHdrLines].assign( FCurrHeader );
   FCurrHeader.clear();
}

void TGmsList::CalcCharsLeft() { FCharsLeft = FRightMargin - ( FIndentColumn >= FLeftMargin ? FIndentColumn : FLeftMargin ); }

void TGmsList::LWrite( bool src )
{
   if( FWrHeader ) PushHeader( "Lwrite" );
   else
   {
      if( FDoLineHeader && src ) WriteLineHeader();
      if( PFile ) fputc( '\n', PFile );
      FFileLineNumber++;
      FLinesOnPage++;
      if( FLinesOnPage >= FLinesPerPage ) FDoPageHeader = true;
   }
   FCharsWritten = 0;
   FDoLineHeader = true;
   CalcCharsLeft();
}

std::string TGmsList::TwoSidePadder( std::string_view S, int W, char blankChar, bool padLeft )
{
   const auto nblanks = static_cast<int>(W - S.length());
   if( nblanks <= 0 ) return std::string { S };
   std::string blanks( nblanks, blankChar );
   if(padLeft) {
      blanks += S;
      return blanks;
   }
   std::string res{S};
   res += blanks;
   return res;
}

// Brief:
//  Pad a string on the right side with blanks
// Arguments:
//  S: Source string
//  W: Number of blanks to add to the right
// Returns:
//  String with additional blanks on the right
// See Also:
//  PadLeft
std::string TGmsList::PadRight( std::string_view S, int W, char blankChar ) { return TwoSidePadder( S, W, blankChar, false ); }

// Brief:
//  Pad a string on the left side with blanks
// Arguments:
//  S: Source string
//  W: Number of blanks to add to the left
// Returns:
//  String with additional blanks on the left
// See Also:
//  PadRight
std::string TGmsList::PadLeft( std::string_view S, int W, char blankChar ) { return TwoSidePadder( S, W, blankChar, true ); }

// Brief:
//   Write a string left or right aligned in a field
//   that is at least Abs(W) characters wide.
// Arguments:
//   S: The string to be written
//   W: The minimum width of the field; when w >= 0 the string will
//      right aligned, otherwise, it will be left aligned
void TGmsList::WrStrFmt( std::string_view s, int w )
{
   if( FsuppressOutput ) return;
   WrStr( w >= 0 ? PadLeft( s, w ) : PadRight( s, -w ) );
}

// Brief:
//   Signal that the text that follows should be written on a new line.
//   When an Indent setting is active, it will be replaced by the previous
//   setting (Pop the Indent Stack)
void TGmsList::WrLn( int times )
{
   if( FsuppressOutput ) return;
   IndentDone();
   LWrite( true );
   if( DoubleSpace && !FDoPageHeader && !FWrHeader ) LWrite( true );
   if( times > 1 ) WrLn( times - 1 );
}

// Brief:
//   Set the text to be displayed as the first line on every
//   page. The layout is:
//   Left     Right Page nnn
// Parameters:
//   Left: Text to be displayed on the left side of the line
//   Right: Text to be displayed on the right side of the line
//          (will be followed by 'Page' and the page number
void TGmsList::SetSysTitle( std::string_view Left, std::string_view Right )
{
   if( FsuppressOutput ) return;
   FSysTitleLeft = Left;
   FSysTitleRight = Right;
}

void TGmsList::SetSubTitle( std::string_view st )
{
   FSubTitle = st;
   FAddSubTitleToIndex = true;
   if( FTitle.empty() ) FTitle = "Sub-title without a title";
   if( st != FSubTitleLastWritten ) FDoPageHeader = true;
}

int TGmsList::GetPageNumber() const { return FPageNumber + ( FDoPageHeader ? 1 : 0 ); }

int TGmsList::GetLineNumber() const { return FFileLineNumber + ( LineIsEmpty() ? 1 : 0 ); }

std::string TGmsList::GetSubTitle() const { return FSubTitle; }

std::string TGmsList::GetTitleLastWritten() const { return FTitleLastWritten; }

std::string TGmsList::GetSubTitleLastWritten() const { return FSubTitleLastWritten; }

int TGmsList::GetLinesOnPage() const { return FLinesOnPage; }

bool TGmsList::LineIsEmpty() const
{
   if( FsuppressOutput ) return true;
   return FWrHeader ? FCurrHeader.empty() : !FCharsWritten;
}

void TGmsList::UsrWrite( std::string_view s )
{
   if( FWrHeader ) FCurrHeader += s;
   else
   {
      if( FDoLineHeader ) WriteLineHeader();
      CheckIndxQue();
      this->SysStrWrite( s );
   }
   FCharsLeft -= static_cast<int>( s.length() );
}

void TGmsList::CheckIndxQue()
{
   global::delphitypes::Bounded<int, 1, MAX_INDXQUE> N;
   if( FNRIndxQue >= 1 )
   {
      for( N = 1; N <= FNRIndxQue; ++N ) this->WriteToIndex( FIndxQue[N].QCode, FIndxQue[N].QTit );
      FNRIndxQue = 0;
   }
}

void TGmsList::SysStrWrite( std::string_view s )
{
   if( !PFile ) return;
   utils::fputstr( PFile, ( CaseAction == casToUpper ? strutilx::UpperCase( s ) : ( CaseAction == casToLower ? strutilx::LowerCase( s ) : s ) ) );
   FCharsWritten += static_cast<int>( s.length() );
}

void TGmsList::SysStrWrite( const char *s, size_t slen )
{
   if( !PFile ) return;
   fwrite( s, sizeof( char ), slen, PFile );
   FCharsWritten += static_cast<int>(slen);
}

void TGmsList::SysChWrite( const char ch )
{
   if( !PFile ) return;
   const char c { CaseAction == casToUpper ? (char) utils::toupper( ch ) : ( CaseAction == casToLower ? (char) utils::tolower( ch ) : ch ) };
   fputc( c, PFile );
   FCharsWritten++;
}

void TGmsList::SysBlWrite( int n )
{
   if( !PFile ) return;
   if( n > 0 )
   {
      fwrite( blanks.data(), sizeof( char ), n, PFile );
      FCharsWritten += n;
   }
}

void TGmsList::WriteToIndex( TIndxCode ic, std::string_view t )
{
   if( !PFileIndx ) return;
   ( *PFileIndx ) << 'a' + ic << " " << FFileLineNumber << " " << t;
}

TGmsList::TGmsList( FILE *ptf, std::fstream *ptinx )
    : PFile { ptf },
      PFileIndx { ptinx },
      Fstars { "****"s }
{
   std::memset( blanks.data(), ' ', sizeof( char ) * blanks.size()-1 );
   blanks.back() = '\0';
   CalcCharsLeft();
   FCharsLeft = 0;
   ShowMargins = false;
   FnextLxiCode = inxcNone;
   FnextID.clear();
}

TGmsList::~TGmsList()
{
   if( PageControl == pcFillPage && FLinesOnPage > 0 && !FWrHeader )
   {
      FDoPageHeader = false;
      while( FLinesOnPage < FLinesPerPage ) LWrite( true );
   }
   else if( FCharsWritten > 0 )
      LWrite( true );

   for( int n = MAX_HDR - 1; n >= 0; n-- )
      if( !FHdrStrStack[n].empty() ) FHdrStrStack[n].clear();
   FCurrHeader.clear();
}

void TGmsList::WriteLineHeader()
{
   if( !FDoLineHeader ) throw std::runtime_error( "No line start" );
   if( FWrHeader ) throw std::runtime_error( "No header here" );
   if( FDoPageHeader ) WritePageHeader();
   FDoLineHeader = false;
   int n { FIndentColumn >= FLeftMargin ? FIndentColumn : FLeftMargin };
   if( PageControl == pcFortran ) n++;
   if( n > 0 ) UsrWrite( strutilx::BlankStr( n ) );
   CalcCharsLeft();
}

void TGmsList::WritePageHeader()
{
   if( !FDoPageHeader ) throw std::runtime_error( "No Page pending" );

   if( FLinesOnPage < FLinesPerPage && FTitle == FTitleLastWritten && FSubTitle == FSubTitleLastWritten )
   {
      FDoPageHeader = false;
      if( FNrHdrLines > 0 ) WriteHeaderLines( 1 );
      return;
   }

   if( PageControl == pcFillPage && FLinesOnPage > 0 && !FWrHeader )
   {
      FDoPageHeader = false;
      while( FLinesOnPage < FLinesPerPage ) LWrite( true );
   }

   FDoPageHeader = false;
   FPageNumber++;
   FLinesOnPage = 0;
   if( ShowRuler )
   {
      int L { FRightMargin };
      int N { 1 };
      while( L >= 10 )
      {
         std::string s = PadLeft( rtl::sysutils_p3::IntToStr( N ), 10 );
         N++;
         SysStrWrite( s );
         L -= 10;
      }
      LWrite( false );
      std::string s { "1234567890" };
      L = FRightMargin;
      while( L >= 10 )
      {
         SysStrWrite( s );
         L -= 10;
      }
      if( L > 0 ) SysStrWrite( s.substr( 0, L ) );
      LWrite( false );
   }

   std::string s { " Page " + rtl::sysutils_p3::IntToStr( FPageNumber ) };
   int L { static_cast<int>( FRightMargin - ( FSysTitleLeft.length() + FSysTitleRight.length() + s.length() ) ) };
   if( PageControl == pcFormFeed ) SysChWrite( '\f' );
   else if( PageControl == pcFortran )
      SysChWrite( '1' );
   SysStrWrite( FSysTitleLeft );
   if( 1 == FLstTitleLeftAligned ) SysStrWrite( "- " );
   else
      SysBlWrite( L );
   SysStrWrite( FSysTitleRight );
   SysStrWrite( s );
   LWrite( false );
   if( !FTitle.empty() )
   {
      if( PageControl == pcFortran ) SysChWrite( ' ' );
      SysStrWrite( FTitle );
      FTitleLastWritten = FTitle;
      LWrite( false );
      if( !FSubTitle.empty() )
      {
         if( PageControl == pcFortran ) SysChWrite( ' ' );
         SysStrWrite( FSubTitle );
         FSubTitleLastWritten = FSubTitle;
         LWrite( false );
         if( FAddSubTitleToIndex )
         {
            FAddSubTitleToIndex = false;
            WriteToIndex( inxcSubTitle, FSubTitle );
         }
      }
   }
   CheckIndxQue();
   LWrite( false );
   LWrite( false );
   if( FNrHdrLines > 0 ) WriteHeaderLines( 1 );
}

void TGmsList::WriteHeaderLines( int StartLev )
{
   for( int N { FHdrStack[StartLev] + 1 }; N <= FNrHdrLines; N++ )
   {
      if( PageControl == pcFortran ) SysChWrite( ' ' );
      auto &obj = FHdrStrStack[N];
      for( int K { 1 }; K <= (int) obj.length(); K++ ) SysChWrite( obj[K - 1] );
      if( obj.length() >= 5 && obj[0] == '-' && utils::starts_with( obj, "--- " ) ) { obj.replace( 0, 4, std::string( 4, ' ' ) ); }
      LWrite( true );
      if( N == FHdrStack[StartLev] + 1 && FnextLxiCode != inxcNone )
      {
         this->AddToIndex( FnextLxiCode, FnextID );
         FnextLxiCode = inxcNone;
         FnextID.clear();
      }
   }

   for( int N { 1 }; N <= NR_AFTER_HEADER; N++ ) LWrite( true );
}

// Brief:
//   Signal that the text that follows should be written on a new page.
//   Ignored when writing to a header
void TGmsList::NewPage()
{
   if( FsuppressOutput ) return;
   if( !FWrHeader )
   {
      if( !LineIsEmpty() ) LWrite( true );
      FDoPageHeader = true;
      FTitleLastWritten.clear();// force
   }
}

int TGmsList::StrBlockLength( std::string_view s, int Indent ) const
{
   if( FsuppressOutput ) return 0;
   return static_cast<int>( s.length() ) / ( FRightMargin - Indent + 1 ) + 1;
}

void TGmsList::WrBl( int n )
{
   if( FsuppressOutput ) return;
   for( ; n > 0; n-- ) WrCh( ' ' );
}

// Brief:
//   Write a character
// Arguments:
//   Ch: The character to be written
void TGmsList::WrCh( char ch )
{
   if( FsuppressOutput ) return;
   if( FCharsLeft <= 0 ) LWrite( true );
   UsrWrite( std::string( 1, ch ) );
}

void TGmsList::SetLinesOnPage( int n )
{
   FLinesOnPage = n;
   if( FLinesOnPage >= FLinesPerPage ) FDoPageHeader = true;
}

void TGmsList::SetTitleLastWritten( std::string_view s )
{
   FTitleLastWritten = s;
   FSubTitleLastWritten.clear();
}

void TGmsList::SetSubTitleLastWritten( std::string_view s ) { FSubTitleLastWritten = s; }

int TGmsList::GetCharactersUsed() const { return FRightMargin - FCharsLeft; }

void TGmsList::SetPageNumber( int n )
{
   if( n > this->GetPageNumber() )
   {
      FPageNumber = n;
      FDoPageHeader = true;
   }
}

void TGmsList::IndentStart()
{
   if( FsuppressOutput ) return;
   assert( FIndentSp < MAX_INDENTS && "Too many indents!" );
   FIndentStack[++FIndentSp] = FIndentColumn;
   int n { FCharsLeft };
   FIndentColumn = FLeftMargin;
   do
   {
      FIndentColumn++;
      CalcCharsLeft();
   } while( FCharsLeft != n );
}

void TGmsList::SetUserLeftMargin( int n )
{
   if( n > 0 ) FUserLeftMargin = n - 1;
}

void TGmsList::SetUserRightMargin( int n )
{
   if( n > 0 ) FUserRightMargin = n - 1;
}

int TGmsList::GetUserLeftMargin() const { return FUserLeftMargin + 1; }

int TGmsList::GetUserRightMargin() const { return FUserRightMargin + 1; }

void TGmsList::HeaderSingle( std::string_view s )
{
   if( FsuppressOutput ) return;
   HeaderStart( "HeaderSingle" );
   WrStrLn( s );
   HeaderDone( "HeaderSingle" );
   HeaderShow( inxcNone, "" );
}

void TGmsList::HeaderDrop( std::string_view debugstr )
{
   if( FsuppressOutput ) return;
   if( DebugHeader ) { debugStream << "\nHeaderDrop: " << debugstr << " Lev = " << FNrHdrLines << " Sp = " << FHdrSp << '\n'; }
   HeaderDone( "HeaderDrop" );
   if( FHdrSp > 0 )
   {
      FNrHdrLines = FHdrStack[FHdrSp];
      FHdrSp--;
   }
   if( DebugHeader ) { debugStream << "\nHeaderDrop_Finished: " << debugstr << " Lev = " << FNrHdrLines << " Sp = " << FHdrSp; }
}

void TGmsList::HeaderShowLast( int FromTop )
{
   if( FsuppressOutput ) return;
   HeaderDone( "HeaderShowLast" );
   if( !FDoPageHeader && FNrHdrLines > 0 )
   {
      int StartLev = FHdrSp - FromTop + 1;
      int NrLines = FNrHdrLines - FHdrStack[StartLev];
      if( ReqLines( NrLines + NR_AFTER_HEADER + NR_EXTRA_HEADER ) ) WriteHeaderLines( StartLev );
   }
}

void TGmsList::HeaderShow( TIndxCode lxiCode, std::string_view ID )
{
   if( FsuppressOutput ) return;
   HeaderDone( "HeaderShow" );
   FnextLxiCode = lxiCode;
   FnextID = ID;
   if( !FDoPageHeader && FNrHdrLines > 0 && ReqLines( FNrHdrLines + NR_BEFORE_HEADER + NR_AFTER_HEADER + NR_EXTRA_HEADER ) )
   {
      for( int N = 1; N <= NR_BEFORE_HEADER; N++ ) LWrite( true );
      WriteHeaderLines( 1 );
   }
}

void TGmsList::HeaderDone( std::string_view debugstr )
{
   if( FsuppressOutput ) return;
   if( DebugHeader ) { debugStream << "\nHeaderDone: " << debugstr << " Lev = " << FNrHdrLines << " Sp = " << FHdrSp << '\n'; }
   if( FWrHeader )
   {
      if( !LineIsEmpty() ) PushHeader( "HeaderDone" );
      FWrHeader = false;
   }
}

void TGmsList::HeaderStart( std::string_view debugstr )
{
   if( FsuppressOutput ) return;
   if( DebugHeader ) { debugStream << "\nHeaderStart: " << debugstr << " Lev = " << FNrHdrLines << " Sp = " << FHdrSp << " " << ( FWrHeader ? 1 : 0 ); }

   if( !LineIsEmpty() ) WrLn();

   if( FWrHeader ) HeaderDone( "HeaderStart" );

   assert( FHdrSp < MAX_HDR && "Header stack capacity exceeded" );
   FHdrSp++;
   FHdrStack[FHdrSp] = FNrHdrLines;
   FCurrHeader.clear();
   FWrHeader = true;
}

void TGmsList::AddToIndex( TIndxCode ic, std::string_view t )
{
   if( FsuppressOutput ) return;
   if( PFileIndx )
   {
      if( !FDoPageHeader ) WriteToIndex( ic, t );
      else
      {
         if( FNRIndxQue < MAX_INDXQUE ) FNRIndxQue = FNRIndxQue + 1;
         auto &obj = FIndxQue[FNRIndxQue];
         obj.QCode = ic;
         obj.QTit = t;
      }
   }
}

void TGmsList::WrStrMod( std::string_view s, int M )
{
   if( FsuppressOutput ) return;
   WrStr( gdlib::strutilx::PadRightMod( s, M ) );
}

void TGmsList::WrBoolFmt( bool b, int w )
{
   if( FsuppressOutput ) return;
   std::string s;
   if( w < 5 ) { s = b ? "T" : "F"; }
   else { s = b ? "TRUE" : "FALSE"; }
   WrStr( PadLeft( s, w ) );
}

void TGmsList::WrPChar( const char *P, int L )
{
   if( FsuppressOutput ) return;
   if( L <= 0 ) return;
   const char *pp = P;
   int w {};
   std::string s;
   for( int k = 1; k <= L; k++ )
   {
      w++;
      s += *pp;
      pp++;
      if( w == 255 )
      {
         s[0] = static_cast<char>( w );
         WrStr( s );
         w = 0;
      }
   }
   if( w > 0 )
   {
      s[0] = static_cast<char>( w );
      WrStr( s );
   }
}

void TGmsList::WrPCharLn( const int LineNr, const bool ShowLineNr, const char *Ps, const int L, const std::string &DsErrors )
{
   auto WritePartialLine = [&]( const char *Str, const int Start, const int Length, int &NextPos ) {
      int p { Start };
      const auto Corr { PageControl == pcFortran ? 1 : 0 };
      static std::array<char, 80003> buf;
      int i {};
      do
      {
         const int k { ShowMargins && ( p == FUserLeftMargin || p == FUserRightMargin ) ? 2 : 1 };
         if( FCharsWritten - Corr + k > FRightMargin ) break;
         if( k == 1 )
         {
            //SysChWrite( Str[p] );
            buf[i++] = Str[p];
         }
         else
         {
            if( p == FUserLeftMargin )
            {
               //SysChWrite( '|' );
               buf[i++] = '|';
            }

            //SysChWrite( Str[p] );
            buf[i++] = Str[p];

            if( p == FUserRightMargin )
            {
               //SysChWrite( '|' );
               buf[i++] = '|';
            }
         }
         p++;
      } while( p <= Length );
      SysStrWrite( buf.data(), i );
      LWrite( true );
      NextPos = p;
   };

   if( FsuppressOutput ) return;
   int wnr { strutilx::IntegerWidth( LineNr ) };
   if( wnr < MIN_WIDTH_LINENR ) wnr = MIN_WIDTH_LINENR;
   int p {}, NxtErrs {};
   bool StarsDone {};
   do
   {
      if( FDoPageHeader ) WritePageHeader();
      if( PageControl == pcFortran ) SysChWrite( ' ' );
      if( ShowLineNr && !p ) SysStrWrite( utils::IntToStrW( LineNr, wnr ) );
      else
         SysBlWrite( wnr );
      const int NrBl = ShowMargins ? 1 : 2;
      SysBlWrite( NrBl );
      int NxtSrc;
      WritePartialLine( Ps, p, L, NxtSrc );
      if( !DsErrors.empty() && p < static_cast<int>( DsErrors.length() ) )
      {
         bool needed = false;
         for( int k = p; k < NxtSrc; k++ )
         {
            if( static_cast<int>( DsErrors.length() ) > k )
            {
               needed = true;
               break;
            }
         }

         if( !needed ) NxtErrs = NxtSrc;
         else if( StarsDone )
            SysBlWrite( wnr + NrBl );
         else
         {
            SysStrWrite( Fstars );
            SysBlWrite( wnr + NrBl - 4 );
            StarsDone = true;
         }
         WritePartialLine( DsErrors.c_str(), p, static_cast<int>(DsErrors.length()) - 1, NxtErrs );
      }
      p = NxtSrc;
   } while( p <= L );

   if( !DsErrors.empty() )
   {
      p = NxtErrs;
      while( p < static_cast<int>( DsErrors.length() ) )
      {
         WritePartialLine( DsErrors.c_str(), p, static_cast<int>(DsErrors.length()) - 1, NxtErrs );
         p = NxtErrs;
      }
   }
}

void TGmsList::WrStrBrk( std::string_view s, char brkch )
{
   if( FsuppressOutput ) return;
   std::string x { s };
   while( !x.empty() )
   {
      int k { static_cast<int>( x.length() ) };
      int M { FCharsLeft };
      if( k <= M )
      {
         UsrWrite( x );
         return;
      }
      do
      {
         while( k >= 1 && x[k] != brkch ) k--;
         if( !k ) break;
         if( k <= M && x[k] == brkch ) break;
         k--;
      } while( k );

      if( k > 0 && k <= M )
      {
         UsrWrite( x.substr( 0, k ) );
         x = x.substr( k, x.length() - k );
         continue;
      }

      k = 0;
      while( !x.empty() && x[1] == ' ' )
      {
         x = x.substr( 1, x.length() - 1 );
         k++;
      }
      if( k > 0 ) continue;
      if( !LineIsEmpty() )
      {
         LWrite( true );
         if( (int) x.length() <= FCharsLeft )
         {
            UsrWrite( x );
            return;
         }
      }
      WrStr( x );
      return;
   }
}

void TGmsList::SetLstTitleLeftAligned( int n ) { FLstTitleLeftAligned = n; }

void TGmsList::SetLineNumber( int n ) { FFileLineNumber = n; }

void TGmsList::SetLinesPerPage( int n )
{
   if( n <= 0 ) n = std::numeric_limits<int>::max();
   else if( n < MIN_PAGELENGTH )
      n = MIN_PAGELENGTH;
   if( PageControl == pcFillPage && n > 200 ) n = 200;
   FLinesPerPage = n;
   if( FLinesOnPage >= FLinesPerPage ) FDoPageHeader = true;
}

void TGmsList::SetRightMargin( int n )
{
   if( FLeftMargin + 20 < n )
   {
      FRightMargin = n;
      CalcCharsLeft();
      if( FCharsLeft < 0 ) LWrite( true );
   }
}

bool TGmsList::ReqCharacters( int N )
{
   if( FsuppressOutput ) return true;
   if( N > FCharsLeft )
   {
      LWrite( true );
      if( DoubleSpace && !FDoPageHeader && !FWrHeader ) LWrite( true );
      return true;
   }
   return false;
}

void TGmsList::SetIndent( int v )
{
   if( v > 1 )
   {
      FIndentSp++;
      FIndentStack[FIndentSp] = FIndentColumn;
      FIndentColumn = v - 1;
   }
}

std::string TGmsList::GetErrorStars() const { return Fstars; }

void TGmsList::SetErrorStars( std::string_view s ) { Fstars = s; }

int TGmsList::GetRightMargin() const { return FRightMargin; }

}// namespace gdlib::gmslist
