#include <iostream>
#include <cassert>
#include <sstream>

#include "common.h"
// Global constants
#include "../../../generated/gclgms.h"

namespace library
{

std::ostream *ErrorStream = &std::cout;
// TODO: Possible improvement for later, but currently results in problems with the tests
// std::ostream *ErrorStream = &std::cerr;

void printErrorMessage( const std::string &message, const bool printError )
{
   if( printError ) *ErrorStream << "Error: ";
   *ErrorStream << message << std::endl;
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

std::vector<std::string> splitString( const std::string &string, const char delimiter )
{
   std::vector<std::string> tokens;
   std::istringstream iss( string );
   std::string token;
   while( getline( iss, token, delimiter ) )
      tokens.push_back( token );
   return tokens;
}

bool canBeQuoted( const char *s, const size_t slen )
{
   if( !s ) return false;
   bool saw_single {}, saw_double {};
   for( int i {}; i < (int) slen; i++ )
   {
      char Ch { s[i] };
      if( Ch == '\'' )
      {
         if( saw_double ) return false;
         saw_single = true;
      }
      else if( Ch == '\"' )
      {
         if( saw_single ) return false;
         saw_double = true;
      }
      else if( static_cast<unsigned char>( Ch ) < ' ' )
         return false;
   }
   return true;
}

bool goodUELString( const char *s, const size_t slen )
{
   return slen < GLOBAL_UEL_IDENT_SIZE && canBeQuoted( s, slen );
}

}// namespace library
