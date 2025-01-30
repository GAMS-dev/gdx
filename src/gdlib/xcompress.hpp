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

#include <string>

// Description:
//  This unit provides the necessary function pointer variables
//  used for dynamic loading of the ZLib1.DLL
//  Note that we rename this DLL to gmszlib1.dll to make sure we
//  know which dll we are using; other components use this dll too!

namespace gdlib::xcompress
{
using pgzFile = void *;
using ulong = unsigned long;

bool LoadZLibLibrary( const std::string &fn, std::string &LoadMsg );
void UnloadZLibLibrary();

bool ZLibDllLoaded();

int compress( void *pdest, ulong &ldest, const void *psrc, ulong lsrc );
int uncompress( void *pdest, ulong &ldest, const void *psrc, ulong lsrc );
pgzFile gzReadOpen( const std::string &fn );
int gzRead( pgzFile pgz, void *Buf, ulong ldest );
int gzReadClose( pgzFile &pgz );
}// namespace gdlib::xcompress