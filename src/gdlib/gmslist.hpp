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
#include "../global/delphitypes.hpp"
#include "gmsgen.hpp"

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace gdlib::gmslist
{
constexpr int  MAX_HDR = 20,
               MAX_INDENTS = 5,
               MAX_INDXQUE = 5;

enum TPageControl : uint8_t
{
   pcFillPage,
   pcFortran,
   pcNothing,
   pcFormFeed
};
enum TCaseAction : uint8_t
{
   casNone,
   casToUpper,
   casToLower
};

enum TIndxCode : uint8_t
{
   inxcNone,
   inxcSubTitle,
   inxcSolveSum,
   inxcEquation,
   inxcColumn,
   inxcSolEQU,
   inxcSolVAR,
   inxcSolution,
   inxcDisplay,
   inxcMessage
};

struct TRINDXQUE {
   TIndxCode QCode {};
   std::string QTit;
};

const std::array<std::string, 10> IndxCodeText = { "???", "SubTitle", "Solve Summary", "Equation", "Column", "SolEQU", "SolVAR", "Solution", "Display", "Message" };

class TGmsList final
{
   std::array<char, 256> blanks;

   int FCharsLeft,
       FCharsWritten {},
       FFileLineNumber{1},
       FLinesOnPage{},
       FLinesPerPage{std::numeric_limits<int>::max()},
       FRightMargin{132}, // gams default
       FLeftMargin{},
       FPageNumber{},
       FLstTitleLeftAligned{1};
   std::string FTitle, FSubTitle, FTitleLastWritten{}, FSubTitleLastWritten{};
   FILE *PFile;
   std::fstream *PFileIndx;
   bool FDoPageHeader{}, FDoLineHeader {true}, FAddSubTitleToIndex {}, FWrHeader {};
   std::string FCurrHeader{};
   std::array<std::string, MAX_HDR> FHdrStrStack;
   std::array<int, MAX_HDR + 1> FHdrStack{};
   int FNrHdrLines{}, FHdrSp{}, FIndentColumn{};
   std::array<int, MAX_INDENTS + 1> FIndentStack {};
   int FIndentSp {}, FUserLeftMargin{-1}, FUserRightMargin{std::numeric_limits<int>::max()};
   std::string FSysTitleLeft{}, FSysTitleRight{};
   std::array<TRINDXQUE, MAX_INDXQUE> FIndxQue;
   std::string Fstars;
   global::delphitypes::Bounded<int, 0, MAX_INDXQUE> FNRIndxQue {};
   TIndxCode FnextLxiCode{inxcNone};
   std::string FnextID;
   bool FsuppressOutput {};

   static std::string TwoSidePadder( std::string_view S, int W, char blankChar, bool padLeft );
   void SysStrWrite( std::string_view s );
   void SysStrWrite( const char *s, size_t slen );
   void SysChWrite( char ch );
   void SysBlWrite( int n );

   void CalcCharsLeft();

   void WritePageHeader();

   void WriteHeaderLines( int StartLev );

public:
   bool ShowRuler{}, ShowMargins, DoubleSpace{}, DebugHeader{};

   explicit TGmsList( FILE *ptf = nullptr, std::fstream *ptinx = nullptr );
   virtual ~TGmsList();

   TPageControl PageControl { pcNothing };
   TCaseAction CaseAction { casNone };

   void SetTitle( std::string_view s );
   [[nodiscard]] std::string GetTitle() const;

   [[nodiscard]] int GetRightMargin() const;

   void WrStrLn( std::string_view s );
   void WrStrLn( const char c )
   {
      WrStrLn(std::string_view{&c, 1});
   }
   void WrStrBlock( std::string_view s );
   void WrInt( int n );
   void WrStr( std::string_view s );

   void WrStr(char s) {
      WrStr( std::string_view { &s, 1 } );
   }

   void WrStrInt( std::string_view s, int N );
   void WrDblFmt( double d, int m, int n );
   void WrIntFmt( int N, int D );
   bool ReqLines( int N );
   void ReqLinesSkip( int L, int S );
   void IndentDone();
   void PushHeader( std::string_view debugstr );
   void WriteLineHeader();
   void LWrite( bool src );

   static std::string PadLeft( std::string_view S, int W, char blankChar = ' ' );
   static std::string PadRight( std::string_view S, int W, char blankChar = ' ' );

   void WrStrFmt( std::string_view s, int w );
   void WrLn( int times = 1 );
   void WrBl( int n );
   void WrCh( char ch );

   void SetSysTitle( std::string_view Left, std::string_view Right );
   void SetSubTitle( std::string_view st );

   [[nodiscard]] int GetPageNumber() const;
   [[nodiscard]] int GetLineNumber() const;
   [[nodiscard]] std::string GetSubTitle() const;
   [[nodiscard]] std::string GetTitleLastWritten() const;
   [[nodiscard]] std::string GetSubTitleLastWritten() const;
   [[nodiscard]] int GetLinesOnPage() const;
   [[nodiscard]] int GetLinesPerPage() const { return FLinesPerPage; }

   [[nodiscard]] bool LineIsEmpty() const;

   void UsrWrite( std::string_view s );

   void CheckIndxQue();

   void WriteToIndex( TIndxCode ic, std::string_view t );

   void NewPage();
   void SetPageNumber( int n );

   [[nodiscard]] int StrBlockLength( std::string_view s, int Indent ) const;

   [[nodiscard]] int GetCharactersUsed() const;

   void SetSubTitleLastWritten( std::string_view s );

   void SetTitleLastWritten( std::string_view s );
   void SetLinesOnPage( int n );

   void SetLineNumber( int n );

   void SetLeftMargin( int n );
   void SetRightMargin( int n );

   void SetLinesPerPage( int n );

   void SetLstTitleLeftAligned( int n );

   bool ReqCharacters( int N );

   void WrStrBrk( std::string_view s, char brkch );

   void WrPChar( const char *P, int L );

   void WrPCharLn( int LineNr, bool ShowLineNr, const char *Ps, int L, const std::string &DsErrors );

   void WrBoolFmt( bool b, int w );

   void WrStrMod( std::string_view s, int M );

   void AddToIndex( TIndxCode ic, std::string_view t );

   void HeaderStart( std::string_view debugstr );

   void HeaderDone( std::string_view debugstr );

   void HeaderShow( TIndxCode lxiCode, std::string_view ID );

   void HeaderShowLast( int FromTop );

   void HeaderDrop( std::string_view debugstr );

   void HeaderSingle( std::string_view s );

   void IndentStart();

   void SetUserLeftMargin( int n );
   void SetUserRightMargin( int n );
   [[nodiscard]] int GetUserLeftMargin() const;
   [[nodiscard]] int GetUserRightMargin() const;

   [[nodiscard]] int GetCharactersLeft() const
   {
      return FCharsLeft;
   }

   void SetIndent( int v );

   [[nodiscard]] std::string GetErrorStars() const;

   void SetErrorStars( std::string_view s );

   void SetSuppressOutput( bool v )
   {
      FsuppressOutput = v;
   }
};

}// namespace gdlib::gmslist
