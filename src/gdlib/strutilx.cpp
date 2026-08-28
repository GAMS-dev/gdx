/*
* GAMS - General Algebraic Modeling System GDX API
 *
 * Copyright (c) 2017-2026 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2026 GAMS Development Corp. <support@gams.com>
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
#include <filesystem>
#include <limits>                // for numeric_limits
#include <stdexcept>             // for runtime_error
#include <string>                // for basic_string, string, operator+, all...

#include "../rtl/p3io.hpp"         // for P3_Str_dd0
#include "../rtl/p3platform.hpp"   // for OSFileType, tOSFileType
#include "../rtl/sysutils_p3.hpp"  // for LastDelimiter, PathDelim, ExtractSho...
#include "../rtl/system_p3.hpp"

#include "utils.hpp"               // for toupper, sameText, ord, in, val, cha...

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

using namespace std::literals::string_literals;
using namespace GDX_NS rtl::sysutils_p3;
using namespace GDX_NS rtl::p3platform;
using namespace GDX_NS utils;

namespace fs = std::filesystem;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace GDX_NS gdlib::strutilx
{

constexpr auto MAXINT_S = "maxint", MININT_S = "minint";
constexpr auto MAXDOUBLE_S = "maxdouble", EPSDOUBLE_S = "eps", MINDOUBLE_S = "mindouble";

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

std::string IntToNiceStr( int64_t N )
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

// heavily LLM-inspired
#if __cplusplus >= 202002L
#ifndef _WIN32

#include <locale.h>
#include <langinfo.h>
#include <strings.h>
#include <cwchar>
#include <cstdlib>

// 1. Check if a specific locale object uses UTF-8
static bool isThreadLocaleUtf8(locale_t loc)
{
    if (loc == (locale_t)0) return false;

    const char* codeset = nl_langinfo_l(CODESET, loc);
    return (strcasecmp(codeset, "UTF-8") == 0 || strcasecmp(codeset, "utf8") == 0);
}

// 2. Safely acquire the best available UTF-8 locale
static locale_t acquireUtf8Locale()
{
    locale_t target_locale = newlocale(LC_CTYPE_MASK, "", (locale_t)0);
    if (target_locale != (locale_t)0 && isThreadLocaleUtf8(target_locale)) {
        return target_locale;
    }
    if (target_locale != (locale_t)0) freelocale(target_locale);

    target_locale = newlocale(LC_CTYPE_MASK, "en_US.UTF-8", (locale_t)0);
    if (target_locale != (locale_t)0) return target_locale;

    target_locale = newlocale(LC_CTYPE_MASK, "C.UTF-8", (locale_t)0);
    if (target_locale != (locale_t)0) return target_locale;

   // for Darwin, maybe BSD
   target_locale = newlocale(LC_CTYPE_MASK, "UTF-8", (locale_t)0);
   if (target_locale != (locale_t)0) return target_locale;

    return (locale_t)0; // System has absolutely no UTF-8 locales
}

enum CaseAction {
   ToUpper,
   ToLower
};

template<enum CaseAction action>
static std::u8string caseChangeUtf8(const std::u8string& input)
{
   if (input.empty()) return input;

   locale_t utf8_locale = acquireUtf8Locale();

   // ==========================================
   // FALLBACK: Pure ASCII Lowercasing
   // ==========================================
   if (utf8_locale == (locale_t)0) {
      std::u8string result = input;
      for (char8_t &c : result) {
         // Cast to unsigned char to safely check bounds
         auto uc = static_cast<unsigned char>(c);

         if (uc > 127) {
            throw std::runtime_error(
               "System lacks UTF-8 support, and a non-ASCII character "
               "was encountered during fallback processing.\n" +
               "The string was: "s + reinterpret_cast<const char *>(input.data()) + "\n"
               "Hint: either set LC_CTYPE to a UTF-8 locale of your choice, "
               "or make sure that one of the following locale is available: en_US.UTF-8, C.UTF-8, UTF-8"
            );
         }

         // Standard ASCII lowercase conversion
         if constexpr (action == ToLower) {
            if (uc >= 'A' && uc <= 'Z') {
               c |= 0x20;
            }
         } else {
            if (uc >= 'a' && uc <= 'z') {
               c &= ~0x20;
            }

         }
      }
      return result;
   }

   // ==========================================
   // PRIMARY: Thread-Local UTF-8 Lowercasing
   // ==========================================

   // Apply the UTF-8 locale strictly to this thread
   locale_t old_locale = uselocale(utf8_locale);

   // Convert UTF-8 to Wide String
   std::vector<wchar_t> wstr(input.size() + 1);
   const char *input_s = reinterpret_cast<const char*>(input.c_str());
   size_t converted = std::mbstowcs(wstr.data(), input_s, wstr.size());

   if (converted == static_cast<size_t>(-1)) {
      uselocale(old_locale);
      freelocale(utf8_locale);
      throw std::runtime_error("Invalid UTF-8 sequence encountered in input.");
   }

   // Lowercase wide characters using the thread's active locale
   for (size_t i = 0; i < converted; ++i) {
      if constexpr (action == ToLower)
         wstr[i] = std::towlower(wstr[i]);
      else
         wstr[i] = std::towupper(wstr[i]);
   }

   // Convert back to UTF-8
   // MB_CUR_MAX is context-aware and respects the current thread's locale
   // but it might be broken on Darwin and BSD: 4 is the UTF-8 upper bound
   std::vector<char> lower_utf8(converted * 4 + 1);
   std::wcstombs(lower_utf8.data(), wstr.data(), lower_utf8.size());

   // Clean up thread state and free memory
   uselocale(old_locale);
   freelocale(utf8_locale);

   return std::u8string(reinterpret_cast<const char8_t*>(lower_utf8.data()));
}

#endif // !_WIN32

static fs::path toLowerPath(const fs::path& path)
{
   if (path.empty()) { return path; }
    auto str = path.native();

#ifdef _WIN32

   int reqSize = LCMapStringW(
      LOCALE_USER_DEFAULT, // Use user's locale (important for the German 'ß', etc.)
      LCMAP_LOWERCASE,     // Flag to lowercase
      str.c_str(),
      static_cast<int>(str.length()),
      nullptr,
      0
   );

   if (reqSize == 0) { throw std::runtime_error("LCMapStringW failed"); }

   std::wstring lowerStr(reqSize, L'\0');
   LCMapStringW(
      LOCALE_USER_DEFAULT,
      LCMAP_LOWERCASE,
      str.c_str(),
      static_cast<int>(str.length()),
      lowerStr.data(),
      reqSize
   );

   return fs::path(lowerStr);

#else

   return fs::path(caseChangeUtf8<ToLower>(path.u8string()));

#endif
}

static fs::path ToUpperPath(const fs::path& path)
{
   if (path.empty()) { return path; }
    auto str = path.native();

#ifdef _WIN32

   int reqSize = LCMapStringW(
      LOCALE_USER_DEFAULT, // Use user's locale (important for the German 'ß', etc.)
      LCMAP_UPPERCASE,
      str.c_str(),
      static_cast<int>(str.length()),
      nullptr,
      0
   );

   if (reqSize == 0) { throw std::runtime_error("LCMapStringW failed"); }

   std::wstring lowerStr(reqSize, L'\0');
   LCMapStringW(
      LOCALE_USER_DEFAULT,
      LCMAP_UPPERCASE,
      str.c_str(),
      static_cast<int>(str.length()),
      lowerStr.data(),
      reqSize
   );

   return fs::path(lowerStr);

#else

   return fs::path(caseChangeUtf8<ToUpper>(path.u8string()));

#endif

}

static fs::path fileCase( const int fc, fs::path &p )
{
   switch( fc )
   {
      // Causes GAMS to upper case file names including the path of the file
      case 1:
         p = ToUpperPath( p );
         break;
         // Causes GAMS to lower case file names including the path of the file
      case 2:
         p = toLowerPath( p );
         break;
         // Causes GAMS to upper case file names only (leave the path alone)
      case 3:
         return p.parent_path() / ToUpperPath( p.filename() );
         break;
         // Causes GAMS to lower case file names only (leave the path alone)
      case 4:
         return p.parent_path() / toLowerPath( p.filename() );
         break;
      default: ;
   }

   return p;
}

#endif // C++20

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

std::string CompleteDirEx( const std::string_view dir1, const std::string_view dir2,
                          int fc, bool relPath )
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
               res = std::string(dir1.substr( 0, 2 )) + res; // assumes dir1 is a:
            else
               res = IncludeTrailingPathDelimiterEx( std::string(dir1) ) + res;
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
            res = IncludeTrailingPathDelimiterEx( std::string(dir1) ) + res;
         res = IncludeTrailingPathDelimiterEx( res );
         cleanpath( res, '/' );
         break;
      default:
         throw std::runtime_error("Unknown operating system!"s);
   }
   fileCase( fc, res );
   return res;
}

#if __cplusplus >= 202002L

#ifdef _WIN32
/**
 * @brief Extract the volume name of a path (windows only)
 *
 * @param path  the path
 *
 * @return      the volume name
 */
