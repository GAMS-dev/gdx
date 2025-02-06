/*
* GAMS - General Algebraic Modeling System GDX API
 *
 * Copyright (c) 2017-2025 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2025 GAMS Development Corp. <support@gams.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include <algorithm>

#include "gamsdirs.hpp"
#include "../rtl/p3platform.hpp"
#include "../rtl/sysutils_p3.hpp"

using namespace rtl::p3utils;
using namespace rtl::p3platform;
using namespace rtl::sysutils_p3;

using namespace std::literals::string_literals;

bool gdlib::gamsdirs::GMSDataLocations( TGMSLocNames &GMSLocNames, const std::string &sysDir )
{
   bool res { true };

   if( OSFileType() == OSFileWIN )
   {
      std::string pathName;
      if( p3WritableLocation( rtl::p3utils::p3Documents, "GAMS"s, pathName ) )
         GMSLocNames.push_back( pathName + PathDelim + "GAMS"s );
      else
         res = false;
   }

   std::string pathName = ExcludeTrailingPathDelimiter( sysDir );

   TLocNames locNames;
   int eCount;
   bool rc = p3StandardLocations( p3AppData, "GAMS"s, locNames, eCount ) && !eCount;

   GMSLocNames.reserve( GMSLocNames.size() + locNames.size() + 1 );
   std::copy_if( locNames.begin(), locNames.end(), std::back_inserter( GMSLocNames ), [&]( const auto &locName ) { return pathName != locName; } );
   GMSLocNames.push_back( pathName );

   return res && rc;
}

bool gdlib::gamsdirs::GMSConfigLocations( TGMSLocNames &GMSLocNames, const std::string &sysDir )
{
   TLocNames locNames;
   int eCount;
   bool res { p3StandardLocations( p3AppConfig, "GAMS", locNames, eCount ) && !eCount };

   std::string pathName { ExcludeTrailingPathDelimiter( sysDir ) };
   GMSLocNames = { pathName };

   for( auto it = locNames.rbegin(); it != locNames.rend(); ++it )
      if( pathName != *it )
         GMSLocNames.push_back( *it );

   if( OSFileType() == OSFileWIN )
   {
      if( p3WritableLocation( p3Documents, "GAMS", pathName ) )
      {
         GMSLocNames.push_back( pathName + PathDelim + "GAMS" );
      }
      else
         return false;
   }

   return res;
}

bool gdlib::gamsdirs::GMSWritableLocation( rtl::p3utils::Tp3Location locType, std::string &locName )
{
   if( OSFileType() == OSFileWIN )
   {
      bool res { p3WritableLocation( p3Documents, "GAMS", locName ) };
      if( res ) locName += ""s + PathDelim + "GAMS"s;
      return res;
   }
   else
   {
      return p3WritableLocation( locType, "GAMS", locName );
   }
}

bool gdlib::gamsdirs::findDataDirFile( const std::string &fName, std::string &fPathName, const std::string &sysDir, bool &allSearched )
{
   allSearched = false;
   fPathName.clear();
   if( fName.empty() ) return false;
   TGMSLocNames GMSLocNames;
   allSearched = GMSDataLocations( GMSLocNames, sysDir );
   for( const auto &GMSLocName: GMSLocNames )
   {
      const auto fnCandidate = GMSLocName + PathDelim + fName;
      if( FileExists( fnCandidate ) )
      {
         fPathName = fnCandidate;
         return true;
      }
   }
   return false;
}