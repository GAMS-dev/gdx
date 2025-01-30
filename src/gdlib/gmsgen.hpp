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



#pragma once
#include "../global/modhead.hpp"
#include "../global/gmsspecs.hpp"

namespace rtl::p3io {
struct P3File;
}

namespace gdlib::gmsgen
{
using TAnsiCharArray = char[global::gmsspecs::BigIndex+1]; //std::array<char, global::gmsspecs::BigIndex>;
using PAnsiCharArray = TAnsiCharArray *;
using DoubleArray = double[global::gmsspecs::BigIndex+1]; //std::array<double, global::gmsspecs::BigIndex>;
using PDoubleArray = DoubleArray *;
using PTextFile = rtl::p3io::P3File *;

using LongIntArray = int[global::gmsspecs::BigIndex+1]; //std::array<int, global::gmsspecs::BigIndex>;
using PLongIntArray = LongIntArray *;

using TBooleanArray = bool[global::gmsspecs::BigIndex+1]; // std::array<bool, global::gmsspecs::BigIndex>;
using PBooleanArray = TBooleanArray *;

using TByteDataArray = uint8_t[global::gmsspecs::BigIndex]; // std::array<uint8_t, global::gmsspecs::BigIndex>;
using PByteDataArray = TByteDataArray *;

enum tfileaction : uint8_t
{
   forRead,
   forWrite,
   forAppend
};

}// namespace gdlib::gmsgen