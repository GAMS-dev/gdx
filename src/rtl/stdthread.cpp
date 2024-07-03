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

#include "stdthread.h"

#include <chrono>
#include <system_error>

#if defined( _WIN32 )
#include <windows.h>
#undef max
union cvTime_t
{
   int64_t i64;
   FILETIME ft;
};
#else
#include <sys/time.h>
#endif

using namespace std::literals::string_literals;

namespace rtl::stdthread
{
/*
   * Epochs (zero-time) for some different time types:
   *   Unix      epoch:  1 Jan 1970 00:00:00.00 UTC
   *   TCVTime   epoch:  1 Jan 1601 00:00:00.00 UTC
   *   TDateTime epoch: 30 Dec 1899 00:00:00.00 local time
   *
   * TCVTime epoch + DATE_DELTA_CV_DT = TDateTime epoch
   * so ...  trunc(TDateTime) + DATE_DELTA_CV_DT = (TCVTime div nicksPerDay)
   * TCVTime epoch + DATE_DELTA_CV_UNIX = Unix epoch
*/
constexpr int DATE_DELTA_CV_UNIX = 134774,
              NICKS_PER_USEC = 10,
              NICKS_PER_MSEC = 1000 * NICKS_PER_USEC,
              NICKS_PER_SEC = 1000 * NICKS_PER_MSEC;

/* 64-bit constants do not initialize properly in P3 so
 * initialize them at execution time in initialization section */
static int64_t nicksPerMin { 60 * NICKS_PER_SEC },
        nicksPerHour { nicksPerMin * 60 },
        nicksPerDay { nicksPerHour * 24 };

/*static int64_t nowCV()
{
#if defined( _WIN32 )
   cvTime_t cvt {};
   GetSystemTimeAsFileTime( &cvt.ft );
   return cvt.i64;
#else
   timeval tv {};
   if( gettimeofday( &tv, nullptr ) )
      return 0;
   return tv.tv_sec * NICKS_PER_SEC + tv.tv_usec * NICKS_PER_USEC + nicksPerDay * DATE_DELTA_CV_UNIX;
#endif
}

static void incCVTimeMillis( int64_t &cvt, const uint32_t ticks )
{
   cvt += ticks * NICKS_PER_MSEC;
}

static void decCVTimeMillis( int64_t &cvt, const uint32_t ticks )
{
   cvt -= ticks * NICKS_PER_MSEC;
   if( cvt < 0 ) cvt = 0;
}*/

static std::chrono::time_point<std::chrono::steady_clock> absTimeTP( const int64_t aTimeCV )
{
#if defined( _WIN32 )
   cvTime_t cvt {};
   GetSystemTimeAsFileTime( &cvt.ft );
   int64_t nowCV = cvt.i64;
#else
   timeval tv {};
   if( gettimeofday( &tv, nullptr ) )
      return std::chrono::steady_clock::now() + std::chrono::milliseconds( 400 );
   int64_t nowCV = tv.tv_sec * NICKS_PER_SEC + tv.tv_usec * NICKS_PER_USEC + nicksPerDay * DATE_DELTA_CV_UNIX;
#endif
   int64_t delta { aTimeCV - nowCV };
   if( delta < 0 )
      delta = 0;
   return std::chrono::steady_clock::now() + std::chrono::microseconds( delta / 10 );
}

TStdThread::TStdThread(const std::function<void()> &execute) : std::thread {execute}
{
}

TStdThread::~TStdThread()
{
   try
   {
      if( joinable() )
         detach();
   }
   catch( const std::exception & )
   {
      // silently ignore any exceptions or failures
   }
}


uint64_t TStdThread::getThreadHash() const
{
   constexpr std::hash<std::thread::id> hasher;
   std::size_t r { hasher(get_id() ) };
   // P3 struggles with unsigned int 64.  For example, intToStr treats it as
   // signed 64-bit, and values with the high bit set look negative.
   // So it is convenient to clear the high bit
   if( constexpr std::size_t mx { std::numeric_limits<std::size_t>::max() / 2 + 1 }; r >= mx )
      r -= mx;
   return r;
}

TStdCondVar::TStdCondVar()
{
   try
   {
      cv = std::make_unique<std::condition_variable_any>();
   }
   catch( const std::system_error &e )
   {
      throw EStdCondVar { "Caught system error creating condition variable: "s + e.what() };
   }
}

TStdCondVar::~TStdCondVar() = default;

void TStdCondVar::notifyOne() const
{
   cv->notify_one();
}

void TStdCondVar::notifyAll() const
{
   cv->notify_all();
}

void TStdCondVar::wait( std::mutex &mx ) const
{
   cv->wait( mx );
}

// timedWaitAbs returns false if it returns because abstime was reached, true o/w
bool TStdCondVar::timedWaitAbs( std::mutex &mx, const int64_t absTime ) const
{
   return std::cv_status::timeout != cv->wait_until(mx,absTimeTP(absTime));
}

}// namespace rtl::stdthread
