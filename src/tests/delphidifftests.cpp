/*
* GAMS - General Algebraic Modeling System GDX API
*
* Copyright (c) 2017-2023 GAMS Software GmbH <support@gams.com>
* Copyright (c) 2017-2023 GAMS Development Corp. <support@gams.com>
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

#undef NDEBUG

#include "doctest.h"// for ResultBuilder, Expressi...

#include <string>
#include <array>
#include <iostream>
#include <chrono>
#include <list>
#include <filesystem>

#include <C:/GAMS/43/apifiles/C/api/gdxcc.h>
#include "../gdx.h"

using namespace std::literals::string_literals;

namespace gdx::tests::delphidifftests
{

TEST_SUITE_BEGIN( "Validate against GAMS 43 Delphi GDX" );

const static std::array skipFiles {"bau_p.gdx"s, "fnsqlog10.gdx"s, "testExcel.gdx"s};
const static bool quiet {true};

static bool ends_with(const std::string &s, const std::string &suffix) {
   for(int i{}; i<suffix.length(); i++)
      if(s[s.size()-1-i] != suffix[suffix.length()-1-i]) return false;
   return true;
}

static auto elapsed_time(const std::chrono::time_point<std::chrono::system_clock> &start) {
   return std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now() - start ).count();
}

const static std::array phases {"uels"s, "records"s};

template<int phase>
void showProgress(std::chrono::time_point<std::chrono::system_clock> &last, std::chrono::time_point<std::chrono::system_clock> &start, int ix, int card) {
   if(!quiet && ix % 1000 == 0 && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - last).count() > 4000) {
      last = std::chrono::system_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(last - start).count();
      double eta {(double)card / ((double)ix / (double)elapsed)};
      double etaHours = eta / (1000.0 * 3600.0);
      std::cout << "Progress comparing "s << phases[phase] << ": "s << (double)ix/(double)card * 100.0 << "% ETA="s << etaHours << " hours"s << std::endl;
   }
}

void validateGDXFile(const std::string &fn) {
   if(std::any_of(skipFiles.begin(), skipFiles.end(), [&](const std::string &skipFile) { return ends_with(fn, skipFile); }))
      return;
   if(!quiet)
      std::cout << "Checking GDX file: "s << fn << std::endl;
   std::array<char, 256> msg {};
   ::gdxHandle_t pgx;
   REQUIRE(::gdxCreateD(&pgx, "C:\\GAMS\\43", msg.data(), msg.size()));
   REQUIRE(pgx);
   REQUIRE_EQ('\0', msg.front());
   std::string msg2;
   gdx::TGXFileObj gdx{msg2};
   REQUIRE(msg2.empty());

   int ErrNr;
   REQUIRE(gdx.gdxOpenRead(fn.c_str(), ErrNr));
   REQUIRE_FALSE(gdx.gdxErrorCount());
   REQUIRE_FALSE(::gdxErrorCount(pgx));
   REQUIRE_FALSE(ErrNr);
   REQUIRE(::gdxOpenRead(pgx, fn.c_str(), &ErrNr));
   REQUIRE_FALSE(ErrNr);

   int nsyms1, nsyms2, nuels1, nuels2;
   REQUIRE(gdx.gdxSystemInfo(nsyms1, nuels1));
   REQUIRE(::gdxSystemInfo(pgx, &nsyms2, &nuels2));
   REQUIRE_EQ(nsyms1, nsyms2);
   REQUIRE_EQ(nuels1, nuels2);

   {
      std::chrono::time_point<std::chrono::system_clock> last, start;
      last = start = std::chrono::system_clock::now();
      std::array<char, GMS_SSSIZE> uelLabel1 {}, uelLabel2 {};
      int uelMap1, uelMap2;
      for( int u { 1 }; u <= nuels1; u++ )
      {
         assert( gdx.gdxUMUelGet( u, uelLabel1.data(), uelMap1 ) );
         assert( gdxUMUelGet( pgx, u, uelLabel2.data(), &uelMap2 ) );
         assert( !std::strcmp( uelLabel1.data(), uelLabel2.data() ) );
         assert( uelMap1 == uelMap2 );
         showProgress<0>(last, start, u, nuels1);
      }
   }

   std::array<char, GMS_SSSIZE> symName1 {}, symName2 {}, explText1 {}, explText2 {};
   int syDim1, syTyp1, syDim2, syTyp2, recCnt1, recCnt2, userInfo1, userInfo2, nrecs, dimFrst1, dimFrst2, nvals;
   std::array<int, GMS_MAX_INDEX_DIM> keys1 {}, keys2 {};
   std::array<double, GMS_VAL_MAX> values1 {}, values2 {};

   for(int n{}; n<=nsyms1; n++) {
      REQUIRE(gdx.gdxSymbolInfo(n, symName1.data(), syDim1, syTyp1));
      REQUIRE_FALSE(gdx.gdxErrorCount());
      REQUIRE(gdxSymbolInfo(pgx, n, symName2.data(), &syDim2, &syTyp2));
      REQUIRE_FALSE(::gdxErrorCount(pgx));

      /*if(!quiet)
         std::cout << "Comparing symbol "s << symName1.data() << "..."s << std::endl;*/

      REQUIRE(!std::strcmp(symName1.data(), symName2.data()));
      REQUIRE_EQ(syDim1, syDim2);
      REQUIRE_EQ(syTyp1, syTyp2);
      nvals = syTyp1 < dt_var ? 1 : GMS_VAL_MAX;

      REQUIRE(gdx.gdxSymbolInfoX(n, recCnt1, userInfo1, explText1.data()));
      REQUIRE(::gdxSymbolInfoX(pgx, n, &recCnt2, &userInfo2, explText2.data()));
      REQUIRE_EQ(recCnt1, recCnt2);
      REQUIRE_EQ(userInfo1, userInfo2);
      REQUIRE(!std::strcmp(explText1.data(), explText2.data()));
      std::chrono::time_point<std::chrono::system_clock> last, start;
      last = start = std::chrono::system_clock::now();

      if(!recCnt1) continue;

      REQUIRE(gdx.gdxDataReadRawStart(n, nrecs));
      REQUIRE_EQ(recCnt1, nrecs);
      REQUIRE(::gdxDataReadRawStart(pgx, n, &nrecs));
      REQUIRE_EQ(recCnt1, nrecs);

      for(int r{}; r<recCnt1; r++) {
         assert(gdx.gdxDataReadRaw(keys1.data(), values1.data(), dimFrst1));
         assert(::gdxDataReadRaw(pgx, keys2.data(), values2.data(), &dimFrst2));
         for(int d{}; d<syDim1; d++)
            assert(keys1[d] == keys2[d]);
         for(int ix{}; ix<nvals; ix++)
            assert(values1[ix] == values2[ix]);
         showProgress<1>(last, start, r, recCnt1);
      }

      REQUIRE(gdx.gdxDataReadDone());
      REQUIRE(::gdxDataReadDone(pgx));
   }

   ::gdxClose(pgx);
}

