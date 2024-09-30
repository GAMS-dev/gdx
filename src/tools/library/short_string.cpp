#include <cstring>
#include <ostream>
#include <cassert>

#include "short_string.h"
#include "../../gdlib/utils.h"

// #define ENABLE_ASSERTIONS

namespace library
{

ShortString_t::ShortString_t()
{
   buffer.front() = '\0';
}

ShortString_t::ShortString_t( const char *s )
{
   const size_t s_length { std::strlen( s ) };
#if defined( ENABLE_ASSERTIONS )
   assert( s_length < GMS_SSSIZE );
#endif
   std::memcpy( buffer.data(), s, s_length );
   buffer[s_length] = '\0';
}

ShortString_t::ShortString_t( const std::string &s ) : ShortString_t( s.data() ) {}

char *ShortString_t::data()
{
   return buffer.data();
}

const char *ShortString_t::data() const
{
   return buffer.data();
}

std::string ShortString_t::string() const
{
   return buffer.data();
}

ShortString_t::operator std::string() const
{
   return buffer.data();
}

uint8_t ShortString_t::length() const
{
   return static_cast<uint8_t>( strlen( buffer.data() ) );
}

char ShortString_t::front() const
{
   const char c { buffer.front() };
#if defined( ENABLE_ASSERTIONS )
   assert( c != '\0' );
#endif
   return c;
}

char ShortString_t::back() const
{
   const uint8_t length { this->length() };
#if defined( ENABLE_ASSERTIONS )
   assert( length > 0 );
#endif
   return buffer[length - 1];
}

char ShortString_t::at( const uint8_t i ) const
{
#if defined( ENABLE_ASSERTIONS )
   [[maybe_unused]] const uint8_t length { this->length() };
   if( i > 0 )
      assert( i < length );
   else
      assert( i <= length );
#endif
   return buffer[i];
}

char ShortString_t::operator[]( const uint8_t i ) const
{
   return buffer[i];
}

bool ShortString_t::empty() const
{
   return buffer.front() == '\0';
}

void ShortString_t::clear()
{
   buffer.front() = '\0';
}

void ShortString_t::append( const char c )
{
   const uint8_t length { this->length() };
#if defined( ENABLE_ASSERTIONS )
   assert( length + 1 < GMS_SSSIZE );
#endif
   buffer[length] = c;
   buffer[length + 1] = '\0';
}

void ShortString_t::append( const char *s )
{
   const uint8_t length { this->length() };
   const size_t s_length { std::strlen( s ) };
#if defined( ENABLE_ASSERTIONS )
   assert( length + s_length < GMS_SSSIZE );
#endif
   std::memcpy( buffer.data() + length, s, s_length );
   buffer[length + s_length] = '\0';
}

void ShortString_t::append( const ShortString_t &s )
{
   append( s.data() );
}

void ShortString_t::append( const std::string &s )
{
   append( s.data() );
}

void ShortString_t::operator+=( const char c )
{
   append( c );
}

void ShortString_t::operator+=( const char *s )
{
   append( s );
}

void ShortString_t::operator+=( const ShortString_t &s )
{
   append( s );
}

void ShortString_t::operator+=( const std::string &s )
{
   append( s );
}

void ShortString_t::to_upper_case()
{
   for( uint8_t i {}; buffer.at( i ) != '\0'; i++ )
      buffer[i] = static_cast<char>( utils::toupper( buffer[i] ) );
}

ShortString_t &ShortString_t::operator=( const std::string &s )
{
   const size_t s_length { s.length() };
#if defined( ENABLE_ASSERTIONS )
   assert( s_length < GMS_SSSIZE );
#endif
   std::memcpy( buffer.data(), s.data(), s_length );
   buffer[s_length] = '\0';
   return *this;
}

bool ShortString_t::operator==( const char *s ) const
{
   return strcmp( buffer.data(), s ) == 0;
}

bool ShortString_t::operator==( const ShortString_t &s ) const
{
   return *this == s.data();
}

bool ShortString_t::operator==( const std::string &s ) const
{
   return *this == s.data();
}

bool ShortString_t::operator<( const ShortString_t &s ) const
{
   return strcmp( buffer.data(), s.data() ) < 0;
}

std::ostream &operator<<( std::ostream &os, const ShortString_t &s )
{
   return os << s.data();
}

std::string operator+( const std::string &lhs, const ShortString_t &rhs )
{
   return lhs + rhs.data();
}

std::string operator+( const ShortString_t &lhs, const std::string &rhs )
{
   return lhs.data() + rhs;
}

std::string operator+( const char lhs, const ShortString_t &rhs )
{
   return lhs + rhs.string();
}

std::string operator+( const ShortString_t &lhs, const char rhs )
{
   return lhs.string() + rhs;
}

}// namespace library
