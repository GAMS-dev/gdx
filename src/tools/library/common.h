#ifndef GDX_LIBRARY_H
#define GDX_LIBRARY_H

#include <string>
#include <vector>

namespace library
{

void printErrorMessage( const std::string &message );

void printErrorMessageWithError( const std::string &message );

void assertWithMessage( bool expression, const std::string &message );

std::string gdxSpecialValuesStr( int i );

std::string gdxDataTypStr( int i );

std::string gdxDataTypStrL( int i );

std::string valTypStr( int i );

std::string varTypStr( int i );

std::string specialValueStr( int i );

std::vector<std::string> splitString( const std::string &string, char delimiter );

bool canBeQuoted( const char *s, size_t slen );

bool goodUELString( const char *s, size_t slen );

class AuditLine_t
{
   std::string
           system_name,
           audit_line;

   void setAuditLine();

public:
   AuditLine_t( const std::string &system_name );

   void setSystemName( const std::string &system_name );

   std::string getAuditLine() const;
};

}// namespace library

#endif//GDX_LIBRARY_H