void validateRecursively(const std::string &path) {
   if(!quiet)
      std::cout << "Descending into directory path: "s << path << std::endl;
   for(const auto &entry : std::filesystem::directory_iterator(path)) {
      std::string fn = entry.path().string();
      if(entry.is_directory())
         validateRecursively( fn );
      else if(ends_with(fn, ".gdx"))
         validateGDXFile(fn);
   }
}

// Writing lots of uels and symbols builds up tables which is lots of effort
// just dumping raw records instead is very efficient
void createBigGDXFile() {
   std::string errMsg;
   gdx::TGXFileObj gdx{errMsg};
   int errNr;
   gdx.gdxOpenWrite("verybig.gdx", "delphidifftest", errNr);

   gdx.gdxUELRegisterRawStart();
   gdx.gdxUELRegisterRaw("i1");
   gdx.gdxUELRegisterDone();

   gdx.gdxDataWriteRawStart("i", "a var", 20, dt_var, 0);
   std::array<int, GMS_MAX_INDEX_DIM> keys {};
   std::array<double, GMS_VAL_MAX> values {};
   for(uint64_t i{}; i<1024*1024*420*2; i++)
   {
      keys.front() = (int)(i+1);
      gdx.gdxDataWriteRaw( keys.data(), values.data() );
   }
   gdx.gdxDataWriteDone();

   gdx.gdxClose();
}

