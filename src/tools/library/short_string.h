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
private:
   static constexpr int MAX_LENGTH = GMS_SSSIZE;
   std::array<char, MAX_LENGTH> buffer {};

public:
   short_string();
   explicit short_string( const char *s );
   explicit short_string( const std::string &s );

   char *data();
   [[nodiscard]] const char *data() const;
   [[nodiscard]] std::string string() const;

   [[nodiscard]] uint8_t length() const;

   [[nodiscard]] char front() const;
   [[nodiscard]] char back() const;

   char operator[]( uint8_t i ) const;

   [[nodiscard]] bool empty() const;

   void clear();

   void append( char c );
   void append( const char *s );
   void append( const std::string &s );
   void append( const short_string &s );

   void operator+=( char c );
   void operator+=( const char *s );
   void operator+=( const std::string &s );
   void operator+=( const short_string &s );

   void to_upper_case();

   short_string &operator=( const std::string &s );

   bool operator==( const std::string &s ) const;

   bool operator<( const short_string &s ) const;

   friend std::ostream &operator<<( std::ostream &os, const short_string &s );
};

}// namespace library

#endif//GDX_SHORT_STRING_H
