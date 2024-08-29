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


// Interface
namespace global::delphitypes
{

template<typename T>
void FreeAndNil( T *&ptr )
{
   if( ptr )
   {
      delete ptr;
      ptr = nullptr;
   }
}

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