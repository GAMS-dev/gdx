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

// FIXME: Get rid of too many "const std::string &" processing functions and use "std::string_view" or "const char *" instead!

#include "utils.hpp"
#include "p3io.hpp"
#include "sysutils_p3.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <list>
#include <fstream>
#include <sstream>
#include <cmath>
#include <chrono>
#include <thread>
#include <cassert>
#include <numeric>

using namespace std::literals::string_literals;

// ==============================================================================================================
// Implementation
// ==============================================================================================================
#if defined(NDEBUG) && !defined(__IN_CPPMEX__)
std::stringstream debugStream;
#endif

namespace utils
{

bool anychar( const std::function<bool( char )> &predicate, const std::string_view s )
{
   return std::any_of( std::cbegin( s ), std::cend( s ), predicate );
}

int indexOf( const std::string &s, const char c )
{
   const auto p { s.find(c) };
   return p == std::string::npos ? -1 : static_cast<int>( p );
}

void permutAssign( std::string &lhs, const std::string &rhs,
                   const std::vector<int> &writeIndices, const std::vector<int> &readIndices )
{
   for( int i = 0; i < static_cast<int>( writeIndices.size() ); i++ )
   {
      lhs[writeIndices[i]] = rhs[readIndices[i]];
   }
}

// Shouldn't this operate on a std::string_view instead?
void removeTrailingCarriageReturnOrLineFeed( std::string &s )
{
   if( const char lchar = s[s.length() - 1]; lchar == '\r' || lchar == '\n' )
      s.pop_back();
}

std::string uppercase( const std::string_view s )
{
   std::string out { s };
   std::transform( s.begin(), s.end(), out.begin(), toupper );
   return out;
}

std::string lowercase( const std::string_view s )
{
   std::string out { s };
   std::transform( s.begin(), s.end(), out.begin(), tolower );
   return out;
}

bool sameTextInvariant( const std::string_view a,
                        const std::string_view b )
{
   if( b.length() != a.length() ) return false;
#if defined(_WIN32)
   // This appears to be still faster with MSVC and icl.exe
   const auto l = a.length();
   for( size_t i {}; i < l; i++ )
   {
      if( a[i] != b[i] && tolower( a[i] ) != tolower( b[i] ) )
         return false;
   }
   return true;
#else
   // Much faster with GCC and Clang (and new Intel compiler)
   return std::equal(a.begin(), a.end(), b.begin(),
      []( const unsigned char c1, const unsigned char c2) {
         return c1 == c2 || tolower(c1) == tolower(c2);
   });
#endif
}

std::string_view trim( const std::string_view s )
{
   if( s.empty() ) return {};
   const auto firstNonBlank = s.find_first_not_of( " \t\n\r" );
   const auto lastNonBlank = s.find_last_not_of( " \t\n\r" );
   return s.substr( firstNonBlank, lastNonBlank - firstNonBlank + 1 );
}

std::string getLineWithSep( std::istream &fs )
{
   std::string line;
   std::getline( fs, line );
   if( !fs.eof() )
   {
      fs.unget();
      line.push_back( static_cast<char>( fs.get() ) );
   }
   return line;
}

bool sameTextPrefix( const std::string_view s, const std::string_view prefix )
{
   return sameText( s.substr( 0, prefix.length() ), prefix );
}

bool hasNonBlank( const std::string_view s )
{
   return std::any_of( s.begin(), s.end(), []( const char c ) {
      return !in( c, ' ', '\t', '\r', '\n' );
   } );
}

std::string trim( const std::string &s )
{
   if( s.empty() ) return s;
   const auto firstNonBlank = s.find_first_not_of( " \t\n\r" );
   if(firstNonBlank == std::string::npos) return ""s;
   const auto lastNonBlank = s.find_last_not_of( " \t\n\r" );
   return s.substr( firstNonBlank, lastNonBlank - firstNonBlank + 1 );
}

std::string trimRight( const std::string &s )
{
   if( s.empty() || !isblank( s.back() ) ) return s;
   const auto lastNonBlank = s.find_last_not_of( " \t" );
   if(lastNonBlank == std::string::npos) return ""s;
   return s.substr( 0, lastNonBlank + 1 );
}

const char *trimRight( const char *s,
                       char *storage,
                       int &slen )
{
   int i;
   slen = -1;// all whitespace? => slen=0!
   for( i = 0; s[i] != '\0'; i++ )
      if( static_cast<unsigned char>( s[i] ) > 32 )
         slen = i;
   if( ++slen == i ) return s;
   std::memcpy( storage, s, slen );
   storage[slen] = '\0';
   return storage;
}

void trimRight( const std::string &s, std::string &storage )
{
   if( s.empty() || !isblank( s.back() ) )
   {
      storage = s;
      return;
   }
   const auto ub = s.find_last_not_of( " \t" ) + 1;
   if(ub == std::string::npos)
   {
      storage.clear();
      return;
   }
   storage.replace( 0, ub, s, 0, ub );
   storage.resize( ub );
}

std::string trimZeroesRight( const std::string &s, const char DecimalSep )
{
   if( s.find( DecimalSep ) == std::string::npos ) return s;
   int i { static_cast<int>( s.length() ) - 1 };
   for( ; i >= 0; i-- )
      if( s[i] != '0' ) break;
   return s.substr( 0, i + 1 );
}

bool hasCharLt( const std::string_view s, int n )
{
   return anychar( [&n]( const char c ) { return static_cast<int>( c ) < n; }, s );
}

double round( const double n, const int ndigits )
{
   return std::round( n * std::pow( 10, ndigits ) ) * pow( 10, -ndigits );
}

void replaceChar( const char a, const char b, std::string &s )
{
   if( a == b ) return;
   std::replace_if(s.begin(), s.end(), [a]( const char i ) { return i == a; }, b );
}

std::string quoteWhitespace( const std::string &s, char quotechar )
{
   return s.find( ' ' ) != std::string::npos ? ""s + quotechar + s + quotechar : s;
}

int strCompare( const char *S1, const char *S2, const bool caseInsensitive )
{
   if( S1[0] == '\0' || S2[0] == '\0' )
      return ( S1[0] != '\0' ? 1 : 0 ) - ( S2[0] != '\0' ? 1 : 0 );
   for( size_t K = 0; S1[K] != '\0' && S2[K] != '\0'; K++ )
   {
      const int c1 = static_cast<unsigned char>( caseInsensitive ? toupper( S1[K] ) : S1[K] );
      const int c2 = static_cast<unsigned char>( caseInsensitive ? toupper( S2[K] ) : S2[K] );
      if( const int d = c1 - c2 )
         return d;
   }
   return static_cast<int>( std::strlen( S1 ) - std::strlen( S2 ) );
}

/**
     * PORTING NOTES FROM ANDRE
     * Pascal/Delphi convention: 1 byte is size/length/charcount, then character bytes, then quote byte END
     * C/C++ convention here: raw character bytes, null terminator \0, quote char after that
     * Doing the quote char after the terminator keeps strlen etc working
     **/

// Convert C++ standard library string to Delphi short string
int strConvCppToDelphi( const std::string_view s, char *delphistr )
{
   if( s.length() > std::numeric_limits<uint8_t>::max() )
   {
      const auto errorMessage { "Error: Maximum short string length is 255 characters!"s };
      strConvCppToDelphi( errorMessage, delphistr );
      return static_cast<int>( errorMessage.length() );
   }
   const auto l = static_cast<uint8_t>( s.length() );
   delphistr[0] = static_cast<char>( l );
   std::memcpy( &delphistr[1], s.data(), l );
   return 0;
}

std::string replaceSubstrs( const std::string_view s, const std::string_view substr, const std::string_view replacement )
{
   if( substr.empty() || substr == replacement )
      return std::string { s };

   std::string res;
   res.reserve( s.length() );

   size_t last_pos {}, find_pos;

   while( ( find_pos = s.find( substr, last_pos ) ) != std::string_view::npos )
   {
      res.append( s.data() + last_pos, find_pos - last_pos );
      res.append( replacement );
      last_pos = find_pos + substr.length();
   }
   res.append( s.data() + last_pos, s.length() - last_pos );
   return res;
}

// Mimicks Delphi System.Val, see:
// https://docwiki.embarcadero.com/Libraries/Sydney/en/System.Val
// https://www.delphibasics.co.uk/RTL.php?Name=Val
void val( const std::string &s, double &num, int &code )
{
   rtl::p3io::P3_Val_dd(s.c_str(), s.length(), &num, &code);
}

// Mimicks Delphi System.Val, see:
// https://docwiki.embarcadero.com/Libraries/Sydney/en/System.Val
// https://www.delphibasics.co.uk/RTL.php?Name=Val
void val(const char* s, const int slen, double& num, int& code)
{
   rtl::p3io::P3_Val_dd(s, slen, &num, &code);
}

// Mimicks Delphi System.Val, see:
// https://docwiki.embarcadero.com/Libraries/Sydney/en/System.Val
// https://www.delphibasics.co.uk/RTL.php?Name=Val
void val( const std::string &s, int &num, int &code )
{
   rtl::p3io::P3_Val_i(s.c_str(), s.length(), &num, &code);
}

// Mimicks Delphi System.Val, see:
// https://docwiki.embarcadero.com/Libraries/Sydney/en/System.Val
// https://www.delphibasics.co.uk/RTL.php?Name=Val
void val( const char *s, const int slen, int &num, int &code )
{
   rtl::p3io::P3_Val_i(s, slen, &num, &code);
}

inline std::string repeatChar( const int n, const char c )
{
   return n > 0 ? std::string( n, c ) : ""s;
}

std::string blanks( const int n )
{
   return repeatChar( n, ' ' );
}

std::string zeros( const int n )
{
   return repeatChar( n, '0' );
}

int lastOccurence( const std::string_view s, const char c )
{
   for( int i = static_cast<int>( s.length() ) - 1; i >= 0; i-- )
      if( s[i] == c ) return i;
   return -1;
}

double parseNumber( const std::string &s )
{
   return std::strtod( s.c_str(), nullptr );
}

double parseNumber( const char *s )
{
   return std::strtod( s, nullptr );
}

void sleep( const int milliseconds )
{
   std::this_thread::sleep_for( std::chrono::milliseconds { milliseconds } );
}

int strLenNoWhitespace( const std::string_view s )
{
   return static_cast<int>( std::count_if( s.begin(), s.end(), []( const char c ) {
      return !std::isspace( c );
   } ) );
}

char &getCharAtIndexOrAppend( std::string &s, const int ix )
{
   const auto l = s.length();
   assert( ix >= 0 && ix <= static_cast<int>( l ) && "Index not in valid range" );
   if( static_cast<size_t>( ix ) == l ) s.push_back( '\0' );
   return s[ix];
}

bool strContains( const std::string_view s, const char c )
{
   return s.find( c ) != std::string::npos;
}

bool strContains( const std::string_view s, const std::initializer_list<char> &cs )
{
   return std::any_of( std::cbegin( s ), std::cend( s ),
                       [&cs]( const char c ) { return std::find( cs.begin(), cs.end(), c ) != cs.end(); } );
}

bool excl_or( const bool a, const bool b )
{
   return ( a && !b ) || ( !a && b );
}

int posOfSubstr( const std::string_view sub, const std::string_view s )
{
   const auto p = s.find( sub );
   return p == std::string::npos ? -1 : static_cast<int>( p );
}

std::list<std::string> split( const std::string_view s, const char sep )
{
   std::list<std::string> res;
   std::string cur;
   for( const char c : s )
   {
      if( c != sep ) cur += c;
      else if( !cur.empty() )
      {
         res.push_back( cur );
         cur.clear();
      }
   }
   if( !cur.empty() ) res.push_back( cur );
   return res;
}

std::list<std::string> splitWithQuotedItems( const std::string_view s )
{
   constexpr char sep = ' ';
   const charset quoteChars { '\"', '\'' };
   std::list<std::string> res;
   std::string cur;
   bool inQuote {};
   for( char c: s )
   {
      if( in( c, quoteChars ) )
      {
         inQuote = !inQuote;
      }
      if( c != sep || inQuote ) cur += c;
      else if( !cur.empty() )
      {
         res.push_back( cur );
         cur.clear();
      }
   }
   if( !cur.empty() ) res.push_back( cur );
   return res;
}

std::string slurp( const std::string &fn )
{
   std::ifstream fp { fn };
   std::stringstream ss;
   std::copy( std::istreambuf_iterator<char>( fp ),
              std::istreambuf_iterator<char>(),
              std::ostreambuf_iterator<char>( ss ) );
   return ss.str();
}

void spit( const std::string &fn, const std::string &contents )
{
   std::ofstream fp { fn };
   fp << contents;
}

void assertOrMsg( bool condition, const std::string &msg )
{
#if !defined(NDEBUG)
   if( !condition )
      throw std::runtime_error( "Assertion failed: " + msg );
#endif
}

// same as std::string::substr but silent when offset > input size
std::string_view substr( const std::string_view s, const int offset, const int len )
{
   return s.empty() || offset > static_cast<int>( s.size() ) - 1 ? std::string_view {} : s.substr( offset, len );
}

std::string constructStr( const int size, const std::function<char( int )> &charForIndex )
{
   std::string s;
   s.resize( size );
   for( int i {}; i < size; i++ )
      s[i] = charForIndex( i );
   return s;
}

std::string join( const char sep, const std::initializer_list<std::string> &parts )
{
   const int len = std::accumulate( parts.begin(), parts.end(), static_cast<int>( parts.size() ) - 1,
                                    []( const int acc, const std::string &s ) -> int { return acc + static_cast<int>( s.length() ); } );
   std::string res( len, sep );
   int i {};
   for( const std::string &part: parts )
   {
      for( int j {}; j < static_cast<int>( part.length() ); j++ )
         res[i++] = part[j];
      if( i < len ) i++;
   }
   return res;
}

bool starts_with( const std::string &s, const std::string &prefix )
{
   if( prefix.length() > s.length() ) return false;
   for( int i = 0; i < static_cast<int>( prefix.length() ); i++ )
   {
      if( s[i] != prefix[i] )
         return false;
   }
   return true;
}

bool ends_with( const std::string &s, const std::string &suffix )
{
   if( suffix.length() > s.length() ) return false;
   for( int i = 0; i < static_cast<int>( suffix.length() ); i++ )
   {
      if( s[s.length() - 1 - i] != suffix[suffix.length() - 1 - i] )
         return false;
   }
   return true;
}

std::string quoteWhitespaceDir( const std::string &s, const char sep, const char quotechar )
{
   if( !strContains( s, ' ' ) ) return s;
   std::string s2 {};
   int ix {};
   for( const auto &part: split( s, sep ) )
   {
      if( ix++ > 0 || s.front() == sep ) s2 += sep;
      s2 += strContains( part, ' ' ) ? quotechar + part + quotechar : part;
   }
   return s.back() == sep ? s2 + sep : s2;
}

std::string doubleToString( const double v, const int width, const int precision )
{
   std::stringstream ss;
   ss.precision( precision );
   ss << std::fixed << v;
   std::string res = ss.str();
   return static_cast<int>( res.length() ) >= width ? res : std::string( width - static_cast<int>( res.length() ), ' ' ) + res;
}

bool strToBool( const std::string &s )
{
   if( s.empty() || s.length() > 4 ) return false;
   return in( s, "1"s, "true"s, "on"s, "yes"s );
}

std::optional<std::list<BinaryDiffMismatch>>
binaryFileDiff( const std::string &filename1, const std::string &filename2, int countLimit )
{
   if( countLimit == -1 ) countLimit = std::numeric_limits<int>::max();
   std::ifstream f1 { filename1, std::ios::binary }, f2 { filename2, std::ios::binary };
   std::list<BinaryDiffMismatch> mismatches {};
   char c1, c2;
   uint64_t offset {};
   while( !f1.eof() && !f2.eof() )
   {
      f1.get( c1 );
      f2.get( c2 );
      if( c1 != c2 )
      {
         mismatches.emplace_back( offset, c1, c2 );
         if( static_cast<int>( mismatches.size() ) >= countLimit )
            break;
      }
      offset++;
   }
   return mismatches.empty() ? std::nullopt : std::make_optional( mismatches );
}

std::string asdelphifmt( const double v, const int precision )
{
   std::stringstream ss;
   ss.precision( precision );
   ss << v;
   std::string s { replaceSubstrs( replaceSubstrs( ss.str(), "+", "" ), "-0", "-" ) };
   replaceChar( 'e', 'E', s );
   return s;
}

void stocp( const std::string &s, char *cp )
{
   std::memcpy( cp, s.c_str(), s.length() + 1 );
}

int strCompare( const std::string_view S1, const std::string_view S2, const bool caseInsensitive )
{
   if( S1.empty() || S2.empty() ) return static_cast<int>( !S1.empty() ) - static_cast<int>( !S2.empty() );
   auto L = S1.length();
   if( L > S2.length() ) L = S2.length();
   for( size_t K {}; K < L; K++ )
   {
      const int c1 = static_cast<unsigned char>(caseInsensitive ? toupper( S1[K] ) : S1[K]);
      const int c2 = static_cast<unsigned char>(caseInsensitive ? toupper( S2[K] ) : S2[K]);
      if( const int d = c1 - c2 )
         return d;
   }
   return static_cast<int>( S1.length() - S2.length() );
}

StringBuffer::StringBuffer( const int size ) : s( size, '\0' ), bufferSize { size } {}

char *StringBuffer::getPtr() { return &s[0]; }

std::string *StringBuffer::getStr()
{
   s.resize( strlen( s.data() ) );
   return &s;
}

int StringBuffer::getBufferSize() const { return bufferSize; }

BinaryDiffMismatch::BinaryDiffMismatch( const uint64_t offset, const uint8_t lhs, const uint8_t rhs )
   : offset( offset ), lhs( lhs ), rhs( rhs ) {}

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

} // namespace utils

