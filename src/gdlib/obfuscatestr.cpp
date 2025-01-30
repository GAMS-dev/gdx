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


#include <cassert>
#include <memory>
#include "obfuscatestr.hpp"
#include "charmaps.hpp"
#include "utils.hpp"

using namespace gdlib::charmaps;

namespace gdlib::obfuscatestr
{
std::unique_ptr<TObfuscateCharMap> LabelCharMapPtr {}, SymbolCharMapPtr {};

TObfuscateStrings::TObfuscateStrings( int len ) : FLength { len },
                                                  FBuf( len + 1 ),
                                                  FNBuf( len )
{
   assert( len > 0 && "*** TObfuscateStrings.Create len<=0" );
}

bool TObfuscateStrings::GetObfuscateString( std::string &obfuscateString )
{
   if( !FCharMapPtr ) return false;
   if( !FInit )
   {
      FInit = true;
      for( int i { 0 }; i < FLength; i++ )
      {
         FNBuf[i] = 0;
         FBuf[i] = FCharMapPtr->FCharmap.front();
      }
      FBuf.front() = FCharMapPtr->FFCharMap.front();
      if( FLength == 1 )
      {
         obfuscateString = FBuf.front();
         return true;
      }
   }
   bool changeFirstChar { true };
   for( int i { FLength - 1 }; i >= 1; i-- )
   {
      if( FNBuf[i] == FCharMapPtr->FCharmap.size() - 1 )
      {
         FNBuf[i] = 0;
         FBuf[i] = FCharMapPtr->FCharmap.front();
      }
      else
      {
         FNBuf[i]++;
         FBuf[i] = FCharMapPtr->FCharmap[FNBuf[i]];
         changeFirstChar = false;
         break;
      }
   }

   if( changeFirstChar )
   {
      if( FNBuf.front() == FCharMapPtr->FFCharMap.size() - 1 ) return false;
      FNBuf.front()++;
      FBuf.front() = FCharMapPtr->FFCharMap[FNBuf.front()];
   }

   if( FFlipIt )
   {
      obfuscateString.resize( FLength );
      for( int i { FLength - 1 }; i >= 0; i-- ) obfuscateString[FLength - i - 1] = FBuf[i];
   }
   else
      obfuscateString.assign( FBuf.data(), FLength );
   return true;
}

void obfuscateInit()
{
   InitCharacterMaps();

   if( !SymbolCharMapPtr )
   {
      int fcharmapLen {};
      for( char c = std::numeric_limits<char>::min(); c < std::numeric_limits<char>::max(); c++ )
         if( IsIdentChar( c ) && !utils::in( c, lowletter ) ) fcharmapLen++;
      const int ffcharmapLen = 'Z' - 'A' + 1;

      SymbolCharMapPtr = std::make_unique<TObfuscateCharMap>( fcharmapLen, ffcharmapLen );
      for( char c { 'A' }; c <= 'Z'; c++ )
         SymbolCharMapPtr->FFCharMap[c - 'A'] = c;
      int i {};
      for( char c = std::numeric_limits<char>::min(); c < std::numeric_limits<char>::max(); c++ )
         if( IsIdentChar( c ) && !utils::in( c, lowletter ) )
            SymbolCharMapPtr->FCharmap[i++] = c;
   }

   if( !LabelCharMapPtr )
   {
      // 33 - 126, 128 - 254, 32 - "a-z"
      const int fcharmapLen = 254 - 32 - ( 'z' - 'a' + 1 ) - 1, ffcharmapLen = fcharmapLen - 1;
      LabelCharMapPtr = std::make_unique<TObfuscateCharMap>( fcharmapLen, ffcharmapLen );
      int k {};
      for( uint8_t i { 33 }; i <= 254; i++ )
      {
         if( i != 127 && !( ( i >= 'a' && i <= 'z' ) || i == '\'' ) )
         {
            LabelCharMapPtr->FCharmap[k] = i;
            LabelCharMapPtr->FFCharMap[k++] = i;
         }
      }
      LabelCharMapPtr->FCharmap[k] = 32;
      assert( LabelCharMapPtr->FCharmap.size() == LabelCharMapPtr->FFCharMap.size() + 1 && "*** wrong char calc" );
   }
}

void obfuscateFini()
{
}

TObfuscateLabelName::TObfuscateLabelName( int len ) : TObfuscateStrings( len )
{
   FCharMapPtr = LabelCharMapPtr.get();
   FFlipIt = true;
}

TObfuscateCharMap::TObfuscateCharMap( int fcharmapLen, int ffcharmapLen ) : FCharmap( fcharmapLen ), FFCharMap( ffcharmapLen ) {}

TObfuscateSymbolName::TObfuscateSymbolName( int len ) : TObfuscateStrings { len }
{
   FCharMapPtr = SymbolCharMapPtr.get();
   FFlipIt = false;
}
}// namespace gdlib::obfuscatestr