#pragma once

#include <string>
#include <array>
#include <gclgms.h>

namespace strindexbuf
{
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

   [[nodiscard]] const char *c_str() const
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

   bool operator==( const std::string &other ) const
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
      for( int i {}; i < static_cast<int>( bufPtrs.size() ); i++ )
      {
         bufPtrs[i] = bufContents[i].data();
         if( strIndex )
            std::memcpy( bufPtrs[i], ( *strIndex )[i].c_str(), ( *strIndex )[i].length() + 1 );
      }
   }

   StrRef operator[]( const int index ) const
   {
      return StrRef { bufPtrs[index] };
   }

   char **ptrs() { return bufPtrs.data(); }
   const char **cptrs() { return const_cast<const char **>( bufPtrs.data() ); }

   void clear()
   {
      for( int i {}; i < static_cast<int>( bufContents.size() ); i++ )
         bufContents[i].fill( 0 );
   }

   [[nodiscard]] StrRef front() const
   {
      return StrRef { bufPtrs[0] };
   }
};

}