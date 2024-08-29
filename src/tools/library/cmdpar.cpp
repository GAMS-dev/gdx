#include <cassert>
#include <set>
#include <fstream>
#include <iostream>

#include "cmdpar.h"
#include "common.h"
#include "../../gdlib/utils.h"
#include "../../gdlib/strutilx.h"

namespace library::cmdpar
{

// Brief:
//   Create instance of command line object
TCmdParams::TCmdParams() = default;

// Brief:
//   Release allocated memory and destroy the object
TCmdParams::~TCmdParams()
{
   ClearParams();
}

void TCmdParams::ClearParams()
{
   FKeyList.clear();
   FParList.clear();
}

void TCmdParams::AddVS( int v, const std::string &s )
{
   for( const auto &pair: FKeyList )
      if( pair.first == s )
         assert( false && "Duplicate keyword!" );
   FKeyList.emplace_back( s, v );
}

// Search from the end so the LAST value will be used
int TCmdParams::FindKeyV( int V )
{
   for( int N { GetParamCount() - 1 }; N >= 0; N-- )
      if( GetParams( N ).Key == V )
         return N;
   return -1;
}

// Brief:
//   Crack the command line
// Returns:
//   True if there were no synyax errors; false otherwise
// Description:
//   After specifying the strings and integer values for
//   keywords and parameters (see AddKeyW and AddParam)
//   this function will build the list of keywords and parameters.
//   A string that starts with '@' is interpreted as the name of
//   a file with additional parameters.
bool TCmdParams::CrackCommandLine( const int ParamCount, const char *ParamStr[] )
{
   return AddParameters( 0, {}, ParamCount, ParamStr );
}

// Brief:
//   Insert additional parameters
// Parameters:
//   AInsp: Insertion point for additional parameters
//   CmdLine: Additional parameters
// Description:
//   This function is used to add more parameters to the parameter list
//   that could not be processed while reading the command line for the
//   first time.
bool TCmdParams::AddParameters( const int AInsP, const std::string &CmdLine, const int ParamCount, const char *ParamStr[] )
{
   std::string FParams;
   int xr, maxr;

   auto SkipBl = [&]() {
      while( xr <= maxr && FParams.at( xr ) <= ' ' )
         xr++;
   };

   auto NextToken = [&]() -> std::string {
      std::string result;
      SkipBl();
      if( xr <= maxr )
      {
         std::set<char> StopSet;
         if( FParams.at( xr ) != '"' )
            StopSet = { ' ', '\t' };
         else
         {
            StopSet = { '"' };
            xr++;
         }
         StopSet.insert( '=' );
         int xk {};
         for( int k { 1 }; k <= maxr - xr + 1; k++ )
            if( utils::in( FParams.at( xr + k - 1 ), StopSet ) )
            {
               xk = k;
               break;
            }
         if( xk == 0 )
         {
            result.append( FParams.substr( xr, FParams.length() - xr ) );
            xr = maxr + 1;
         }
         else
         {
            result.append( FParams.substr( xr, xk - 1 ) );
            xr += xk;
         }
      }
      // std::cout << "token = |" << result << '|' << std::endl;
      return result;
   };

   auto NextKey = [&]( std::string &s ) -> int {
      s.clear();
      std::string T { NextToken() };
      if( T.empty() )
         return static_cast<int>( CmdParamStatus::ke_empty );
      else
      {
         int result { -1 };
         for( const auto &pair: FKeyList )
            if( gdlib::strutilx::LowerCase( pair.first ) == gdlib::strutilx::LowerCase( T ) )
            {
               result = pair.second;
               break;
            }
         if( result < 0 )
         {
            if( T.front() == '@' )
            {
               s = T.substr( 1 );
               return static_cast<int>( CmdParamStatus::ke_badfile );
            }
            else
            {
               s = T;
               return static_cast<int>( CmdParamStatus::ke_unknown );
            }
         }
         else if( result >= static_cast<int>( CmdParamStatus::kk_big ) )
         {
            result -= static_cast<int>( CmdParamStatus::kk_big );
            SkipBl();
            if( xr <= maxr && FParams.at( xr ) == '=' )
               xr++;
            s = NextToken();
            if( s.empty() )
               return static_cast<int>( CmdParamStatus::ke_noparam );
            else
            {
               int k { static_cast<int>( s.length() ) };
               if( k >= 2 && s.front() == '\'' && s.back() == '\'' )
                  s = s.substr( 1, k - 2 );
            }
         }
         return result;
      }
   };

   auto ExpandCommandLine = [&]() -> std::string {
      std::string result;
      for( int N { 1 }; N < ParamCount; N++ )
      {
         std::string S { ParamStr[N] };
         if( S.empty() )
            continue;
         if( S.find( ' ' ) == std::string::npos )
            result.append( ' ' + S );
         else
         {
            size_t k { S.find( '=' ) };
            if( k == std::string::npos )
            {
               if( S.front() != '@' )
                  result.append( " \"" + S + '"' );
               else
                  // Keep double-quote after @ sign
                  result.append( " @\"" + S.substr( 1 ) + '"' );
            }
            else
            {
               result.append( ' ' + S.substr( 0, k + 1 ) );
               result.append( " \"" + S.substr( k + 1 ) + '"' );
            }
         }
      }
      return result;
   };

   auto ExpandFiles = []( const std::string &Src, std::string &Dest ) -> bool {
      constexpr int MAXBUF { 4 };
      std::array<std::string, MAXBUF> sBuf;
      bool result { true };
      Dest.clear();
      int sp {};
      do {
         int fnd { -1 };
         for( int k { sp }; k < static_cast<int>( Src.length() ); k++ )
            if( Src.at( k ) == '@' )
            {
               fnd = k;
               break;
            }
         if( fnd == -1 )
         {
            Dest.append( Src.substr( sp, Src.length() - sp + 2 ) );
            break;
         }
         Dest.append( Src.substr( sp, fnd - sp ) );
         sp = fnd + 1;
         if( sp >= static_cast<int>( Src.length() ) )
         {
            result = false;
            break;
         }
         char Brk;
         if( Src.at( sp ) != '"' )
            Brk = ' ';
         else
         {
            sp++;
            Brk = '"';
         }

         std::string fname;
         {
            int k { sp };
            while( k < static_cast<int>( Src.length() ) && Src.at( k ) != Brk )
               k++;
            if( k > static_cast<int>( Src.length() ) )
            {
               result = false;
               break;
            }
            fname = Src.substr( sp, k - sp );
            sp = k;
         }

         if( sp < static_cast<int>( Src.length() ) && Src.at( sp ) == '"' )
            sp++;
         if( gdlib::strutilx::ExtractFileExtEx( fname ).empty() )
            fname = gdlib::strutilx::ChangeFileExtEx( fname, ".txt" );
         std::ifstream fi( fname );
         if( !fi.is_open() )
         {
            std::cerr << "**** could not open file: " << fname << std::endl;
            result = false;
         }
         else
         {
            std::string line;
            while( getline( fi, line ) )
            {
               std::vector<std::string> strings = library::splitString( line, ' ' );
               for( int i {}; i < sBuf.size() && i < strings.size(); i++ )
                  sBuf.at( i ) = strings.at( i );
               if( sBuf.front().empty() || sBuf.front().front() == '*' )
                  continue;
               Dest.append( " " );
               for( int k {}; k < sBuf.size(); k++ )
               {
                  if( sBuf.at( k ).empty() )
                     break;
                  Dest.append( sBuf.at( k ) );
               }
               if( sBuf.back().length() == 255 )
               {
                  std::cerr << "**** Input line longer than " << MAXBUF * 255 << " characters" << std::endl;
                  result = false;
               }
            }
            fi.close();
         }
      } while( true );
      // Dest.Write;
      return result;
   };

   /* AddParameters */

   int Insp;
   bool result;
   if( !CmdLine.empty() )
   {
      Insp = AInsP;
      result = ExpandFiles( CmdLine, FParams );
   }
   else
   {
      Insp = 0;
      const std::string wrk { ExpandCommandLine() };
      result = ExpandFiles( wrk, FParams );
   }

   xr = 0;
   maxr = static_cast<int>( FParams.length() ) - 1;
   do {
      std::string ks;
      const int kw = NextKey( ks );
      if( kw == static_cast<int>( CmdParamStatus::ke_empty ) )
         break;
      FParList.insert( { Insp, TParamRec { kw, ks } } );
      Insp++;
   } while( true );

   if( !HasKey( static_cast<int>( CmdParamStatus::kp_input ) ) && GetParams( 0 ).Key == static_cast<int>( CmdParamStatus::ke_unknown ) )
      FParList.at( 0 ).Key = static_cast<int>( CmdParamStatus::kp_input );

   return result;
}

void TCmdParams::AddKeyWord( int v, const std::string &s )
{
   library::assertWithMessage( v >= 0 && v < static_cast<int>( CmdParamStatus::kk_big ),
                               "Bad value for AddKeyw = " + std::to_string( v ) );
   AddVS( v, utils::trim( s ) );
}

void TCmdParams::AddParam( int v, const std::string &s )
{
   library::assertWithMessage( v >= 0 && v < static_cast<int>( CmdParamStatus::kk_big ),
                               "Bad value for AddKeyw = " + std::to_string( v ) );
   AddVS( v + static_cast<int>( CmdParamStatus::kk_big ), utils::trim( s ) );
}

bool TCmdParams::HasParam( int v, library::short_string &s )
{
   int N = FindKeyV( v );
   bool result = N >= 0;
   if( !result )
      s.clear();
   else
      s = GetParams( N ).KeyS;
   return result;
}

// Brief:
//   Check if a keyword was specified
// Parameters:
//   V: Key number
// Returns:
//   True if keyword was specified; false otherwise
bool TCmdParams::HasKey( int v )
{
   return FindKeyV( v ) >= 0;
}

TParamRec TCmdParams::GetParams( int n )
{
   if( n >= 0 && n < static_cast<int>( FParList.size() ) )
      return FParList.at( n );
   else
      return TParamRec { static_cast<int>( CmdParamStatus::ke_empty ), {} };
}

int TCmdParams::GetParamCount() const
{
   return static_cast<int>( FParList.size() );
}

std::string TCmdParams::GetParamText( int key ) const
{
   for( const auto &pair: FKeyList )
      if( pair.second == key )
         return pair.first;
   return "?" + std::to_string( key ) + "?";
}

}// namespace library::cmdpar