static fs::path extractVolumeName(const fs::path &p)
{
   /*
    * input                    output
    *
    * C:\[xxx]                 C:\
    * \\?C:\[xxx]              \\?C:\
    * \\Server[\xxx]           \\Server
    * \\?\Server[\xxx]         \\?\Server
    * \\?\UNC\Server[\xxx]     \\?\UNC\Server
    */
   // Win32 File Namespace prefix
   constexpr std::wstring_view WFNprefix = LR"(\\?)";
   constexpr std::wstring_view UNCprefix = LR"(UNC)";

   // Sadly, \\?\UNC\Server\share\... has relative path UNC\Server\share\...
   //        \\?\UNC\SERVER is actually the volume name we want
   fs::path dirRelative = p.relative_path();
   std::wstring dirRelativeStr = dirRelative.wstring();
   fs::path rootName = p.root_name();

   if (rootName.empty()) { return fs::path(); }

   // no \\?[\UNC] shenanigans: return "X:\" or "\\Server"
   // TODO: "\\." could be another prefix
   if (rootName != WFNprefix) {
      return rootName / p.root_directory();
   }

   // \\?\C:[\xxx] -> call extractVolumeName(C:[\xxx])
   if (dirRelativeStr.length() >= 2 && dirRelativeStr[1] == L':') {
      // NOTE: we are really fighting the fs library here.
      // The `\\?\` prefix is not maintained if not necessary, as below
      //return fs::path(WFNprefix) / extractVolumeName(dirRelative);
      //we need to go to string manipulation
      return fs::path(std::wstring(WFNprefix) + L"\\" + extractVolumeName(dirRelative).wstring());

   }

   // find first separator in (\\UNC)\xxx[\yyy]
   // NOTE: there is an edge case when dir is \\?\UNC\Server
   size_t offset = dirRelativeStr.starts_with(UNCprefix) ? UNCprefix.length()+1 : 0;
   size_t nextSep = dirRelativeStr.find_first_of(LR"(/\)", offset);

   auto subPathStr = dirRelativeStr.substr(0, nextSep);

   return rootName / fs::path(subPathStr);
}

