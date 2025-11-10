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

#include "strutilx.hpp"

#include <algorithm>             // for min, transform, find
#include <array>                 // for array
#include <cassert>               // for assert
#include <cmath>                 // for isinf, isnan, modf, trunc
#include <cstring>               // for memcpy, strlen, memmove, size_t
#include <limits>                // for numeric_limits
#include <stdexcept>             // for runtime_error
#include <string>                // for basic_string, string, operator+, all...

#include "../rtl/p3io.hpp"         // for P3_Str_dd0
#include "../rtl/p3platform.hpp"   // for OSFileType, tOSFileType
#include "../rtl/sysutils_p3.hpp"  // for LastDelimiter, PathDelim, ExtractSho...
#include "../rtl/system_p3.hpp"

#include "utils.hpp"               // for toupper, sameText, ord, in, val, cha...

using namespace std::literals::string_literals;
using namespace rtl::sysutils_p3;
using namespace rtl::p3platform;
using namespace utils;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace gdlib::strutilx
{

const auto MAXINT_S = "maxint"s, MININT_S = "minint"s;
const auto MAXDOUBLE_S = "maxdouble"s, EPSDOUBLE_S = "eps"s, MINDOUBLE_S = "mindouble"s;

bool sameTextSR( const DelphiStrRef &sr, const std::string &s )
{
   if( s.length() != sr.length ) return false;
   for( int i {}; i < sr.length; i++ )
      if( tolower( sr.chars[i] ) != tolower(s[i]) )
         return false;
   return true;
}

std::string UpperCase( const std::string_view s )
{
   std::string out { s };
   std::transform( s.begin(), s.end(), out.begin(), ::toupper );
   return out;
}

std::string LowerCase( const std::string_view s )
{
   std::string out { s };
   std::transform( s.begin(), s.end(), out.begin(), ::tolower );
   return out;
}

// Brief:
//  Convert an integer to a string with leading blanks and thousands separators
// Arguments:
//  N: The number to be converted
//  Width: Minimum total width of result
// Returns:
//  The converted number as a string
std::string IntToNiceStrW( int64_t n, int width )
{
   // With two's complement representation of integers, we have one more negative
   // integer than positive. So reflect positive values to negative.
   const bool neg {n < 0};
   if(n > 0) n = -n;
   constexpr auto maxShortStrLen {255};
   uint8_t k {maxShortStrLen-1}, k2 {};
   // Fill s with digits from the right starting with least significant one
   // Prefix is garbage
   sstring s;
   s.back() = '\0';
   do
   {
      s[k--] = static_cast<char>(ord('0') - n % 10);
      n /= 10;
      if(++k2 == 3)
      {
         if(n)
            s[k--] = ',';
         k2 = 0;
      }
   } while(n);
   if(neg)
      s[k--] = '-';
   k++;
   // limit to short string length of 255 chars
   if(width > maxShortStrLen)
      width = maxShortStrLen;
   const int ndigits {maxShortStrLen-k}; // can include sign and commas
   // no blanks
   if(ndigits >= width) return &s[k];
   // fill to target width with blanks on the left
   std::string res(width, ' ');
   std::memcpy(&res[width-ndigits], &s[k], ndigits);
   return res;
}

std::string IntToNiceStr( int N )
{
   return IntToNiceStrW( N, 0 );
}

std::string BlankStr( unsigned int Len )
{
   return std::string( Len, ' ' );
}

int StrExcelCol( const std::string &s )
{
   int res {};
   for( int i {}; i < static_cast<int>( s.length() ); i++ )
   {
      const int j { ord(toupper( s[i] )) - ord( 'A' ) };
      if( j < 0 || j > 25 || res >= std::numeric_limits<int>::max() / 26 + 26 )
         return 0;
      res = res * 26 + j + 1;
   }
   return res;
}

std::string ExcelColStr( int C )
{
   if( C <= 0 ) return {};
   std::string res;
   for( res.clear(); C; C /= 26 )
      res += static_cast<char>( ord( 'A' ) + --C % 26 );
   return res;
}

int IntegerWidth( int n )
{
   int res = n >= 0 ? 0 : 1;
   if( res ) n = -n;
   do {
      res++;
      n /= 10;
   } while( n );
   return res;
}

int PadModLength( std::string_view s, const int M )
{
   int res { static_cast<int>( s.length() ) };
   if( M > 0 && res % M != 0 ) res += M - res % M;
   return res;
}

std::string PadRightMod( std::string_view s, const int M )
{
   std::string res{s};
   res += BlankStr( PadModLength( s, M ) - static_cast<int>(s.length()) );
   return res;
}

// Brief:
//  Search for a character from the left from a starting position
// Arguments:
//  Ch: Character to search
//  S: String to be searched
//  Sp: Starting position
// Returns:
//  Location of the character when found; -1 otherwise
int LChPosSp( const char Ch, const char *S, int Sp )
{
   if( Sp < 0 ) Sp = 0;
   for( int K { Sp }; S[K]; K++ )
      if( S[K] == Ch ) return K;
   return -1;
}

int LChPos( const char Ch, const char *S )
{
   return LChPosSp( Ch, S, 0 );
}

// Brief:
//  Search for a set of characters from the left
// Arguments:
//  Cs: Character set to search
//  S: String to be searched
// Returns:
//  Location of the character when found; -1 otherwise
int LChSetPos( const char *Cs, const char *S, const int slen )
{
   const char *c { Cs };
   for( int k { 0 }; k <= slen - 1; k++ )
   {
      while( *c )
         if( *c++ == S[k] ) return k;
      c = Cs;
   }
   return -1;
}

// Brief:
//  Search for a set of characters from the right
// Arguments:
//  Cs: Character set to search
//  S: String to be searched
// Returns:
//  Location of the character when found; -1 otherwise
int RChSetPos( const char *Cs, const char *S, const int slen )
{
   const char *c { Cs };
   for( int k { slen - 1 }; k >= 0; k-- )
   {
      while( *c )
         if( *c++ == S[k] ) return k;
      c = Cs;
   }
   return -1;
}

char gsgetchar( const std::string &s, int p )
{
   return p > 0 && p <= static_cast<int>( s.length() ) ? s[p - 1] : static_cast<char>( 0 );
}

static uint8_t DblToStrSepCore(double V, const char DecimalSep, char *s)
{
   size_t eLen {};
   rtl::p3io::P3_Str_dd0( V, s, 255, &eLen );
   // output string has E notation (https://en.wikipedia.org/wiki/Scientific_notation#E_notation)
   // example: 2.30000000000000E+0001 for 23
   const auto slen = static_cast<int>( std::strlen( s ) );
   if( V < 0.0 )
      V = -V;
   const auto k { RChSetPos( "+-", s, slen ) },
           j { LChPos( '.', s ) };
   assert(k > -1); // exponent should always have sign
   assert(j > -1);
   if( V >= 1e-4 && V < 1e15 )
   {
      int e, scrap;
      val( &s[k], 5, e, scrap );
      for( int i = k - 1; i < slen; i++ )
         s[i] = '0';
      if( e >= 0 )
      {
         for( int i = j + 1; i <= j + e; i++ )
            s[i - 1] = s[i];
         s[j + e] = DecimalSep;
         for( int i = slen - 1; i >= j + e + 1; i-- )
         {
            if( s[i] == '0' )
            {
               s[i] = ' ';
               if( i == j + e + 1 )
                  s[j + e] = ' ';
            }
            else
               break;
         }
      }
      else
      {
         s[j] = s[j - 1];
         s[j - 1] = '0';
         e = -e;
         for( int i = k - 2; i >= j; i-- )
            s[i + e] = s[i];
         for( int i = j + 1; i <= j + e - 1; i++ )
            s[i] = '0';
         s[j] = DecimalSep;
         for( int i = slen - 1; i >= j + e + 1; i-- )
         {
            if( s[i] == '0' )
               s[i] = ' ';
            else
               break;
         }
      }
   }
   else
   {
      assert(k >= 0);
      if( s[k] == '+' )
         s[k] = ' ';
      for( int i = k + 1; i < slen; i++ )
      {
         if( s[i] == '0' )
         {
            s[i] = ' ';
            if( i == static_cast<int>( slen ) )
               s[k - 1] = ' ';
         }
         else break;
      }
      for( int i = k - 2; i >= j + 1; i-- )
      {
         if( s[i] == '0' )
         {
            s[i] = ' ';
            if( i == j + 1 )
               s[j] = ' ';
         }
         else
            break;
      }
   }
   return ui8(slen);
}

// Closer port of corresponding Delphi function (faster?)
// Brief:
//   Convert a double to its string representation
//   using the fullest precision.
// Parameters:
//   V: Value to be converted to a string
// Returns:
//   String representation of V
std::string DblToStrSep( double V, const char DecimalSep )
{
   if( V == 0.0 )
      return "0"s;
   sstring s;
   const auto slen { DblToStrSepCore( V, DecimalSep, s.data() ) };
   // only with short strings
   std::string res;
   res.reserve( slen );
   for(int i{}; i<slen; i++)
      if(s[i] != ' ') res += s[i];
   return res;
}

uint8_t DblToStrSep(double V, const char DecimalSep, char* sout)
{
   if (V == 0.0) {
      sout[0] = '0';
      sout[1] = '\0';
      return 1;
   }
   uint8_t slen = DblToStrSepCore( V, DecimalSep, sout );
   // only with short strings
   int i {};
   for( int l {}; l < slen; i++, l++ )
   {
      if( sout[l] == ' ' )
      {
         while( sout[++l] == ' ' && sout[l] != '\0' )
            ;
      }
      sout[i] = sout[l];
   }
   return ui8(i - 1);
}

std::string DblToStr( const double V )
{
   return DblToStrSep( V, '.' );
}

uint8_t DblToStr(double V, char* s)
{
   return DblToStrSep( V, '.', s );
}

bool StrAsIntEx( const std::string &s, int &v )
{
   if( sameText( s, MAXINT_S ) )
   {
      v = std::numeric_limits<int>::max();
      return true;
   }
   if( sameText( s, MININT_S ) )
   {
      v = std::numeric_limits<int>::min();
      return true;
   }

   int k;
   val( s, v, k );
   return !k;
}

bool SpecialStrAsInt( const std::string &s, int &v )
{
   std::array<std::string, 3> specialStrs = {
           "off"s, "on"s, "silent"s };
   const auto it = std::find( specialStrs.begin(), specialStrs.end(), s );
   if( it != specialStrs.end() )
   {
      v = static_cast<int>(it - specialStrs.begin());
      return true;
   }
   return false;
}

std::string IncludeTrailingPathDelimiterEx( const std::string &S )
{
   return !S.empty() && ( S.back() == PathDelim || (OSFileType() == OSFileWIN && S.back() == '/') ) ? S : S + PathDelim;
}

std::string ExcludeTrailingPathDelimiterEx( const std::string &S )
{
   return !S.empty() && ( S.back() == PathDelim || (OSFileType() == OSFileWIN && S.back() == '/') ) ? std::string{S.begin(), S.end()-1} : S;
}

static void fileCase( const int fc, std::string &gs )
{
   switch( fc )
   {
      // Causes GAMS to upper case file names including the path of the file
      case 1:
         gs = UpperCase( gs );
         break;
         // Causes GAMS to lower case file names including the path of the file
      case 2:
         gs = LowerCase( gs );
         break;
         // Causes GAMS to upper case file names only (leave the path alone)
      case 3:
         gs = ExtractFilePathEx( gs ) + UpperCase( ExtractFileNameEx( gs ) );
         break;
         // Causes GAMS to lower case file names only (leave the path alone)
      case 4:
         gs = ExtractFilePathEx( gs ) + LowerCase( ExtractFileName( gs ) );
         break;
      default:
         break;
   }
}

static int gsposchar( const std::string &s, int p, const char c )
{
   // TODO: Replace gsposchar calls in cleanpath and XXXexpand with LChPosSp
   if( p <= 0 ) p = 1;
   const auto res = s.find( c, p - 1 );
   return res == std::string::npos ? 0 : static_cast<int>( res + 1 );
};

static int gsnegchar( const std::string &s, int p, const char c )
{
   // TODO: Replace gsnegchar calls in cleanpath and XXXexpand with RChPosSp
   if( p <= 0 ) p = 1;
   const auto res = s.find_last_of( c, p - 1 );
   return res == std::string::npos ? 0 : static_cast<int>( res ) + 1;
};

/*
        Note: - The comments mostly use '\' as path delimiter, though it is actually
          delim sent to this function
        - Windows accepts both '\' and '/' as delimiters. First step in this
          function is to replace '/' by '\' in path to have it consistent.
        - Always keep a . when a dot was the first char
        - Get rid of all .\ following the first \
        - Change all \.\ to \ and all \*\..\ to \
    */
void cleanpath( std::string &path, const char delim )
{
   // Replace forward '/' by backward slashes '\'
   if( OSFileType() == OSFileWIN )
      path = ReplaceChar( utils::charset { '/' }, PathDelim, path );

   auto del = []( std::string &s, int off, int count ) {
      s.erase( off - 1, count );
   };

   /* Change \.\ into \  and \\ into \ */
   for( int pos2 = gsposchar( path, 2, delim ); pos2; )
   {
      if( gsgetchar( path, pos2 + 1 ) == '.' && gsgetchar( path, pos2 + 2 ) == delim )
         del( path, pos2, 2 ); /* \.\ into \ */
      else if( gsgetchar( path, pos2 + 1 ) == delim )
         del( path, pos2, 1 ); /* \\ into \ */
      else
         pos2 = gsposchar( path, pos2 + 1, delim ); // skip to next delim index (or 0 if it was the last)
   }

   /* Change \*\..\  into \ */
   for( int pos2 = gsposchar( path, 1, delim ); pos2; )
   {
      if( gsgetchar( path, pos2 + 1 ) == '.' && gsgetchar( path, pos2 + 2 ) == '.' && gsgetchar( path, pos2 + 3 ) == delim )
      {                                                 // potential
         int pos1 = gsnegchar( path, pos2 - 1, delim ); /* find \ to the left */
         if( !pos1 )
         {
            /* .\..\xx abcd\..\ ..\..\ a:\..\  */
            /*   ..\xx          ..\..\ a:\..\  */
            if( pos2 == 2 )                                       /* possible .\..\ */
               pos1 = gsgetchar( path, pos2 - 1 ) == '.' ? 2 : 5; /* go .\..\xxx or x\..\xxx */
            else if( pos2 == 3 )
            {                                                                                          /* possible ..\..\ and c:\..\ */
               pos1 = ( ( gsgetchar( path, pos2 - 1 ) == '.' && gsgetchar( path, pos2 - 2 ) == '.' ) ||// nothing
                        ( OSFileType() == OSFileWIN && gsgetchar( path, pos2 - 1 ) == ':' ) )
                              ? 0
                              : 6;
               /* c:\..\ or xx\..\ */
            }
            else /* xxx\..\xxx */
               pos1 = pos2 + 3;
            del( path, 1, pos1 );
            pos2 = gsposchar( path, pos2 - pos1 + 1, delim );
         }
         /* yyyy\xxxx\..\ */
         else if( gsgetchar( path, pos2 - 1 ) == '.' && gsgetchar( path, pos2 - 2 ) == '.' ) // skip
            pos2 = gsposchar( path, pos2 + 3, delim );
         else
         {
            del( path, pos1 + 1, pos2 - pos1 + 3 );
            pos2 = pos1;
         }
      }// look for the next one
      else
         pos2 = gsposchar( path, pos2 + 1, delim );
   }
}

std::string CompleteDirEx( const std::string &dir1, const std::string &dir2, int fc, bool relPath )
{
   /*
   complete d2 with information from dir1
   if pfinteger[pfrelpath] != this routine does nothing
   otherwise complete result with dir1, we assume dir1 is a syntactically correct
   if result is null and we don't complet we return null?
   */

   // pfrelpath works only with UNIX and DOS
   std::string res { dir2 };
   bool noexpansion {};

   switch( OSFileType() )
   {
      case OSFileWIN:
         /*
         xxx[\]    \yy\yy[\]         d:\yy\yy\
                    yy\yy[\]         xxx\yy\yy\
                     d:yy[\]         d:\yy\  ???????  not good
                    d:\yy[\]         unchanged
                       empty          xxx\
         */
         if( !res.empty() )
            res = IncludeTrailingPathDelimiterEx( res );
         // could be a UNC name
         if( gsgetchar( res, 2 ) == ':'
            || (gsgetchar( res, 1 ) == '\\' && gsgetchar( res, 2 ) == '\\')
            || ( relPath && gsgetchar(res, 1 ) == '.' ) )
            noexpansion = true;

         if( !noexpansion )
         {
            // combine
            if( utils::in( gsgetchar( res, 1 ), '\\', '/' ) ) // \xxx | /xxx
               res = dir1.substr( 0, 2 ) + res; // assumes dir1 is a:
            else
               res = IncludeTrailingPathDelimiterEx( dir1 ) + res;
         }

         // d:xxxxx
         if( !relPath
            && gsgetchar( res, 2 ) == ':'
            && !( utils::in( gsgetchar( res, 3 ), '\\', '/' ) ) )
         {
            //int drivenum = gsgetchar( res, 1 ) - 'A' + 1;
            std::string temp = IncludeTrailingPathDelimiterEx(GetCurrentDir());
            // assume d:.\ ...
            if( gsgetchar( res, 3 ) == '.' )
               res.erase( 0, 4 );
            // assume d:name
            else
               res.erase( 0, 2 );
            res = temp + res;
         }
         res = IncludeTrailingPathDelimiterEx( res );
         cleanpath( res, '\\');
         break;
      case OSFileUNIX:
         /*
           1. add trailing / to dir1 and result if not already there
           2. if result=/xxx  => done
              if result=xxx   => result := dir1 // result
         */
         if( !res.empty() )
            res = IncludeTrailingPathDelimiterEx( res );
         if( gsgetchar( res, 1 ) == '/' || ( relPath && gsgetchar( res, 1 ) == '.' ) )
            noexpansion = true;
         if( !noexpansion )
            res = IncludeTrailingPathDelimiterEx( dir1 ) + res;
         res = IncludeTrailingPathDelimiterEx( res );
         cleanpath( res, '/' );
         break;
      default:
         throw std::runtime_error("Unknown operating system!"s);
   }
   fileCase( fc, res );
   return res;
}

// assumes all different strings
// curdir must be a full path including the drive:
// drive:\path\ for example
//
static void XXXexpand( const std::string &dirspc, const std::string &fname, bool relPath, std::string &fullname )
{
   bool noexpansion {};

   switch( OSFileType() )
   {
      case OSFileWIN:
         noexpansion = gsposchar( fname, 1, ':' ) > 0 ||
            (gsgetchar( fname, 1 ) == '\\' && gsgetchar(fname, 2) == '\\') ||
            (relPath && gsgetchar(fname, 1) == '.');
         if( noexpansion ) fullname = fname;
         else {
            if( utils::in( gsgetchar( fname, 1 ), '\\', '/' ) )// add current drive to fname
            {
               int colonpos = gsposchar( dirspc, 1, ':' );
               if( colonpos > 0 )
                  fullname = dirspc.substr( 0, colonpos ) + fname;
            }
            else // combine curdir with fname
               fullname = dirspc + fname;
            // below ???
            if( gsgetchar( fullname, static_cast<int>(fullname.length()) ) == ':' )
               fullname.pop_back();
         }

         // process current drives
         // 1. process d:.\ or d:name
         if( !relPath && gsgetchar( fullname, 2 ) == ':' )
         {
            if (!utils::in(gsgetchar(fullname, 3), '\\', '/')) {
               const int drivenum { utils::toupper( gsgetchar( fullname, 1 ) ) - static_cast<int>( 'A' )  + 1};
               std::string temp1;
               rtl::system_p3::getdir( drivenum, temp1 );
               if (drivenum == utils::toupper(gsgetchar(temp1, 1)) - static_cast<int>( 'A' ) + 1) {
                  temp1 = IncludeTrailingPathDelimiterEx( temp1 );
                  if( gsgetchar( fullname, 3 ) == '.' )
                     fullname.erase( 0, 4 );
                  else
                     fullname.erase( 0, 2 );
                  fullname = temp1 + fullname;
               }
            }
         }
         cleanpath( fullname, '\\' );
         break;
      case OSFileUNIX:
         /*
         * Unix file specs are always of form:
            /pc1/pc2/pc3/..../pcn/fn.ext  or
            /pc1/pc2/pc3/..../pcn/fn
            no leading device - devices are treated like path components
         */
         noexpansion = fname[0] == '/' || ( relPath && fname[0] == '.' );
         fullname = noexpansion ? fname : dirspc + ( dirspc[dirspc.length() - 1] != '/' ? "/"s : ""s ) + fname;
         cleanpath( fullname, '/' );
         break;
      default:
         break;
   }
}

std::string ExtractFileNameEx( const std::string &FileName )
{
   const static auto Delims {""s + PathDelim + ( OSFileType() == OSFileWIN ? "/" : "" ) + DriveDelim};
   const auto offset {LastDelimiter( Delims, FileName ) + 1};
   return std::string{FileName.begin()+offset, FileName.end()};
}

bool StrAsDoubleEx( const std::string &s, double &v )
{
   if( sameText( s, MAXDOUBLE_S ) )
   {
      v = std::numeric_limits<double>::max();
      return true;
   }
   if( sameText( s, MINDOUBLE_S ) )
   {
      v = std::numeric_limits<double>::min();
      return true;
   }
   if( sameText( s, EPSDOUBLE_S ) )
   {
      v = std::numeric_limits<double>::epsilon();
      return true;
   }
   std::string ws = s;
   replaceChar( 'D', 'E', ws );
   replaceChar( 'd', 'E', ws );
   int k;
   val( ws, v, k );
   if( std::isnan( v ) || std::isinf( v ) ) return false;
   return !k;
}

bool StrAsIntEx2( const std::string &s, int &v )
{
   bool res = StrAsIntEx( s, v );
   if( !res )
   {
      v = 0;
      double d;
      res = StrAsDoubleEx( s, d );
      if( res )
      {
         double intpart;
         res = d >= std::numeric_limits<int>::min() && d <= std::numeric_limits<int>::max() && std::modf( d, &intpart ) == 0.0;
         if( res ) v = static_cast<int>( std::trunc( d ) );
      }
   }
   return res;
}

// Brief:
//  Compare two strings for equality ignoring case
// Arguments:
//  S1: First string
//  S2: Second string
// Returns:
//  True if the strings are equal; False otherwise
bool StrUEqual( const std::string_view S1, const std::string_view S2 )
{
   const int L { static_cast<int>( S1.length() ) };
   if( L != static_cast<int>( S2.length() ) ) return false;
   for( int K { L - 1 }; K >= 0; K-- )// significant stuff at the end?
      if( toupper( S1[K] ) != toupper( S2[K] ) ) return false;
   return true;
}

bool StrUEqual( const DelphiStrRef &S1, const std::string_view S2 )
{
   const auto L { S1.length };
   if( L != S2.length() ) return false;
   for( int K { L - 1 }; K >= 0; K-- )// significant stuff at the end?
      if( toupper( S1.chars[K] ) != toupper( S2[K] ) ) return false;
   return true;
}

std::string ExtractFilePathEx( const std::string &FileName )
{
   return FileName.substr( 0, LastDelimiter( ""s + PathDelim + ( OSFileType() == OSFileWIN ? "/" : "" ) + DriveDelim, FileName ) + 1 );
}

std::string PadRight( const std::string &s, const int W )
{
   const int ww = std::min<int>( 255, W ) - static_cast<int>( s.length() );
   return ww <= 0 ? s : s + std::string( ww, ' ' );
}

std::string PadLeft( const std::string &s, const int W )
{
   const int ww = std::min<int>( 255, W ) - static_cast<int>( s.length() );
   return ww <= 0 ? s : std::string( ww, ' ' ) + s;
}

// Brief:
//   Extract the next token from a string
// Arguments:
//   s: String to extract token from
//   p: Starting position of scan
// Returns:
//   Extracted token
// Description:
//   Scanning starts at position p; blanks are skipped.
//   A token can be enclosed by a single or double quote character
//   Such a quote character will be removed in the returned value.
std::string ExtractToken( const std::string &s, int &p )
{
   if( p <= 0 ) return ""s;
   const auto L = static_cast<int>( s.length() );
   // skip leading blanks
   while( p <= L && s[p] == ' ' ) p++;
   if( p > L ) return ""s;
   char Stop;
   if( !in( s[p], '\'', '\"' ) ) Stop = ' ';
   else
   {
      Stop = s[p];
      p++;
   }
   const int rs = p;
   while( p <= L && s[p] != Stop ) p++;
   std::string res { s.substr( rs - 1, p - rs ) };
   if( p <= L && s[p] == Stop ) p++;
   return res;
}

// Brief:
//   Decode a string as an integer
// Parameters:
//   S: The string to be decoded
// Returns:
//   The integer value of the string, or zero when
//   the string does not represent an integer
// Note:
//   could use a lot more error checking
int StrAsInt( const std::string &s )
{
   int k, res;
   val( s, res, k );
   return k ? 0 : res;
}

std::string CompleteFileExtEx( const std::string &FileName, const std::string &Extension )
{
   return ExtractFileExtEx( FileName ).empty() ? ChangeFileExtEx( FileName, Extension ) : FileName;
}

std::string ChangeFileExtEx( const std::string &FileName, const std::string &Extension )
{
   const int I { LastDelimiter( OSFileType() == OSFileWIN ? "\\/:." : "/.", FileName ) };
   return FileName.substr( 0, I == -1 || FileName[I] != '.' ? static_cast<int>( FileName.length() ) : I ) + Extension;
}

std::string ExtractFileExtEx( const std::string &FileName )
{
   const int I { LastDelimiter( OSFileType() == OSFileWIN ? "\\/:." : "/.", FileName ) };
   return I >= 0 && FileName[I] == '.' ? std::string { FileName.begin() + I, FileName.end() } : ""s;
}

std::string CompleteFileNameEx( const std::string &directory, const std::string &filename, int fc, bool relPath )
{
   if(directory.empty() || filename.front() == '@')
      return filename;
   std::string res;
   XXXexpand( directory, filename, relPath, res );
   fileCase(fc, res);
   return res;
}

bool checkBOMOffset( const tBomIndic &potBOM, int &BOMOffset, std::string &msg )
{
   enum tBOM : uint8_t
   {
      bUTF8,
      bUTF16BE,
      bUTF16LE,
      bUTF32BE,
      bUTF32LE,
      num_tboms
   };
   const std::array<std::string, num_tboms> BOMtxt = { "UTF8"s, "UTF16BE"s, "UTF16LE"s, "UTF32BE"s, "UTF32LE"s };
   const std::array<std::array<uint8_t, maxBOMLen + 1>, num_tboms> BOMS = {
           {
                   { 3, 239, 187, 191, 0 },// UTF8
                   { 2, 254, 255, 0, 0 },  // UTF16BE
                   { 2, 255, 254, 0, 0 },  // UTF16LE
                   { 4, 0, 0, 254, 255 },  // UTF32BE
                   { 4, 255, 254, 0, 0 }   // UTF32LE
           } };
   msg.clear();
   BOMOffset = 0;
   for( int b = 0; b < num_tboms; b++ )
   {
      bool match { true };
      for( int j { 1 }; j <= BOMS[b][0]; j++ )
      {
         if( BOMS[b][j] != potBOM[j - 1] )
         {
            match = false;
            break;
         }
      }
      if( !match ) continue;

      if( b == bUTF8 ) BOMOffset = BOMS[b].front();// UTF8 is the only one, which is OK atm
      else
      {
         msg = BOMtxt[b] + " BOM detected. This is an unsupported encoding.";
         return false;
      }
      break;
   }
   return true;
}

// Brief:
//  Replace a set of characters by another character
// Arguments:
//  ChSet: Set of character to be replaced
//  New: Replacement character
//  S: Source string
// Returns:
//  String with characters replaced
std::string ReplaceChar( const charset &ChSet, const char New, const std::string &S )
{
   std::string out = S;
   for( char &i: out )
      if( in( i, ChSet ) )
         i = New;
   return out;
}

// Brief:
//  Replace every occurance of a string with another string
// Arguments:
//  Old: String to be replaced
//  New: Replacement string
//  S: Source string
// Returns:
//  String with substrings replaced
std::string ReplaceStr( const std::string &substr, const std::string &replacement, const std::string &S )
{
   return replaceSubstrs( S, substr, replacement );
}

// Brief:
//   Converts a file name to the short 8.3 form
// Arguments:
//   FileName: file/folder name to be converted
// Returns:
//   Converted name, empty string if the file or directory does not exist
// Description:
//   This function throws an exception if there was no conversion an the result
//   contains a blank or an unicode character. Both can be problematic. (distinguish which should cause an error by argument?)
//   see also http://blogs.msdn.com/b/winsdk/archive/2013/10/09/getshortpathname-doesn-t-return-short-path-name.aspx
std::string ExtractShortPathNameExcept( const std::string &FileName )
{
   std::string res { ExtractShortPathName( FileName ) };
   for( const char c: res )
   {
      if( static_cast<unsigned char>( c ) >= 128 ) throw std::runtime_error( "Problem extracting short path, result contains extended ASCII codes: "s + res + " (maybe 8.3 form is disabled)"s );
      if( c == ' ' ) throw std::runtime_error( "Problem extracting short path, result contains spaces: "s + res + " (maybe 8.3 form is disabled)"s );
   }
   return res;
}

/**
     * PORTING NOTES FROM ANDRE
     * Pascal/Delphi convention: 1 byte is size/length/charcount, then character bytes, then quote byte END
     * C/C++ convention here: raw character bytes, null terminator \0, quote char after that
     * Doing the quote char after the terminator keeps strlen etc working
     **/

// In-place conversion of C string (0 terminated suffix) to Pascal/Delphi string (size byte prefix)
// Returns 1 iff. the C string exceeds maximum short string length of 255 characters
int strConvCtoDelphi( char *cstr )
{
   const auto len = strlen( cstr );
   if( len > std::numeric_limits<uint8_t>::max() )
   {
      const auto errMsg { "Error: Maximum short string length is 255 characters!"s };
      cstr[0] = 0;
      std::memcpy( &cstr[1], errMsg.c_str(), errMsg.length() + 1 );
      return static_cast<int>( std::strlen( &cstr[1] ) );
   }
   std::memmove( cstr + 1, cstr, len );
   reinterpret_cast<unsigned char *>( cstr )[0] = static_cast<unsigned char>( len );
   return 0;
}

// In-place conversion of Pascal/Delphi string (size byte prefix) to C string (0 terminated suffix)
void strConvDelphiToC( char *delphistr )
{
   const auto len = static_cast<uint8_t>( delphistr[0] );
   std::memmove( delphistr, delphistr + 1, len );
   delphistr[len] = '\0';
}

// Value-copy conversion of Pascal/Delphi string (size byte prefix) to C++ standard library (STL) string
std::string strConvDelphiToCpp( const char *delphistr )
{
   sstring buffer {};
   const auto len = static_cast<uint8_t>( delphistr[0] );
   for( int i = 0; i < len; i++ )
      buffer[i] = delphistr[i + 1];
   buffer[len] = '\0';
   return std::string { buffer.data() };
}

// Convert C++ standard library string to Delphi short string
int strConvCppToDelphi( const std::string &s, char *delphistr )
{
   if( s.length() > std::numeric_limits<uint8_t>::max() )
   {
      const auto errorMessage { "Error: Maximum short string length is 255 characters!"s };
      std::memcpy( &delphistr[1], errorMessage.c_str(), errorMessage.length() + 1 );
      return static_cast<int>( errorMessage.length() );
   }
   const auto l = static_cast<uint8_t>( s.length() );
   delphistr[0] = static_cast<char>( l );
   std::memcpy( &delphistr[1], s.c_str(), l );
   return 0;
}

bool PStrUEqual( const std::string_view P1, const std::string_view P2 )
{
   if( P1.empty() || P2.empty() ) return P1.empty() && P2.empty();
   const size_t L { P1.length() };
   if( L != P2.length() ) return false;
   for( int K = static_cast<int>( L ) - 1; K >= 0; K-- )
   {
      if( toupper( P1[K] ) != toupper( P2[K] ) )
         return false;
   }
   return true;
}

inline int b2i( const bool b ) { return b ? 1 : 0; }

int PStrUCmp( const std::string_view P1, const std::string_view P2 )
{
   return !P1.empty() && !P2.empty() ? StrUCmp( P1, P2 ) : b2i( !P1.empty() ) - b2i( !P2.empty() );
}

int StrUCmp( const std::string_view S1, const std::string_view S2 )
{
   auto L = S1.length();
   if( L > S2.length() ) L = S2.length();
   for( int K {}; K < static_cast<int>( L ); K++ )
   {
      if( const int d = toupper( S1[K] ) - toupper( S2[K] ) )
         return d;
   }
   return static_cast<int>( S1.length() - S2.length() );
}

bool PStrEqual( const std::string_view P1, const std::string_view P2 )
{
   if( P1.empty() || P2.empty() ) return P1.empty() && P2.empty();
   const size_t L { P1.length() };
   if( L != P2.length() ) return false;
   for( int K = static_cast<int>( L ) - 1; K >= 0; K-- )
   {
      if( P1[K] != P2[K] )
         return false;
   }
   return true;
}

// Brief:
//  Search for a substring in a string from a starting position
// Arguments:
//  Pat: Substring to search for
//  S: String to be searched (n characters)
//  Sp: Starting position (0...n-1)
// Returns:
//  Location of the substring when found; -1 otherwise
int LStrPosSp( const std::string &pat, const std::string &s, const int sp )
{
   const size_t lp { pat.length() };
   if( const size_t ls { s.length() };
      !lp || !ls || sp < 0 || sp + lp > ls )
      return -1;
   const char pat1 { pat.front() };
   if(lp == 1)
   {
      for(int p{sp}; p<static_cast<int>(s.length()); p++)
         if(s[p] == pat1)
            return p;
   }
   else
   {
      for(int p{sp}; p<=static_cast<int>(s.length()-lp); p++)
      {
         if(s[p] != pat1)
            continue;
         int res {p};
         for(int k{1}; k<static_cast<int>(lp); k++)
         {
            if(pat[k] != s[p+k])
            {
               res = -1;
               break;
            }
         }
         if(res > -1)
            return res;
      }
   }
   return -1;
}

// Brief:
//  Search for a substring in a string
// Arguments:
//  Pat: Substring to search for
//  S: String to be searched
// Returns:
//  Location of the substring when found; zero otherwise
int LStrPos( const std::string &Pat, const std::string &S )
{
   return LStrPosSp(Pat, S, 0);
}

int StrUCmp( const DelphiStrRef &S1, const DelphiStrRef &S2 )
{
   auto L = S1.length;
   if( L > S2.length ) L = S2.length;
   for( int K {}; K < L; K++ )
   {
      if( const int d = toupper( S1.chars[K] ) - toupper( S2.chars[K] ) )
         return d;
   }
   return S1.length - S2.length;
}

}// namespace gdlib::strutilx
