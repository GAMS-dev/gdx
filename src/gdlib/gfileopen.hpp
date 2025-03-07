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
#include "../global/delphitypes.hpp"
#include "../rtl/p3utils.hpp"

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace gdlib::gfileopen
{
int grRewrite( const std::string &fn, bool ReTry, std::fstream &FileHandle, int &IORes );
int grReset( const std::string &fn, bool ReTry, std::fstream &FileHandle, int &IORes );
int grAppend( const std::string &fn, bool ReTry, std::fstream &FileHandle, int &IORes );

int grRewrite( const std::string &fn, bool ReTry, FILE *&FileHandle, int &IORes );
int grReset( const std::string &fn, bool ReTry, FILE *&FileHandle, int &IORes );
int grAppend( const std::string &fn, bool ReTry, FILE *&FileHandle, int &IORes );

int grResetUntyped( const std::string &fn, int RecSize, bool ReTry, std::fstream &FileHandle, int &IORes );
int grRewriteUntyped( const std::string &fn, int RecSize, bool ReTry, std::fstream &FileHandle, int &IORes );

int grResetUntyped( const std::string &fn, int RecSize, bool ReTry, rtl::p3utils::Tp3FileHandle &FileHandle, int &IORes );
int grRewriteUntyped( const std::string &fn, int RecSize, bool ReTry, rtl::p3utils::Tp3FileHandle &FileHandle, int &IORes );

int grResetUntyped( const std::string &fn, int RecSize, bool ReTry, FILE *&FileHandle, int &IORes );
int grRewriteUntyped( const std::string &fn, int RecSize, bool ReTry, FILE *&FileHandle, int &IORes );
}// namespace gdlib::gfileopen
