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

#pragma once

#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <string>

namespace rtl::p3io
{

void dig2Exp( const char *dig, size_t digLen, int decPos, int isNeg, int width, int decimals, char *buf, size_t *bufLen );
void padLeftC2P( const char eBuf[], size_t eLen, int width, char *s, uint8_t sMax );

void P3_Str_dd0( double x, char *s, uint8_t sMax, size_t *eLen );

void P3_Val_dd(const char *s, size_t slen, double *d, int *code);
void P3_Val_dd(const char *s, double *d, int *code);

void P3_Val_i(const char *s, size_t slen, int *i, int *code);
void P3_Val_i(const char *s, int *i, int *code);

enum P3FileType : uint8_t
{
   ft_text_file,
   ft_typed,
   ft_untyped
};

enum P3FileMode : uint8_t
{
   P3_APPEND = 0,
   P3_RESET = 4,
   P3_REWRITE = 8,
   P3_UPDATE = 12
};

enum P3FileState : uint8_t
{
   P3_UNASSIGNED,
   P3_CLOSED,
   P3_OPEN
};

constexpr uint8_t P3_MODEMASK = 12;

struct P3File {
   FILE *f;
   uint8_t status;
   uint32_t block_size;
   std::string nam;
};

extern uint8_t SYSTEM_filemode;

#ifdef __IN_CPPMEX__
void P3FileOpn( P3File *fil, uint8_t status, P3FileType type, uint32_t block_size );
void P3FileOpn(P3File *fil, const char *s);
#endif

}// namespace rtl::p3io