void readFileCpp(const std::string &fn) {
   if(std::any_of(skipFiles.begin(), skipFiles.end(), [&](const std::string &skipFile) { return ends_with(fn, skipFile); }))
      return;
   if(!quiet)
      std::cout << "Checking GDX file: "s << fn << std::endl;
   std::string msg2;
   gdx::TGXFileObj gdx{msg2};
   REQUIRE(msg2.empty());
   int ErrNr;
   REQUIRE(gdx.gdxOpenRead(fn.c_str(), ErrNr));
   REQUIRE_FALSE(gdx.gdxErrorCount());
   REQUIRE_FALSE(ErrNr);
   int nsyms1, nuels1;
   REQUIRE(gdx.gdxSystemInfo(nsyms1, nuels1));
   {
      std::chrono::time_point<std::chrono::system_clock> last, start;
      last = start = std::chrono::system_clock::now();
      std::array<char, GMS_SSSIZE> uelLabel1 {};
      int uelMap1;
      for( int u { 1 }; u <= nuels1; u++ )
      {
         assert( gdx.gdxUMUelGet( u, uelLabel1.data(), uelMap1 ) );
         showProgress<0>(last, start, u, nuels1);
      }
   }
   std::array<char, GMS_SSSIZE> symName1 {}, explText1 {};
   int syDim1, syTyp1, recCnt1, userInfo1, nrecs, dimFrst1, nvals;
   std::array<int, GMS_MAX_INDEX_DIM> keys1 {};
   std::array<double, GMS_VAL_MAX> values1 {};
   for(int n{}; n<=nsyms1; n++) {
      REQUIRE(gdx.gdxSymbolInfo(n, symName1.data(), syDim1, syTyp1));
      REQUIRE_FALSE(gdx.gdxErrorCount());
      nvals = syTyp1 < dt_var ? 1 : GMS_VAL_MAX;
      REQUIRE(gdx.gdxSymbolInfoX(n, recCnt1, userInfo1, explText1.data()));
      std::chrono::time_point<std::chrono::system_clock> last, start;
      last = start = std::chrono::system_clock::now();
      if(!recCnt1) continue;
      REQUIRE(gdx.gdxDataReadRawStart(n, nrecs));
      REQUIRE_EQ(recCnt1, nrecs);
      for(int r{}; r<recCnt1; r++) {
         assert(gdx.gdxDataReadRaw(keys1.data(), values1.data(), dimFrst1));
         showProgress<1>(last, start, r, recCnt1);
      }
      REQUIRE(gdx.gdxDataReadDone());
   }
}

void readRecursivelyCpp(const std::string &path) {
   if(!quiet)
      std::cout << "Descending into directory path: "s << path << std::endl;
   for(const auto &entry : std::filesystem::directory_iterator(path)) {
      std::string fn = entry.path().string();
      if(entry.is_directory())
         readRecursivelyCpp( fn );
      else if(ends_with(fn, ".gdx"))
         readFileCpp(fn);
   }
}

void readFileDelphi(const std::string &fn) {
   if(std::any_of(skipFiles.begin(), skipFiles.end(), [&](const std::string &skipFile) { return ends_with(fn, skipFile); }))
      return;
   if(!quiet)
      std::cout << "Checking GDX file: "s << fn << std::endl;
   std::array<char, 256> msg {};
   ::gdxHandle_t pgx;
   REQUIRE(::gdxCreateD(&pgx, "C:\\GAMS\\43", msg.data(), msg.size()));
   REQUIRE(pgx);
   REQUIRE_EQ('\0', msg.front());
   REQUIRE_FALSE(::gdxErrorCount(pgx));
   int ErrNr;
   REQUIRE(::gdxOpenRead(pgx, fn.c_str(), &ErrNr));
   REQUIRE_FALSE(ErrNr);
   int nsyms2, nuels2;
   REQUIRE(::gdxSystemInfo(pgx, &nsyms2, &nuels2));
   {
      std::chrono::time_point<std::chrono::system_clock> last, start;
      last = start = std::chrono::system_clock::now();
      std::array<char, GMS_SSSIZE> uelLabel2 {};
      int uelMap2;
      for( int u { 1 }; u <= nuels2; u++ )
      {
         assert( gdxUMUelGet( pgx, u, uelLabel2.data(), &uelMap2 ) );
         showProgress<0>(last, start, u, nuels2);
      }
   }
   std::array<char, GMS_SSSIZE>  symName2 {}, explText2 {};
   int syDim2, syTyp2, recCnt2, userInfo2, nrecs, dimFrst2, nvals;
   std::array<int, GMS_MAX_INDEX_DIM> keys2 {};
   std::array<double, GMS_VAL_MAX> values2 {};
   for(int n{}; n<=nsyms2; n++) {
      REQUIRE(gdxSymbolInfo(pgx, n, symName2.data(), &syDim2, &syTyp2));
      REQUIRE_FALSE(::gdxErrorCount(pgx));
      nvals = syTyp2 < dt_var ? 1 : GMS_VAL_MAX;
      REQUIRE(::gdxSymbolInfoX(pgx, n, &recCnt2, &userInfo2, explText2.data()));
      std::chrono::time_point<std::chrono::system_clock> last, start;
      last = start = std::chrono::system_clock::now();
      if(!recCnt2) continue;
      REQUIRE(::gdxDataReadRawStart(pgx, n, &nrecs));
      REQUIRE_EQ(recCnt2, nrecs);
      for(int r{}; r<recCnt2; r++) {
         assert(::gdxDataReadRaw(pgx, keys2.data(), values2.data(), &dimFrst2));
         showProgress<1>(last, start, r, recCnt2);
      }
      REQUIRE(::gdxDataReadDone(pgx));
   }
   ::gdxClose(pgx);
}

