#include <cassert>
#include <cstring>
#include <cctype>
#include <ostream>

#include "short_string.h"

namespace library
{

short_string::short_string() = default;

short_string::short_string( const char *s )
{
   const size_t s_length { std::strlen( s ) };
   assert( s_length < MAX_LENGTH );
   std::memcpy( buffer.data(), s, s_length );
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
   uint8_t i {};
   while( buffer[i] != '\0' ) i++;
   return i;
}

char short_string::front() const
{
   const char c { buffer.front() };
   assert( c != '\0' );
   return c;
}

char short_string::back() const
{
   const uint8_t length { this->length() };
   assert( length > 0 );
   return buffer[length - 1];
}

char short_string::operator[]( const uint8_t i ) const
{
   assert( i < length() );
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
   assert( length + 1 < MAX_LENGTH );
   buffer[length] = c;
   buffer[length + 1] = '\0';
}

void short_string::append( const char *s )
{
   const uint8_t length { this->length() };
   const size_t s_length { std::strlen( s ) };
   assert( length + s_length < MAX_LENGTH );
   std::memcpy( buffer.data() + length, s, s_length );
   buffer[length + s_length] = '\0';
}

void short_string::append( const std::string &s )
{
   append( s.data() );
}

void short_string::append( const short_string &s )
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

void short_string::operator+=( const std::string &s )
{
   append( s );
}

void short_string::operator+=( const short_string &s )
{
   append( s );
}

void short_string::to_upper_case()
{
   uint8_t i {};
   while( buffer[i] != '\0' )
   {
      buffer[i] = static_cast<char>( std::toupper( buffer[i] ) );
      i++;
   }
}

short_string &short_string::operator=( const std::string &s )
{
   const size_t s_length { s.length() };
   assert( s_length < MAX_LENGTH );
   std::memcpy( buffer.data(), s.data(), s_length );
   buffer[s_length] = '\0';
   return *this;
}

bool short_string::operator==( const std::string &s ) const
{
   uint8_t i {};
   while( buffer[i] != '\0' && s[i] != '\0' )
   {
      if( buffer[i] != s[i] ) return false;
      i++;
   }
   return buffer[i] == s[i];
}

bool short_string::operator<( const short_string &s ) const
{
   return strcmp( data(), s.data() ) < 0;
}

std::ostream &operator<<( std::ostream &os, const short_string &s )
{
   return os << s.string();
}

}// namespace library
