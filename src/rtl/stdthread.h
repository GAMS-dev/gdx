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

/* stdthread: Threading unit for use in p3/libs.
* This unit implements several classes for multi-threading:
*   - TStdThread: parallels C++ std::thread
*   - EStdThread: exception thrown for TStdThread errors
*   - TStdMutex: simple mutex/lock (lock/trylock/unlock)
*   - EStdMutex: exception thrown for TStdMutex errors
*   - TStdCondVar: condition variable for use with TStdMutex
*   - EStdCondVar: exception thrown for TStdCondVar errors
* With Delphi, this should compile but produce mostly exceptions
* or wrong values during execution
*/

#pragma once

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace rtl::stdthread
{

using EStdThread = std::runtime_error;
using EStdMutex = std::runtime_error;
using EStdCondVar = std::runtime_error;

// implements a thread class - a parallel to std::thread
class TStdThread : public std::thread
{
public:
   explicit TStdThread(const std::function<void()> &execute);
   virtual ~TStdThread();
   [[nodiscard]] uint64_t getThreadHash() const;
};

class TStdCondVar
{
   std::unique_ptr<std::condition_variable_any> cv {};
public:
   TStdCondVar();
   ~TStdCondVar();
   void notifyOne() const;
   void notifyAll() const;
   void wait(std::mutex &mx ) const;
   bool timedWaitAbs(std::mutex &mx, int64_t absTime ) const;
};

}