#endif // _WIN32

enum class CompletePathType {
   Dir,
   File,
};

/**
 * @brief Apply filecase and ensures there is a trailing separator for a directory
 *
 * @param path   the path to operate on
 * @param fc     if nonzero, force either the filename or the full path to be either upper or lower case
 *
 * @return       the resulting path
 */
template<CompletePathType type>
static fs::path applyFileCase(fs::path &path, int fc)
{
   if constexpr (type == CompletePathType::Dir) { // ensure trailing separator
      if (path.native().back() != fs::path::preferred_separator) {
         path += fs::path::preferred_separator;
      }
   }

   return fileCase( fc, path );
}

/**
 * @brief Complete, if needed, a path using either the argument, or some working directory
 *
 * On UNIX, the semantics are trivial: if the path is relative and keepRelPath is false,
 * output prefixDir / path.
 *
 * On Windows, look at the code
 *
 * @param prefixDir     the prefix directory
 * @param path          the path to operate on
 * @param fc            if nonzero, force either the filename or the full path to be either upper or lower case
 * @param keepRelPath   if true, keep the path relative in some cases, but not others (see code)
 *
 * @return              the potentially completed path, with casing as specified
 */
template<CompletePathType type>
static fs::path CompletePathEx(const fs::path &prefixDir, const fs::path &path, int fc, bool keepRelPath )
{
   assert(prefixDir.is_absolute()); // untested with relative path

   fs::path outPath {}, outPathClean {};
   if (path.empty()) {
      outPathClean = prefixDir.lexically_normal();
      return applyFileCase<type>(outPathClean, fc);
   }

   // NOTE: d:xxx is NOT absolute
   if (path.is_absolute()) {
      outPathClean = path.lexically_normal();
      return applyFileCase<type>(outPathClean, fc);
   }

   auto pathStr = path.native();
   bool hasLeadingDot = (pathStr.length() > 0 && pathStr[0] == '.');


   // path starts with '.': if it starts with '..', take normalized form;
   //                       else keep the leading '.'
   if (keepRelPath && hasLeadingDot) {

      if (pathStr.length() >= 2 && pathStr[1] == '.') { // path starts with '..'
         outPathClean = path.lexically_normal();
      } else { // path is ".\dir", but inputDir is "dir"
         outPathClean = fs::path(*path.begin()) / path.lexically_normal();
      }

      return applyFileCase<type>(outPathClean, fc);
   }

#ifdef _WIN32
   // NOTE: the original comment made no sense. This is my interpreted version --OH
   /*
         prefixDir         dir         outputDir
         ---------    -----------      ---------
         z:\xxx[\]      \yy\yy[\]      z:\yy\yy       case 1
                         yy\yy[\]      z:\xxx\yy\yy   case 2a
                       .\yy\yy         z:\xxx\yy\yy   case 2b [keepRelPath ignored]
                       d:yy[\]         $PWD\yy\       case 3
                         empty         z:\xxx         case 4
                      d:\yy[\]         unchanged      case 5
         */
   fs::path inPath { path.lexically_normal() };
   fs::path relPath { inPath.relative_path() };

   // FIXME: is this needed?
   inPath.make_preferred(); // convert '/' to '\'; just in case

   // split path C:\test\p1 into  'C:' '\' 'test\p1'
   auto rootName = inPath.root_name();
   auto rootDir = inPath.root_directory();
   bool isRel = inPath.is_relative();

   if (isRel && !rootDir.empty() && rootName.empty()) { // case 1, unconditional
      //
      assert(rootName.empty());
      outPath = extractVolumeName(prefixDir) / inPath.relative_path();

   } else if (rootName.empty() && (!keepRelPath || relPath.wstring()[0] != L'.')) {
      // case 2a
      if( rootName.empty() && !rootDir.empty()) {

         if (prefixDir.is_relative()) {
            std::runtime_error("unexpected relative path as prefixDir in CompleteDirEx");
         }

         outPath = extractVolumeName(prefixDir) / inPath;
      } // case 2b
      else
      outPath = prefixDir / inPath;

   } else {
      outPath = inPath;
   }

   // case 3: d:yy -> d:xxx\yy
   if(!keepRelPath && outPath.root_directory().empty()) {
      // NOTE: different behavior for directory or file
      //
      if constexpr (type == CompletePathType::Dir) {
         outPath = fs::current_path() / inPath.relative_path();
      } else if constexpr (type == CompletePathType::File) {
         outPath = fs::absolute(inPath);
         if (outPath.is_relative()) { // fix when there is no cwd for that disk
            outPath = outPath.root_path() / outPath.relative_path();
         }
      } else {
         std::runtime_error("Unhandled complete type");
      }

   }

   // catch all
   if (outPath.empty()) {
      outPath = inPath;
   }

#else //_WIN32
   /*
      If inputDir is absolute, like /xxx  => done
      else outDir = prefixDir / inputDir
   */
   if( path.is_relative() && !keepRelPath )
      outPath = prefixDir / path;

#endif //_WIN32

   outPathClean = outPath.lexically_normal();

   return applyFileCase<type>(outPathClean, fc);
}

