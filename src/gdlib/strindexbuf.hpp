#pragma once

#include <string>
#include <cstring>
#include <cassert>
#include <array>
#include <gclgms.h>

#ifndef GDX_NS
#define GDX_NS gdxlib::
#endif

namespace GDX_NS gdlib::strindexbuf
{
class StrRef
{
   char *s;

public:
   explicit StrRef( char *_s ) : s( _s ) {}

   StrRef &operator=( std::string_view other )
   {
      assert( other.length() <= GMS_SSSIZE - 1 );
      std::memmove( s, other.data(), other.length() );
      s[other.length()] = '\0';
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
      return std::string( s );
   }

   [[nodiscard]] std::string str() const
   {
      return std::string( s );
   }

   bool operator==( std::string_view other ) const
   {
      return other == s;
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
         {
            const auto &str = ( *strIndex )[i];
            std::memmove( bufPtrs[i], str.c_str(), str.length() + 1 );
         }
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

namespace gdlib {
namespace strindexbuf = GDX_NS gdlib::strindexbuf;
}
