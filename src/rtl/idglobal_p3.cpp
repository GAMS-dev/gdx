/*
* GAMS - General Algebraic Modeling System GDX API
 *
 * Copyright (c) 2017-2024 GAMS Software GmbH <support@gams.com>
 * Copyright (c) 2017-2024 GAMS Development Corp. <support@gams.com>
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

#if defined(_WIN32)
#include <Windows.h>
#undef max
#else
#include <sys/time.h>
#endif

#include "idglobal_p3.h"
#include <limits>

// ==============================================================================================================
// Implementation
// ==============================================================================================================
namespace rtl::idglobal_p3
{
uint32_t GetTickDiff( const uint32_t AOldTickCount, const uint32_t ANewTickCount )
{
   return ANewTickCount >= AOldTickCount ? ANewTickCount - AOldTickCount : std::numeric_limits<unsigned int>::max() - AOldTickCount + ANewTickCount;
}

uint32_t GetTickCount() {
#if defined( _WIN32 )
   return ::GetTickCount();
#else
  timeval tv {};
  gettimeofday (&tv, nullptr);
  // force tv_sec to take the type of the result
  uint64_t result = tv.tv_sec;
  return result * 1000 + tv.tv_usec / 1000;
#endif
}

}// namespace rtl::idglobal_p3