void readRecursivelyDelphi(const std::string &path) {
   if(!quiet)
      std::cout << "Descending into directory path: "s << path << std::endl;
   for(const auto &entry : std::filesystem::directory_iterator(path)) {
      std::string fn = entry.path().string();
      if(entry.is_directory())
         readRecursivelyDelphi( fn );
      else if(ends_with(fn, ".gdx"))
         readFileDelphi(fn);
   }
}

void readRecursivelyDelphiAndCpp(const std::string &path) {
   if(!quiet)
      std::cout << "Descending into directory path: "s << path << std::endl;
   for(const auto &entry : std::filesystem::directory_iterator(path)) {
      std::string fn = entry.path().string();
      if(entry.is_directory())
         readRecursivelyDelphiAndCpp( fn );
      else if(ends_with(fn, ".gdx"))
      {
         auto start { std::chrono::system_clock::now() };
         readFileCpp( fn );
         auto elapsedCpp {elapsed_time(start)};
         start = std::chrono::system_clock::now();
         readFileDelphi( fn );
         auto elapsedDelphi {elapsed_time(start)};
         if(elapsedCpp > 500 && (double)elapsedCpp > (double)elapsedDelphi * 1.05)
            std::cout << fn << ';' << (double)elapsedCpp/(double)elapsedDelphi*100.0 << "%;"s << elapsedCpp << ";"s << elapsedDelphi << std::endl;
      }
   }
}

void totalRuntimeDelphiVsCpp() {
   {
      auto start { std::chrono::system_clock::now() };
      //readRecursivelyDelphi( R"(G:\Shared drives\GAMS Performance Suite\gdxfiles)" );
      readRecursivelyDelphi( R"(C:\dockerhome)" );
      std::cout << "Elapsed time Delphi: "s << elapsed_time(start) << std::endl;
   }

   {
      auto start { std::chrono::system_clock::now() };
      //readRecursivelyCpp( R"(G:\Shared drives\GAMS Performance Suite\gdxfiles)" );
      readRecursivelyCpp( R"(C:\dockerhome)" );
      std::cout << "Elapsed time C++: "s << elapsed_time(start) << std::endl;
   }
}

TEST_CASE( "Read all contents of a GDX file with both GAMS 43 P3/Delphi-GDX and C++-GDX" )
{
   //const std::string fn {R"(C:\dockerhome\pAmatrix_Figaro_reg.gdx)"};
   //validateRecursively(R"(C:\dockerhome)");

   //validateGDXFile("C:\\dockerhome\\mrb\\tr20.gdx");

   //validateRecursively(R"(G:\Shared drives\GAMS Performance Suite\gdxfiles)");

   readRecursivelyDelphiAndCpp(R"(G:\Shared drives\GAMS Performance Suite\gdxfiles)");

   //totalRuntimeDelphiVsCpp();

   //createBigGDXFile();
   //validateGDXFile("verybig.gdx");
}

TEST_SUITE_END();

}