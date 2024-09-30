#ifndef GDX_SHORT_STRING_H
#define GDX_SHORT_STRING_H

#include <array>
#include <cstdint>
#include <string>

#include "../../../generated/gclgms.h"

namespace library
{

class short_string
{
   std::array<char, GMS_SSSIZE> buffer;

public:
   short_string();
   explicit short_string( const char *s );
   explicit short_string( const std::string &s );

   char *data();
   [[nodiscard]] const char *data() const;
   [[nodiscard]] std::string string() const;
   [[nodiscard]] operator std::string() const;

   [[nodiscard]] uint8_t length() const;

   [[nodiscard]] char front() const;
   [[nodiscard]] char back() const;

   [[nodiscard]] char at( uint8_t i ) const;
   [[nodiscard]] char operator[]( uint8_t i ) const;

   [[nodiscard]] bool empty() const;

   void clear();

   void append( char c );
   void append( const char *s );
   void append( const short_string &s );
   void append( const std::string &s );

   void operator+=( char c );
   void operator+=( const char *s );
   void operator+=( const short_string &s );
   void operator+=( const std::string &s );

   void to_upper_case();

   short_string &operator=( const std::string &s );

   [[nodiscard]] bool operator==( const char *s ) const;
   [[nodiscard]] bool operator==( const short_string &s ) const;
   [[nodiscard]] bool operator==( const std::string &s ) const;

   [[nodiscard]] bool operator<( const short_string &s ) const;

   friend std::ostream &operator<<( std::ostream &os, const short_string &s );

   friend std::string operator+( const std::string &lhs, const short_string &rhs );
   friend std::string operator+( const short_string &lhs, const std::string &rhs );

   friend std::string operator+( char lhs, const short_string &rhs );
   friend std::string operator+( const short_string &lhs, char rhs );
};

}// namespace library

#endif//GDX_SHORT_STRING_H
