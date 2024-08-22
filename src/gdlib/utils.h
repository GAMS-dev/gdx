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

#include <algorithm>
#include <array>
#include <functional>
#if !defined(NDEBUG) || defined(__IN_CPPMEX__)
#include <iostream>
#endif
#include <iterator>
#include <initializer_list>
#include <unordered_set>
#include <list>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <map>
#include <optional>
#include <cstring>
#include <cstdint>
#include <numeric>
#include <bitset>
#include <cassert>// for assert

#ifndef _WIN32
#include <strings.h>// for strcasecmp
#endif

// ==============================================================================================================
// Interface
// ==============================================================================================================
#if !defined(NDEBUG) || defined(__IN_CPPMEX__)
#define debugStream std::cout
#else
extern std::stringstream debugStream;
#endif

namespace utils
{

template<typename T, int card>
class bsSet
{
   std::bitset<card> hasSym {};

public:
   bsSet() = default;

   bsSet( const bsSet &other ) : hasSym( other.hasSym )
   {
   }

   bsSet( const std::initializer_list<T> &syms )
   {
      for( const T s: syms )
         hasSym.set( s );
   }

   bool empty() const {
      for( int s {}; s < card; s++ )
         if( hasSym[s] ) return false;
      return true;
   }

   template<typename... Cdr>
   void insert( const T s, Cdr... cdr )
   {
      hasSym.set( s );
      insert( cdr... );
   }
   void insert() {}

   void insert( const T s )
   {
      hasSym.set( s );
   }

   void insert( const std::initializer_list<T> &syms )
   {
      for( const T s: syms )
         hasSym.set( s );
   }

   void insertVec( const std::vector<T> &syms )
   {
      for( const T s: syms )
         hasSym.set( s );
   }

   template<int count>
   void insertVec( const std::array<T, count> &syms )
   {
      for( const T s: syms )
         hasSym.set( s );
   }

   void eraseVec( const std::vector<T> &syms )
   {
      for( const T s: syms )
         hasSym.reset( s );
   }

   bool contains( const T s ) const
   {
      return hasSym[s];
   }

   void erase( const T s )
   {
      hasSym.reset( s );
   }

   template<typename... Cdr>
   void erase( const T s, Cdr... cdr )
   {
      hasSym.reset( s );
      erase( cdr... );
   }
   void erase() {}

   void clear()
   {
      hasSym.reset();
   }

   bsSet &operator=( const bsSet &other )
   {
      hasSym = other.hasSym;
      return *this;
   }
};

class charset {
   std::bitset<256> chars {};
   constexpr static int offset{128};

public:
   charset(const std::initializer_list<char> cs) {
      for(char c : cs)
         insert(c);
   }
   charset(const charset &other) = default;
   charset() = default;

   void insert(char c) {
      chars.set(c+offset);
   }

   void insert(const charset &other) {
      for(int i{}; i<256; i++)
         if(other.chars[i])
            chars.set(i);
   }

   [[nodiscard]] bool contains(char c) const {
      return chars[c+offset];
   }

   void clear() {
      chars.reset();
   }

