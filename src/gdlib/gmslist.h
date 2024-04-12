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
#include "../global/delphitypes.h"
#include "gmsgen.h"

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace gdlib::gmslist
{
constexpr int  MAX_HDR = 20,
               MAX_INDENTS = 5,
               MAX_INDXQUE = 5;

enum TPageControl
{
   pcFillPage,
   pcFortran,
   pcNothing,
   pcFormFeed
};
enum TCaseAction
{
   casNone,
   casToUpper,
   casToLower
};

enum TIndxCode
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
   std::ostream *PFile;
   gmsgen::PTextFile PFileIndx;
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

   static std::string TwoSidePadder( const std::string &S, int W, char blankChar, bool padLeft );
   void SysStrWrite( const std::string &s );
   void SysChWrite( char ch );
   void SysBlWrite( int n );

   void CalcCharsLeft();

   void WritePageHeader();

   void WriteHeaderLines( int StartLev );

public:
   bool ShowRuler{}, ShowMargins, DoubleSpace{}, DebugHeader{};

   explicit TGmsList( std::ostream *ptf = nullptr, gmsgen::PTextFile ptinx = nullptr );
   virtual ~TGmsList();

   TPageControl PageControl { pcNothing };
   TCaseAction CaseAction { casNone };

   void SetTitle( const std::string &s );
   std::string GetTitle() const;

   int GetRightMargin() const;

   void WrStrLn( const std::string &s );
   void WrStrBlock( const std::string &s );
   void WrInt( int n );
   void WrStr( const std::string &s );
   void WrStrInt( const std::string &s, int N );
   void WrDblFmt( double d, int m, int n );
   void WrIntFmt( int N, int D );
   bool ReqLines( int N );
   void ReqLinesSkip( int L, int S );
   void IndentDone();
   void PushHeader( const std::string &debugstr );
   void WriteLineHeader();
   void LWrite( bool src );

   static std::string PadLeft( const std::string &S, int W, char blankChar = ' ' );
   static std::string PadRight( const std::string &S, int W, char blankChar = ' ' );

   void WrStrFmt( const std::string &s, int w );
   void WrLn( int times = 1 );
   void WrBl( int n );
   void WrCh( char ch );

   void SetSysTitle( const std::string &Left, const std::string &Right );
   void SetSubTitle( const std::string &st );

   int GetPageNumber() const;
   int GetLineNumber() const;
   std::string GetSubTitle() const;
   std::string GetTitleLastWritten() const;
   std::string GetSubTitleLastWritten() const;
   int GetLinesOnPage() const;

   bool LineIsEmpty() const;

   void UsrWrite( const std::string &s );

   void CheckIndxQue();

   void WriteToIndex( TIndxCode ic, const std::string &t );

   void NewPage();
   void SetPageNumber( int n );

   int StrBlockLength( const std::string &s, int Indent ) const;

   int GetCharactersUsed() const;

   void SetSubTitleLastWritten( const std::string &s );

   void SetTitleLastWritten( const std::string &s );
   void SetLinesOnPage( int n );

   void SetLineNumber( int n );

   void SetRightMargin( int n );

   void SetLinesPerPage( int n );

   void SetLstTitleLeftAligned( int n );

   bool ReqCharacters( int N );

   void WrStrBrk( const std::string &s, char brkch );

   void WrPChar( const char *P, int L );

   void WrPCharLn( int LineNr, bool ShowLineNr, const char *Ps, int L, const std::string &DsErrors );

   void WrBoolFmt( bool b, int w );

   void WrStrMod( const std::string &s, int M );

   void AddToIndex( TIndxCode ic, const std::string &t );

   void HeaderStart( const std::string &debugstr );

   void HeaderDone( const std::string &debugstr );

   void HeaderShow( TIndxCode lxiCode, const std::string &ID );

   void HeaderShowLast( int FromTop );

   void HeaderDrop( const std::string &debugstr );

   void HeaderSingle( const std::string &s );

   void IndentStart();

   void SetUserLeftMargin( int n );
   void SetUserRightMargin( int n );
   int GetUserLeftMargin() const;
   int GetUserRightMargin() const;

   int GetCharactersLeft() const
   {
      return FCharsLeft;
   }

   void SetIndent( int v );

   std::string GetErrorStars() const;

   void SetErrorStars( const std::string &s );

   void SetSuppressOutput( bool v )
   {
      FsuppressOutput = v;
   }
};

}// namespace gdlib::gmslist
