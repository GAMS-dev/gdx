/*
* GAMS - General Algebraic Modeling System GDX API
*
* Copyright (c) 2017-2026 GAMS Software GmbH <support@gams.com>
* Copyright (c) 2017-2026 GAMS Development Corp. <support@gams.com>
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

#include "p3utils.hpp"
#include "../doctest.hpp"
#include <iostream>
#include <string>

#if __cplusplus >= 202002L
#include <ranges>
#endif

using namespace std::literals::string_literals;

using namespace rtl::p3utils;

namespace tests::rtltests::p3utilstests
{

TEST_SUITE_BEGIN( "rtl::p3utils" );

TEST_CASE( "Test get digits" )
{
   REQUIRE_EQ( "23"s, getDigits( 23 ) );
}

#if __cplusplus >= 202002L
#ifdef __linux__

static std::array<const locpath_t, p3Documents+1> GenerateReference(const std::filesystem::path &home,
                                                              const std::filesystem::path &appName)
{
   namespace fs = std::filesystem;

   std::array<const locpath_t, p3Documents+1> locPathsExpected = {
      locpath_t { home / ".config", fs::path("/etc/xdg/"), fs::path("/etc/") },
      locpath_t { home / ".config" / appName, fs::path("/etc/xdg") / appName, fs::path("/etc/") / appName},
      locpath_t { home / ".local/share" / appName, fs::path("/usr/local/share") / appName, fs::path("/usr/share") / appName},
      locpath_t { home / ".local/share" / appName, fs::path("/usr/local/share") / appName, fs::path("/usr/share") / appName},
      locpath_t { home / ".local/share" / appName, fs::path("/usr/local/share") / appName, fs::path("/usr/share") / appName},
      locpath_t { home / "Documents" / appName },
   };

   return locPathsExpected;
}

static std::array<const locpath_t, p3Documents+1> GenerateXdgReference(
   const std::filesystem::path &home,
   const std::vector<std::vector<std::string_view>> &xdgDirs,
   const std::filesystem::path &xdgDataHome,
   const std::filesystem::path &xdgConfigHome,
   const std::filesystem::path &appName)
{
   namespace fs = std::filesystem;

   locpath_t locConfig { xdgConfigHome };
   // FIXME: locpath_t locAppConfig { xdgConfigHome / appName };
   locpath_t locAppConfig { xdgConfigHome };
   // FIXME: locpath_t locData { xdgDataHome / appName };
   locpath_t locData { xdgDataHome };

   const auto &xdgConfigDirs = xdgDirs.at(0);
   const auto &xdgDataDirs = xdgDirs.at(1);

   for (const auto &dir : xdgConfigDirs) {
      std::string d { dir };
      std::erase(d, ':');
      auto p = fs::path(d);
      locConfig.push_back(p / "");
      locAppConfig.push_back(p / appName);
   }

   for (const auto &dir : xdgDataDirs) {
      std::string d { dir };
      std::erase(d, ':');
      auto p = fs::path(d);
      locData.push_back(p / appName);
   }

   std::array<const locpath_t, p3Documents+1> locPathsExpected = {
      locConfig,
      locAppConfig,
      locData,
      locData,
      locData,
      locpath_t { home / "Documents" / appName },
   };

   return locPathsExpected;
}

static std::string strFromEnv(const char *name)
{
   const char *val = std::getenv(name);
   return val ? val : "";
}

static unsigned checkLocations(std::array<const locpath_t, p3Documents+1> &refPaths)
{
   int rc = 0;
   locpath_t locPaths {};
   for (auto locType : std::views::iota(0, static_cast<int>(p3Documents+1))) {

      locPaths.clear();
      int err;
      // return code indicate failure in p3WritableLocation
      p3StandardLocations(static_cast<Tp3Location>(locType), u8"GAMS", locPaths, err);

      if (locPaths != refPaths[locType]) {
         std::cerr << "ERROR: paths are different for locType " << locType << "\n";

         std::cerr << "Got paths:\n";
         for (const auto &p: locPaths) {
            std::cerr << "\t" << p << "\n";
         }

         std::cerr << "Expected paths:\n";
         for (const auto &p: refPaths[locType]) {
            std::cerr << "\t" << p << "\n";
         }

         err = 1;
      }

      rc += err;
   }

   return rc;
}

// LLM code, to be replace with std::views::join_with in C++-23
// This template accepts ANY range (vector, view, list, etc.)
template <std::ranges::input_range R>
std::string join_range(R&& range, std::string_view delimiter) {
   std::string result;
    auto it = std::ranges::begin(range);
    auto end = std::ranges::end(range);

    // If the range is completely empty, return early
    if (it == end) return result;

    // Append the first element
    result += *it;
    ++it;

    // Append the delimiter and the rest of the elements
    for (; it != end; ++it) {
        result += delimiter;
        result += *it;
    }

    return result;
}

#endif //__linux__

#endif //c++20

TEST_CASE( "Test p3StandardLocations" )
{
   namespace fs = std::filesystem;

   int count;
   TLocNames dirNames;
   p3StandardLocations( p3Data, "APPNAME"s, dirNames, count );
   REQUIRE_FALSE(dirNames.empty());

#if __cplusplus >= 202002L
#ifdef __linux__

   std::string xdgDataDirs    { strFromEnv("XDG_DATA_DIRS")   };
   std::string xdgConfigDirs  { strFromEnv("XDG_CONFIG_DIRS") };
   std::string xdgDataHome    { strFromEnv("XDG_DATA_HOME")   };
   std::string xdgConfigHome  { strFromEnv("XDG_CONFIG_HOME") };
   std::string home           { strFromEnv("HOME") };

   unsetenv("XDG_DATA_DIRS");
   unsetenv("XDG_CONFIG_DIRS");
   unsetenv("XDG_DATA_HOME");
   unsetenv("XDG_CONFIG_HOME");

   unsigned err = 0;

   auto refs = GenerateReference(home, "GAMS");
   err += checkLocations(refs);

   setenv("HOME", "/tmp/test", 1);

   const char xdgDataHomeTest[] = "/xdg/dataHome";
   const char xdgConfigHomeTest[] = "/xdg/configHome";

   setenv("XDG_DATA_HOME", xdgDataHomeTest, 1);
   setenv("XDG_CONFIG_HOME", xdgConfigHomeTest, 1);

   std::vector<std::vector<std::vector<std::string_view>>> xdgDirsTestArray = {
      {
         { "/xdg/ConfigDirs" },
         { "/xdg/dataDirs" },
      },
      {
         { ":/xdg/ConfigDirs"},
         { "/xdg/dataDirs:"},
      },
      {
         { ":/xdg/ConfigDirs:"},
         { ":/xdg/dataDirs:"},
      },
      {
      { "/xdg/ConfigDirs", "/etc/xdg" },
      { "/xdg/dataDirs", "/var/lib/flatpak/exports/share" },
      },
      {
         { ":/xdg/ConfigDirs", "/etc/xdg" },
         { "/xdg/dataDirs", "/var/lib/flatpak/exports/share:" }
      },
      {
         { ":/xdg/dataDirs", "/var/lib/flatpak/exports/share:" },
         { ":/xdg/dataDirs", "/var/lib/flatpak/exports/share:" },
      },
      {
         { "/xdg/ConfigDirs", "/etc/xdg", "/some/config/dir" },
         { "/xdg/dataDirs", "/var/lib/flatpak/exports/share", "/some/data/dir" },
      },
      {
         { ":/xdg/ConfigDirs", "/etc/xdg", "/some/config/dir" },
         { "/xdg/dataDirs", "/var/lib/flatpak/exports/share", "/some/data/dir:" },
      },
      {
         { ":/xdg/ConfigDirs", "/etc/xdg", "/some/config/dir:" },
         { ":/xdg/dataDirs", "/var/lib/flatpak/exports/share", "/some/data/dir:" },
      },
   };

   for (const auto &pair : xdgDirsTestArray ) {
      const auto &xdgConfigDirsV = pair.at(0);
      const auto &xdgDataDirsV = pair.at(1);

      setenv("XDG_CONFIG_DIRS", join_range(xdgConfigDirsV, ":").c_str(), 1);
      setenv("XDG_DATA_DIRS", join_range(xdgDataDirsV, ":").c_str(), 1);

      auto refs = GenerateXdgReference("/tmp/test", pair, xdgDataHomeTest, xdgConfigHomeTest, "GAMS");
      err += checkLocations(refs);

   }

   setenv("XDG_DATA_DIRS", xdgDataDirs.c_str(), 1);
   setenv("XDG_CONFIG_DIRS", xdgConfigDirs.c_str(), 1);
   setenv("XDG_DATA_HOME", xdgDataHome.c_str(), 1);
   setenv("XDG_CONFIG_HOME", xdgConfigHome.c_str(), 1);

   setenv("HOME", home.c_str(), 1);

   auto splitPath = [] (const std::string_view &s) -> std::vector<std::string_view>
      {
         std::vector<std::string_view> v;
         size_t start = 0, pos, sz = s.size();
         do {
            pos = s.find_first_of(':', start);
            if (pos > start) {
               v.push_back(s.substr(start, pos-start));
            }
            start = pos+1;
         } while (pos < sz-1);

         return v;
      };

   // Right now, we only support testing when either all XDG_* env variable are present, or none
   if (!xdgConfigDirs.empty() && !xdgDataDirs.empty() && !xdgDataHome.empty() && !xdgConfigHome.empty()) {

      std::vector<std::string_view> xdgConfigDirsV = splitPath(xdgConfigDirs);
      std::vector<std::string_view> xdgDataDirsV = splitPath(xdgDataDirs);
      std::vector<std::vector<std::string_view>> xdgDirsUser { xdgConfigDirsV, xdgDataDirsV };

      fs::path xdgConfigHomePath = xdgConfigHome.empty() ? fs::path(home) / ".config" : fs::path(xdgConfigHome);
      fs::path xdgDataHomePath = xdgDataHome.empty() ? fs::path(home) / ".local/share" : fs::path(xdgDataHome);
      auto r = GenerateXdgReference(home, xdgDirsUser, xdgDataHomePath, xdgConfigHomePath, "GAMS");
      err += checkLocations(r);

   } else if (xdgConfigDirs.empty() && xdgDataDirs.empty() && xdgDataHome.empty() && xdgConfigHome.empty()) {

      auto r = GenerateReference(home, "GAMS");
      err += checkLocations(r);

   }

   REQUIRE_FALSE(err);

#endif
#endif
}


TEST_SUITE_END();

}
