#ifndef GDX_LIBRARY_H
#define GDX_LIBRARY_H

#include <string>
#include <vector>

namespace library
{

void printErrorMessage( const std::string &message, bool printError = false );

void assertWithMessage( bool expression, const std::string &message );

std::string gdxSpecialValuesStr( int i );

std::string gdxDataTypStr( int i );

std::string gdxDataTypStrL( int i );

std::string valTypStr( int i );

std::string varTypStr( int i );

std::string specialValueStr( int i );

std::vector<std::string> splitString( const std::string &string, char delimiter );

bool CanBeQuoted( const char *s, size_t slen );

bool GoodUELString( const char *s, size_t slen );

}// namespace library

#endif//GDX_LIBRARY_H