   void erase(char c) {
      chars.reset(c+offset);
   }
};

inline char toupper( const char c )
{
   return c >= 'a' && c <= 'z' ? static_cast<char>( c ^ 32 ) : c;
}

inline char tolower( const char c )
{
   return c >= 'A' && c <= 'Z' ? static_cast<char>( c ^ 32 ) : c;
}

inline charset unionOp(const charset &a, const charset &b) {
   charset res{a};
   res.insert(b);
   return res;
}

inline void insertAllChars( charset &charset, const std::string_view chars )
{
   for(char c : chars)
      charset.insert(c);
}

inline void charRangeInsert( charset &charset, const char lbIncl, const char ubIncl )
{
   //for (char c : std::ranges::iota_view{ lbIncl, ubIncl + 1 })
   for( char c = lbIncl; c <= ubIncl; c++ )
      charset.insert( c );
}

template<class T>
bool in( const T &val, const std::vector<T> &elems )
{
   return std::find( elems.begin(), elems.end(), val ) != elems.end();
}

inline bool in( char c, const charset &elems )
{
   return elems.contains(c);
}

template<typename T>
bool in( const T &val,
         const T &last )
{
   return val == last;
}

template<typename T, typename... Args>
bool in( const T &val,
         const T &first,
         Args... rest )
{
   return val == first || in( val, rest... );
}

template<typename T>
bool in( const T &val, const std::set<T> &elems )
{
   // C++20 starts offering contains method
   return elems.find( val ) != elems.end();
}

template<typename T, int card>
bool in( const T &val, const bsSet<T, card> &elems )
{
   return elems.contains( val );
}

template<typename K, typename V>
bool in( const K &val, const std::map<K, V> &m )
{
   // C++20 starts offering contains method
   return m.find( val ) != m.end();
}

template<typename K, typename V>
bool in( const K &val, const std::unordered_set<K, V> &m )
{
   // C++20 starts offering contains method
   return m.find( val ) != m.end();
}

template<typename T>
class IContainsPredicate
{
public:
   virtual ~IContainsPredicate() = default;
   [[nodiscard]] virtual bool contains( const T &elem ) const = 0;
};

template<typename T>
bool in( const T &val,
         const IContainsPredicate<T> &coll )
{
   return coll.contains( val );
}

inline void charRangeInsertIntersecting( charset &charset, const char lbIncl, const char ubIncl, const class charset &other )
{
   //for (char c : std::ranges::iota_view{ lbIncl, ubIncl + 1 })
   for( char c = lbIncl; c <= ubIncl; c++ )
      if( utils::in( c, other ) )
         charset.insert( c );
}

template<class T>
bool any( std::function<bool( const T & )> predicate, const std::initializer_list<T> &elems )
{
   return std::any_of( std::cbegin( elems ), std::cend( elems ), predicate );
}

bool anychar( const std::function<bool( char )> &predicate, std::string_view s );

template<typename T, int count, const int notFound = -1>
int indexOf( const std::array<T, count> &arr, const T &elem )
{
   for( int i = 0; i < count; i++ )
      if( arr[i] == elem )
         return i;
   return notFound;
}

template<typename T, const int notFound = -1>
int indexOf( const std::vector<T> &elems, const T &elem )
{
   int i {};
   for( const T &other: elems )
   {
      if( other == elem )
         return i;
      i++;
   }
   return notFound;
}

template<typename T, const int notFound = -1>
int indexOf( const std::list<T> &elems, const T &elem )
{
   int i {};
   for( const T &other: elems )
   {
      if( other == elem )
         return i;
      i++;
   }
   return notFound;
}

template<typename T, int count, const int notFound = -1>
int indexOf( const std::array<T, count> &arr, std::function<bool( const T & )> predicate )
{
   for( int i {}; i < count; i++ )
      if( predicate( arr[i] ) ) return i;
   return notFound;
}

template<typename T, const int notFound = -1>
int indexOf( const std::vector<T> &elems, std::function<bool( const T & )> predicate )
{
   int i {};
   for( const T &elem: elems )
   {
      if( predicate( elem ) ) return i;
      i++;
   }
   return notFound;
}

template<typename T, const int notFound = -1>
int indexOf( const std::list<T> &elems, std::function<bool( const T & )> predicate )
{
   int i {};
   for( const T &elem: elems )
   {
      if( predicate( elem ) ) return i;
      i++;
   }
   return notFound;
}

int indexOf( const std::string &s, char c );

template<typename A, typename B, const int notFound = -1>
int pairIndexOfFirst( const std::vector<std::pair<A, B>> &elems, A &a )
{
   int i {};
   for( const auto &[aa, b]: elems )
   {
      if( aa == a ) return i;
      i++;
   }
   return notFound;
}

template<typename T>
inline auto nth( const std::list<T> &elems, int n )
{
   return *( std::next( elems.begin(), n ) );
}

template<typename T>
auto &nthRef( std::list<T> &elems, int n )
{
   return *( std::next( elems.begin(), n ) );
}

template<typename T>
const auto &nthRefConst( const std::list<T> &elems, int n )
{
   return *( std::next( elems.begin(), n ) );
}

template<typename T>
const auto &nthRefConst( const std::vector<T> &elems, int n )
{
   return elems[n];
}

template<typename T>
auto &nthRef( std::vector<T> &elems, int n )
{
   return elems[n];
}

template<typename T>
auto nth( const std::initializer_list<T> &elems, int n )
{
   return *( std::next( elems.begin(), n ) );
}

template<typename T>
void append( std::list<T> &l, const std::initializer_list<T> &elems )
{
   std::copy( elems.begin(), elems.end(), std::back_inserter( l ) );
}

void permutAssign( std::string &lhs, const std::string &rhs,
                   const std::vector<int> &writeIndices, const std::vector<int> &readIndices );

inline charset multiCharSetRanges( std::initializer_list<std::pair<char, char>> lbUbInclCharPairs )
{
   charset res;
   for( const auto &[lb, ub]: lbUbInclCharPairs )
      charRangeInsert( res, lb, ub );
   return res;
}

std::string strInflateWidth( const int num, const int targetStrLen, const char inflateChar =  ' ');

void removeTrailingCarriageReturnOrLineFeed( std::string &s );

std::string uppercase( std::string_view s );
std::string lowercase( std::string_view s );

bool sameTextInvariant( std::string_view a,
                        std::string_view b );

template<const bool caseInvariant = true>
bool sameText( const std::string_view a, const std::string_view b )
{
   return caseInvariant ? sameTextInvariant( a, b ) : a == b;
}

bool sameTextAsAny( std::string_view a, const std::initializer_list<std::string_view> &bs );
bool sameTextPrefix( std::string_view s, std::string_view prefix );

// Port of PStr(U)Equal
template<const bool caseInvariant = true>
bool sameTextPChar( const char *a,
                    const char *b )
{
   if( !a || !b ) return !a && !b;
   if constexpr( !caseInvariant ) return !std::strcmp( a, b );
#if defined( _WIN32 )
   return !_stricmp( a, b );
#else
   return !strcasecmp( a, b );
#endif
}

std::string_view trim( std::string_view s );

std::string getLineWithSep( std::istream &fs );

void getline( FILE *f, std::string &s );
std::string getline( FILE *f );

inline void fputstr(FILE* f, std::string_view s) {
   fwrite( s.data(), sizeof( char ), s.length(), f );
}

std::string trim( const std::string &s );
std::string trimRight( const std::string &s );
void trimRight( const std::string &s, std::string &storage );
const char *trimRight( const char *s, char *storage, int &slen );
std::string trimZeroesRight( const std::string &s, char DecimalSep = '.' );

bool hasCharLt( std::string_view s, int n );

double round( double n, int ndigits );

// since std::round behaves differently from Delphi's System.Round
template<class T>
T round( const double n)
{
   return static_cast<T>(n >= 0 ? n+0.5 : n-0.5);
}

void replaceChar( char a, char b, std::string &s );

std::vector<size_t> substrPositions( std::string_view s, std::string_view substr );
std::string replaceSubstrs( std::string_view s, std::string_view substr, std::string_view replacement );

std::string blanks( int n );
std::string zeros( int n );

int lastOccurence( std::string_view s, char c );

// Mimick val function of System unit in Delphi
void val( const std::string &s, double &num, int &code );
void val( const std::string &s, int &num, int &code );
void val( const char *s, int slen, int &num, int &code );
void val( const char *s, int slen, double &num, int &code );
double parseNumber( const std::string &s );
double parseNumber( const char *s );

void sleep( int milliseconds );

int strLenNoWhitespace( std::string_view s );

char &getCharAtIndexOrAppend( std::string &s, int ix );

bool strContains( std::string_view s, char c );

bool strContains( std::string_view s, const std::initializer_list<char> &cs );

template<typename T>
int genericCount( T start, std::function<T( T )> next, std::function<bool( T )> predicate )
{
   int acc {};
   for( T it = start; it; it = next( it ) )
      if( predicate( it ) ) acc++;
   return acc;
}

bool excl_or( bool a, bool b );

template<typename T>
std::vector<T> constructVec( int size, std::function<T( int )> elemForIndex )
{
   std::vector<T> elems( size );
   for( int i {}; i < size; i++ )
   {
      elems[i] = elemForIndex( i );
   }
   return elems;
}

template<const int lbIncl, const int ubIncl>
auto constructArrayRange()
{
   std::array<int, ubIncl - lbIncl + 1> res;
   std::iota( res.begin(), res.end(), lbIncl );
   return res;
}

std::string constructStr( int size, const std::function<char( int )> &charForIndex );

int posOfSubstr( std::string_view sub, std::string_view s );

std::list<std::string> split( std::string_view s, char sep = ' ' );

std::list<std::string> splitWithQuotedItems( std::string_view s );

std::string slurp( const std::string &fn );
void spit( const std::string &fn, const std::string &contents );

void assertOrMsg( bool condition, const std::string &msg );

std::string_view substr( std::string_view s, int offset, int len );

std::string join( char sep, const std::initializer_list<std::string> &parts );

bool ends_with( const std::string &s, const std::string &suffix );

bool starts_with( const std::string &s, const std::string &prefix );

std::string quoteWhitespace( const std::string &s, char quotechar = '\'' );

std::string quoteWhitespaceDir( const std::string &s, char sep, char quotechar = '\"' );

bool hasNonBlank( std::string_view s );

std::string doubleToString( double v, int width, int precision );

class StringBuffer
{
   std::string s;
   int bufferSize;

public:
   explicit StringBuffer( int size = BUFSIZ );
   char *getPtr();
   std::string *getStr();
   [[nodiscard]] int getBufferSize() const;
};

template<typename A, typename B>
std::optional<A> keyForValue( const std::map<A, B> &mapping, const B &value )
{
   for( const auto &[k, v]: mapping )
   {
      if( v == value ) return k;
   }
   return std::nullopt;
}

bool strToBool( const std::string &s );

// TODO: This should be more general and work with any sequential collection
template<typename T, const int size>
void assignRange( std::array<T, size> &arr, const int lbIncl, const int ubIncl, T value )
{
   std::fill_n( arr.begin() + lbIncl, ubIncl - lbIncl + 1, value );
}


struct BinaryDiffMismatch {
   BinaryDiffMismatch( uint64_t offset, uint8_t lhs, uint8_t rhs );

