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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.hpp"

/*
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.hpp"
#include <string>
#include <chrono>
#include "gdx.hpp"

int main(int argc, char** argv)
{
   std::string ErrMsg;
   gdx::TGXFileObj pgx { ErrMsg };
   int ErrNr;
   const auto filename { R"(C:\Users\aschn\Downloads\model\MJjacobian_red2.gdx)" };

   const auto start = std::chrono::high_resolution_clock::now();

   for( int i {}; i < 10; i++ )
   {
      pgx.gdxOpenRead( filename, ErrNr );
      pgx.gdxClose();
   }

   const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::high_resolution_clock::now() - start );
   std::cout << "Execution time: " << duration.count() << " ms" << std::endl;

   return 0;
}*/
