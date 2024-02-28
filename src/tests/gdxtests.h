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

#include <functional>// for function
#include <string>    // for string, operator""s
#include <vector>    // for vector
#include <array>     // for array, array<>::value_type
#include <cassert>   // for assert

namespace gdx::tests::gdxtests
{

bool setEnvironmentVar( const std::string &name, const std::string &val );
void unsetEnvironmentVar( const std::string &name );

void basicTest( const std::function<void( TGXFileObj & )> &cb );
void testRead( const std::string &filename, const std::function<void( TGXFileObj & )> &cb );
void testWrite( const std::string &filename, const std::function<void( TGXFileObj & )> &cb );

void writeMappedRecordsOutOfOrder( TGXFileObj &pgx );
void domainSetGetTestSetupPrefix( TGXFileObj &pgx );
std::string acquireGDXforModel( const std::string &model );
void commonSetGetDomainTests( const std::vector<std::string> &domainNames, const std::vector<int> &domainIndices );
void testReadModelGDX( const std::string &model, const std::function<void( TGXFileObj & )> &func );
void testWithCompressConvert( bool compress, const std::string &convert );

class StrRef
{
   char *s;

public:
   explicit StrRef( char *_s ) : s( _s ) {}

   StrRef &operator=( const std::string &other )
   {
      std::memcpy( s, other.c_str(), sizeof( char ) * ( other.length() + 1 ) );
      return *this;
   }

   const char *c_str()
   {
      return s;
   }

   [[nodiscard]] bool empty() const
   {
      return s[0] == '\0';
   }

   explicit operator std::string() const
   {
      std::string res;
      res.assign( s );
      return res;
   }

   [[nodiscard]] std::string str() const
   {
      std::string res;
      res.assign( s );
      return res;
   }

   bool operator==( const std::string &other )
   {
      return !std::strcmp( other.c_str(), s );
   }
};

using TgdxStrIndex = std::array<std::string, GMS_MAX_INDEX_DIM>;

class StrIndexBuffers
{
   std::array<std::array<char, GMS_SSSIZE>, GMS_MAX_INDEX_DIM> bufContents {};
   std::array<char *, GMS_MAX_INDEX_DIM> bufPtrs {};

public:
   explicit StrIndexBuffers( const TgdxStrIndex *strIndex = nullptr )
   {
      for( int i {}; i < (int) bufPtrs.size(); i++ )
      {
         bufPtrs[i] = bufContents[i].data();
         if( strIndex )
            std::memcpy( bufPtrs[i], ( *strIndex )[i].c_str(), ( *strIndex )[i].length() + 1 );
      }
   }

   StrRef operator[]( int index )
   {
      return StrRef { bufPtrs[index] };
   }

   char **ptrs() { return bufPtrs.data(); }
   const char **cptrs() { return (const char **) bufPtrs.data(); }

   void clear()
   {
      for( int i {}; i < (int) bufContents.size(); i++ )
         bufContents[i].fill( 0 );
   }

   StrRef front()
   {
      return StrRef { bufPtrs[0] };
   }
};

}// namespace gdx::tests::gdxtests