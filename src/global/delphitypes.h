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
#include "modhead.h"
#include <stdexcept>
#include <string>
#include <sstream>
// only supported by MSVC so far :(
//#include <format>
#include <functional>
#include <set>
#include <cmath>
#include <array>
#include <cassert>
#include <cstring>
#include <cstdint>


// Interface
namespace global::delphitypes
{

class ShortStringHeap
{
   char *buf;

public:
   ShortStringHeap(const ShortStringHeap &other) {
      buf = new char[other.buf[0]+2];
      std::memcpy(buf, other.buf, sizeof(char)*buf[0]+2);
   }

   explicit ShortStringHeap( const char *s )
   {
      auto l { std::strlen( s ) };
      buf = new char[l + 2];
      buf[0] = static_cast<uint8_t>( l );
      std::memcpy( &buf[1], s, l + 1 );
   }

   ~ShortStringHeap()
   {
      delete[] buf;
   }

   [[nodiscard]] std::string string() const
   {
      return &buf[1];
   }

   [[nodiscard]] char *c_str() const
   {
      return &buf[1];
   }

   char *d_str()
   {
      return buf;
   }

   [[nodiscard]] uint8_t size() const
   {
      return (uint8_t) buf[0];
   }

   [[nodiscard]] bool empty() const
   {
      return !(uint8_t) buf[0];
   }

   bool operator==( const std::string &s ) const
   {
      if( s.length() != size() ) return false;
      for( int i {}; i < size(); i++ )
         if( s[i] != buf[i + 1] ) return false;
      return true;
   }

   ShortStringHeap &operator=( const std::string &s )
   {
      buf[0] = static_cast<uint8_t>( s.length() );
      std::memcpy( &buf[1], s.c_str(), s.length() + 1 );
      return *this;
   }
};

class ShortString
{
   uint8_t length {};
   std::array<char, 255> buf {};

public:
   explicit ShortString( const char *s ) : length { (uint8_t) std::strlen( s ) }
   {
      assert( std::strlen( s ) <= 254 );
      std::memcpy( buf.data(), s, length + 1 );
   }

   explicit ShortString( const std::string &s ) : ShortString( s.c_str() )
   {
   }

   [[nodiscard]] std::string string() const
   {
      return buf.data();
   }

   char *c_str()
   {
      return buf.data();
   }

   char *d_str()
   {
      return (char *) &length;
   }

   [[nodiscard]] uint8_t size() const
   {
      return length;
   }

   [[nodiscard]] bool empty() const
   {
      return !length;
   }

   bool operator==( const std::string &s ) const
   {
      if( s.length() != length ) return false;
      for( int i {}; i < length; i++ )
         if( s[i] != buf[i] ) return false;
      return true;
   }

   ShortString &operator=( const std::string &s )
   {
      length = (uint8_t) s.length();
      std::memcpy( buf.data(), s.c_str(), length + 1 );
      return *this;
   }
};

template<typename T>
inline void FreeAndNil( T *&ptr )
{
   if( ptr )
   {
      delete ptr;
      ptr = nullptr;
   }
}

// According to Embarcadero docs
// http://docwiki.embarcadero.com/RADStudio/Sydney/en/Simple_Types_(Delphi)
using Byte = uint8_t; // unsigned char
using Word = uint16_t;// unsigned short
using LongWord = uint32_t; // unsigned
using Cardinal = uint32_t;// unsigned int
using Longint = int;
using Int32 = int32_t;// int
using Int64 = int64_t;// long long
using Shortint = signed char;
using Integer = int;
using Smallint = int16_t;// short

using tDateTime = double;
using Text = std::fstream*;

// Both bounds are inclusive
template<typename T, T lowerBoundIncl, T upperBoundIncl>
class Bounded
{
   T value;

public:
   Bounded() : value( lowerBoundIncl ) {}

   Bounded( T initialValue ) : value( initialValue )
   {
      checkBounds();
   }

   Bounded &operator=( const T &rhs )
   {
      value = rhs;
      checkBounds();
      return *this;
   }

   Bounded &operator+=( const T &rhs )
   {
      value += rhs;
      checkBounds();
      return *this;
   }

   Bounded &operator-=( const T &rhs )
   {
      value -= rhs;
      checkBounds();
      return *this;
   }

   operator T() const
   {
      return value;
   }

   // Prefix increment (apply and then read)
   Bounded &operator++()
   {
      value += 1;
      checkBounds();
      return *this;
   }

   // Prefix decrement (apply and then read)
   Bounded &operator--()
   {
      value -= 1;
      checkBounds();
      return *this;
   }

   // Postfix increment (read and then apply)
   Bounded operator++( int )
   {
      Bounded temp = *this;
      ++*this;
      return temp;
   }

   // Postfix decrement (read and then apply)
   Bounded operator--( int )
   {
      Bounded temp = *this;
      --*this;
      return temp;
   }

   inline void checkBounds()
   {
#if !defined(NDEBUG)
      if( value < lowerBoundIncl || value > upperBoundIncl )
      {
         std::stringstream msgStream;
         msgStream << value << " is out of bounded range [" << lowerBoundIncl << "," << upperBoundIncl << "]";
         throw std::out_of_range( msgStream.str() );
      }
#endif
   }

   constexpr static T getLowerBound()
   {
      return lowerBoundIncl;
   }

   constexpr static T getUpperBound()
   {
      return upperBoundIncl;
   }

   T *getStorage()
   {
      return &value;
   }
};

inline double frac( double v )
{
   return v - trunc( v );
}

}// namespace global::delphitypes