/**
 * @brief  If needed, complete a given directory to make it absolute
 *
 * @param prefixDir    the prefix directory
 * @param path         the path to operate on
 * @param fc           file case value: a nonzero value induces changes
 * @param keepRelPath  if true, keep relative path as is (do not change to an absolute path)
 *
 * @return         the modified path
 */
fs::path CompleteDirEx( const fs::path &prefixDir, const fs::path &path, int fc, bool keepRelPath )
{
   return CompletePathEx<CompletePathType::Dir>(prefixDir, path, fc, keepRelPath);
}
#endif

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
   while( p <= L && s[p-1] == ' ' ) p++;
   if( p > L ) return ""s;
   char Stop;
   if( !in( s[p-1], '\'', '\"' ) ) Stop = ' ';
   else
   {
      Stop = s[p-1];
      p++;
   }
   const int rs = p;
   while( p <= L && s[p-1] != Stop ) p++;
   std::string res { s.substr( rs - 1, p - rs ) };
   if( p <= L && s[p-1] == Stop ) p++;
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

std::string CompleteFileExtEx( const std::string_view FileName, const std::string_view Extension )
{
   return ExtractFileExtEx( FileName ).empty() ? ChangeFileExtEx( FileName, Extension ) : std::string(FileName);
}

std::string ChangeFileExtEx( const std::string_view FileName, const std::string_view Extension )
{
   const int I { LastDelimiter( OSFileType() == OSFileWIN ? "\\/:." : "/.", FileName ) };
   return std::string(FileName).substr( 0, I == -1 || FileName[I] != '.' ? static_cast<int>( FileName.length() ) : I ) + std::string(Extension);
}

