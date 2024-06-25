#include <iostream>
#include <cassert>
#include <sstream>

#include "library.h"
// Global constants
#include "../../../generated/gclgms.h"

namespace library
{

std::string padLeft( const std::string &text, const int width, const char paddingChar )
{
   return std::string( std::max( width - static_cast<int>( text.length() ), 0 ), paddingChar ) + text;
}

std::string padRight( const std::string &text, const int width, const char paddingChar )
{
   return text + std::string( std::max( width - static_cast<int>( text.length() ), 0 ), paddingChar );
}

void printErrorMessage( const std::string &message, const bool printError )
{
   std::cerr << ( printError ? "Error: " : "" ) << message << std::endl;
}

void assertWithMessage( const bool expression, const std::string &message )
{
   if( !expression ) printErrorMessage( message );
   assert( expression );
}

std::string gdxSpecialValuesStr( const int i )
{
   if( i < 0 || i >= GMS_SVIDX_MAX )
   {
      assertWithMessage( false, "Unknown type" );
      return "Unknown";
   }
   else
      return gmsSVText[i];
}

std::string gdxDataTypStr( const int i )
{
   switch( i )
   {
      case 0:
         return "Set";
      case 1:
         return "Par";
      case 2:
         return "Var";
      case 3:
         return "Equ";
      case 4:
         return "Alias";
      default:
         assertWithMessage( false, "Unknown type" );
         return "Unknown";
   }
}

std::string gdxDataTypStrL( const int i )
{
   if( i < 0 || i >= GMS_DT_MAX )
   {
      assertWithMessage( false, "Unknown type" );
      return "Unknown";
   }
   else
      return gmsGdxTypeText[i];
}

std::string valTypStr( const int i )
{
   if( i < 0 || i >= GMS_VAL_MAX )
   {
      assertWithMessage( false, "Unknown type" );
      return "Unknown";
   }
   else
   {
      // TODO: Test this conversion
      std::string result { gmsValTypeText[i] };
      result.erase( 0, 1 );
      for( char &c: result ) c = static_cast<char>( toupper( c ) );
      return result;
   }
}

std::string varTypStr( const int i )
{
   if( i < 0 || i >= GMS_VARTYPE_MAX )
   {
      assertWithMessage( false, "Unknown type" );
      return "Unknown";
   }
   else
      return gmsVarTypeText[i];
}

std::string specialValueStr( const int i )
{
   if( i < 0 || i >= GMS_SVIDX_MAX )
   {
      assertWithMessage( false, "Unknown type" );
      return "Unknown";
   }
   else
      return gmsSVText[i];
}

std::vector<std::string> splitString( const std::string &s, const char delimiter )
{
   std::vector<std::string> tokens;
   std::istringstream iss( s );
   std::string token;
   while( getline( iss, token, delimiter ) )
   {
      tokens.push_back( token );
   }
   return tokens;
}

}// namespace library