   uint64_t offset;
   uint8_t lhs, rhs;
};

std::optional<std::list<BinaryDiffMismatch>> binaryFileDiff( const std::string &filename1, const std::string &filename2, int countLimit = -1 );

template<class T, const int size>
std::array<T, size> arrayWithValue( T v )
{
   std::array<T, size> res;
   res.fill( v );
   return res;
}

std::string asdelphifmt( double v, int precision = 8 );

// Do not use this in inner-loop performance critical code!
template<typename T, const int card>
std::array<T, card> asArray( const T *ptr )
{
   std::array<T, card> a {};
   for( int i = 0; i < card; i++ )
      a[i] = ptr[i];
   return a;
}

template<typename T, const int card>
std::array<T, card> asArrayN( const T *ptr, const int n )
{
   std::array<T, card> a {};
   for( int i = 0; i < std::min<int>( n, card ); i++ )
      a[i] = ptr[i];
   return a;
}

void stocp( const std::string &s, char *cp );

template<typename A, typename B>
A reduce( const std::vector<B> &elems, A initial, const std::function<A( A, B )> combine )
{
   A acc { initial };
   for( const auto &elem: elems )
   {
      acc = combine( acc, elem );
   }
   return acc;
}

int strCompare( std::string_view S1, std::string_view S2, bool caseInsensitive = true );

constexpr int maxBOMLen = 4;
using tBomIndic = std::array<uint8_t, maxBOMLen>;
bool checkBOMOffset( const tBomIndic &potBOM, int &BOMOffset, std::string &msg );
int strConvCppToDelphi( std::string_view s, char *delphistr );

int strCompare( const char *S1,
                const char *S2,
                bool caseInsensitive = true );

inline void assignStrToBuf( const std::string &s, char *buf, const int outBufSize = 256 )
{
   if( static_cast<int>( s.length() ) > outBufSize ) return;
#if defined( _WIN32 )
   std::memcpy( buf, s.c_str(), s.length() + 1 );
#else
   std::strcpy( buf, s.c_str() );
#endif
}

inline void assignPCharToBuf( const char *s, const size_t slen, char *buf, const size_t outBufSize = 256 )
{
   if( slen + 1 > outBufSize ) return;
   std::memcpy( buf, s, slen + 1 );
}

inline void assignPCharToBuf( const char *s, char *buf, const size_t outBufSize = 256 )
{
   size_t i;
   for( i = 0; i < outBufSize && s[i] != '\0'; i++ )
      buf[i] = s[i];
   buf[i == outBufSize ? i - 1 : i] = '\0';// truncate when exceeding
}

inline void assignViewToBuf( const std::string_view s,
                             char *buf,
                             const size_t outBufSize = 256 )
{
   if( s.length() + 1 > outBufSize ) return;
   std::memcpy( buf, s.data(), s.length() );
   buf[s.length()] = '\0';
}

inline char *NewString( const char *s,
                        const size_t slen )
{
   if( !s ) return nullptr;
   assert( s[slen] == '\0' );
   const auto buf { new char[slen + 1] };
   utils::assignPCharToBuf( s, slen, buf, slen + 1 );
   return buf;
}

inline char *NewString( const char *s,
                        const size_t slen,
                        size_t &memSize )
{
   char *buf { NewString( s, slen ) };
   memSize += slen + 1;
   return buf;
}

int64_t queryPeakRSS();

inline int ord( const char c )
{
   return static_cast<unsigned char>( c );
}

inline int pos( const char c, const std::string &s )
{
   const auto p { s.find( c ) };
   return p == std::string::npos ? 0 : static_cast<int>( p ) + 1;
}

void copy_to_uppercase( const std::string &s, char *buf );
void copy_to_uppercase( const char *s, char *buf );

std::string IntToStrW( int n, int w, char blankChar = ' ' );

void trimLeft( std::string &s );

template<int N, int firstValid=0>
inline int indexOfSameText(const std::array<std::string, N> &strs, const std::string &s) {
   int i{firstValid};
   for(const std::string &s2 : strs) {
      if(sameText(s, s2))
         return i;
      i++;
   }
   return firstValid-1;
}

}// namespace utils
