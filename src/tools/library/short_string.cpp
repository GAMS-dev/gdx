#include <cstring>
#include <ostream>
#include <cassert>

#include "short_string.h"
#include "../../gdlib/utils.h"

// #define ENABLE_ASSERTIONS

namespace library
{

short_string::short_string()
{
   buffer.front() = '\0';
}

short_string::short_string( const char *s )
{
   const size_t s_length { std::strlen( s ) };
#if defined( ENABLE_ASSERTIONS )
   assert( s_length < GMS_SSSIZE );
#endif
   std::memcpy( buffer.data(), s, s_length );
   buffer[s_length] = '\0';
}

short_string::short_string( const std::string &s ) : short_string( s.data() ) {}

char *short_string::data()
{
   return buffer.data();
}

const char *short_string::data() const
{
   return buffer.data();
}

std::string short_string::string() const
{
   return buffer.data();
}

uint8_t short_string::length() const
{
   return static_cast<uint8_t>( strlen( buffer.data() ) );
}

char short_string::front() const
{
   const char c { buffer.front() };
#if defined( ENABLE_ASSERTIONS )
   assert( c != '\0' );
#endif
   return c;
}

char short_string::back() const
{
   const uint8_t length { this->length() };
#if defined( ENABLE_ASSERTIONS )
   assert( length > 0 );
#endif
   return buffer[length - 1];
}

char short_string::at( const uint8_t i ) const
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

char short_string::operator[]( const uint8_t i ) const
{
   return buffer[i];
}

bool short_string::empty() const
{
   return buffer.front() == '\0';
}

void short_string::clear()
{
   buffer.front() = '\0';
}

void short_string::append( const char c )
{
   const uint8_t length { this->length() };
#if defined( ENABLE_ASSERTIONS )
   assert( length + 1 < GMS_SSSIZE );
#endif
   buffer[length] = c;
   buffer[length + 1] = '\0';
}

void short_string::append( const char *s )
{
   const uint8_t length { this->length() };
   const size_t s_length { std::strlen( s ) };
#if defined( ENABLE_ASSERTIONS )
   assert( length + s_length < GMS_SSSIZE );
#endif
   std::memcpy( buffer.data() + length, s, s_length );
   buffer[length + s_length] = '\0';
}

void short_string::append( const short_string &s )
{
   append( s.data() );
}

void short_string::append( const std::string &s )
{
   append( s.data() );
}

void short_string::operator+=( const char c )
{
   append( c );
}

void short_string::operator+=( const char *s )
{
   append( s );
}

void short_string::operator+=( const short_string &s )
{
   append( s );
}

void short_string::operator+=( const std::string &s )
{
   append( s );
}

void short_string::to_upper_case()
{
   for( uint8_t i {}; buffer.at( i ) != '\0'; i++ )
      buffer[i] = static_cast<char>( utils::toupper( buffer[i] ) );
}

short_string &short_string::operator=( const std::string &s )
{
   const size_t s_length { s.length() };
#if defined( ENABLE_ASSERTIONS )
   assert( s_length < GMS_SSSIZE );
#endif
   std::memcpy( buffer.data(), s.data(), s_length );
   buffer[s_length] = '\0';
   return *this;
}

bool short_string::operator==( const char *s ) const
{
   return strcmp( buffer.data(), s ) == 0;
}

bool short_string::operator==( const short_string &s ) const
{
   return *this == s.data();
}

bool short_string::operator==( const std::string &s ) const
{
   return *this == s.data();
}

bool short_string::operator<( const short_string &s ) const
{
   return strcmp( buffer.data(), s.data() ) < 0;
}

std::ostream &operator<<( std::ostream &os, const short_string &s )
{
   return os << s.string();
}

}// namespace library