// Needed for peak working set size query
#if defined( _WIN32 )
#include <windows.h>
#include <Psapi.h>
#include <processthreadsapi.h>
#endif

namespace utils
{
int64_t queryPeakRSS()
{
#if defined( _WIN32 )
   PROCESS_MEMORY_COUNTERS info;
   if( !GetProcessMemoryInfo( GetCurrentProcess(), &info, sizeof( info ) ) )
      return 0;
   return static_cast<int64_t>( info.PeakWorkingSetSize );
#elif defined( __linux )
   std::ifstream ifs { "/proc/self/status" };
   if( !ifs.is_open() ) return 0;
   std::string line;
   while( !ifs.eof() )
   {
      std::getline( ifs, line );
      if( starts_with( line, "VmHWM" ) )
      {
         auto parts = split( line );
         return std::stoi( nthRef( parts, 1 ) );
      }
   }
   return 0;
#elif defined( __APPLE__ )
   return 0;
#endif
}

void copy_to_uppercase( const std::string &s, char *buf )
{
   int j {};
   for( const char c : s )
      buf[j++] = toupper( c );
   buf[j] = '\0';
}

void copy_to_uppercase( const char *s, char *buf )
{
   int j {};
   for( const char *c = s; *c != '\0'; c++ )
      buf[j++] = toupper( *c );
   buf[j] = '\0';
}

std::string IntToStrW( const int n, const int w, const char blankChar )
{
   if( w < 0 || w > 255 ) return ""s;
   std::string t = rtl::sysutils_p3::IntToStr( n );
   return static_cast<int>( t.length() ) < w ? std::string( w - static_cast<int>( t.length() ), blankChar ) + t : t;
}

void trimLeft( std::string &s )
{
   size_t i;
   for( i = 0; i < s.length(); i++ )
      if( !std::isspace( s[i] ) )
         break;
   s.erase( 0, i );
}

double frac( const double x )
{
   return x - std::trunc(x);
}

void getline( FILE *f, std::string &s )
{
   constexpr int bsize {512};
   std::array<char, bsize> buf;
   s.clear();
   while(std::fgets( buf.data(), bsize, f )  && !ferror( f ))
   {
      s += buf.data();
      if(!s.empty() && s.back() == '\n')
         break;
   }
}

std::string getline( FILE *f )
{
   constexpr int bsize {512};
   std::array<char, bsize> buf {};
   std::string s;
   while(std::fgets( buf.data(), bsize, f ) && !ferror( f ))
   {
      s += buf.data();
      if(!s.empty() && s.back() == '\n')
         break;
   }
   return s;
}

std::string strInflateWidth( const int num, const int targetStrLen, const char inflateChar )
{
   auto s = rtl::sysutils_p3::IntToStr( num );
   const auto l = s.length();
   if( l >= static_cast<size_t>( targetStrLen ) ) return s;
   return std::string( targetStrLen - l, inflateChar ) + s;
}

} // namespace utils
