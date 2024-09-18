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
#include <functional>
#include <cmath>
#include <array>
#include <cassert>
#include <cstring>
#include <cstdint>

// Interface
namespace global::delphitypes
{

// According to Embarcadero docs
// http://docwiki.embarcadero.com/RADStudio/Sydney/en/Simple_Types_(Delphi)
using Byte = uint8_t; // unsigned char
using Word = uint16_t;// unsigned short
using LongWord = uint32_t; // unsigned
using Cardinal = uint32_t;// unsigned int
using Longint = int32_t;
using Int32 = int32_t;// int
using Int64 = int64_t;// long long
using Shortint = signed char;
using Integer = int;
using Smallint = int16_t;// short

// mimick Pascal "array[lbIncl..ubIncl] of T"
// internally uses std::array. lbIncl maps to 0, ubIncl maps to size()-1
template<typename T, int lbIncl, int ubIncl>
class OffsetArray : public std::array<T, ubIncl-lbIncl+1>
{
   // Hide direct buffer access as this could be error-prone
   T *data() { return nullptr; }
public:
   T& operator[]( const int ix) {
      assert( ix >= lbIncl && ix <= ubIncl && "Index must be in range!" );
      return std::array<T, ubIncl-lbIncl+1>::operator[](ix - lbIncl);
   }

   const T& operator[]( const int ix) const {
      assert( ix >= lbIncl && ix <= ubIncl && "Index must be in range!" );
      return std::array<T, ubIncl-lbIncl+1>::operator[](ix - lbIncl);
   }
};

template<typename T>
void FreeAndNil( T *&ptr )
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
using Longint = int32_t;
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

   // Number of elements from lower bound to upper bound (both inclusive)
   constexpr static T getCount()
   {
      return upperBoundIncl - lowerBoundIncl + 1;
   }

   T *getStorage()
   {
      return &value;
   }
};

inline double frac( const double v )
{
   return v - trunc( v );
}

}// namespace global::delphitypes