std::string ExtractFileExtEx( const std::string_view FileName )
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

#if __cplusplus >= 202002L
fs::path CompleteFileNameEx( const fs::path &directory, const fs::path &filename, int fc, bool keepRelPath )
{
   if(directory.empty() || filename.native().front() == '@')
      return filename;

   // FIXME: This needs to be tested but porting the XXXexpand function should be avoided
   return CompletePathEx<CompletePathType::File>(directory, filename, fc, keepRelPath);
}
#endif

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

#if __cplusplus >= 202002L
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
template<typename T>
T ExtractShortPathNameExcept( const T &FileName )
{
   if (FileName.empty()) { return FileName; }

   T res { ExtractShortPathName( FileName ) };
   for( auto c : res )
   {
      if( static_cast<unsigned char>( c ) >= 128 )
         throw std::runtime_error( "Problem extracting short path, result contains extended ASCII codes: "s
                                  + reinterpret_cast<const char *>(to_u8string(res.c_str()).c_str())
                                  + " (maybe 8.3 form is disabled)"s );
      if( c == ' ' ) throw std::runtime_error( "Problem extracting short path, result contains spaces: "s
                                              + reinterpret_cast<const char *>(to_u8string(res.c_str()).c_str())
                                              + " (maybe 8.3 form is disabled)"s );
   }
   return res;
}

template std::string ExtractShortPathNameExcept(const std::string &FileName);
template std::wstring ExtractShortPathNameExcept(const std::wstring &FileName);

#endif

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
