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
#include "../global/gmsspecs.h"

// Interface
namespace gdlib::gmsglob
{
enum tssymbol : uint8_t
{
   ssyeq,
   ssygt,
   ssyge,
   ssylt,
   ssyle,
   ssyne,
   ssyplus,
   ssysubtr,
   ssymult,
   ssydiv,
   ssylagpp,
   ssylagmm,
   ssyasspar,
   ssyassequ,
   ssyassdol,
   ssyor,
   ssyxor,
   ssyno,
   ssyyes,
   ssyna,
   ssyinf,
   ssyeps,
   ssysum,
   ssyprod,
   ssysmin,
   ssysmax,
   ssysca,
   ssyacr,
   ssymod,
   ssyset,
   ssypar,
   ssyvar,
   ssyequ,
   ssyfile,
   ssypro,
   ssypre,
   ssymac,
   ssyfunc,
   ssyendloop,
   ssyendif,
   ssyendwhile,
   ssyendfor,
   ssyfre,
   ssybin,
   ssypos,
   ssyneg,
   ssyint,
   ssysos1,
   ssysos2,
   ssysemi,
   ssysemiint,
   ssymin,
   ssymax,
   // !!! Be very careful introducing new symbols before
   // !!! ssyeque. On different spots we assume to see 53
   // !!! elements before that -> better do it further down
   // !!! the list; compare e.g.
   // !!! EQU_USERINFO_BASE in gmsspecs.pas
   ssyeque,
   ssyequg,
   ssyequl,
   ssyequn,
   ssyequx,
   ssyequc,
   ssyequb,
   ssysetm,
   ssysets,
   /* there is an ord in init.inc goe into symtab  */
   ssydisp,
   ssyabort,
   ssyexec,
   ssyload,
   ssyunload,
   ssyloadpoint,
   ssyloadhandle,
   ssyloaddc,
   ssyunloaddi,
   ssyunloadidx,
   ssyexectool,
   ssygdxload,
   ssyBreak,
   ssyContinue,
   ssysand,
   ssysor,
   ssyput,
   ssyptl,
   ssyphd,
   ssypclear,
   ssyppg,
   ssypcl,
   ssyround,
   ssysquare,
   ssycurly,
   ssyimp,
   ssyeqv,
   ssypbruce,
   ssyundf,
   ssyother,
   numtssymbols
};

const std::array ssymboltext = {
        "eq", "gt", "ge", "lt", "le",
        "ne",
        "plus", "subtr",
        "mult", "div",
        "lagpp", "lagmm",
        "asspar", "assequ", "assdol",
        "or", "xor",
        "no", "yes", "na", "inf", "eps",
        "sum", "prod", "smin", "smax",
        "sca", "acr", "mod", "set",
        "par", "var", "equ", "file",
        "pro", "pre", "mac", "func",
        "endloop", "endif", "endwhile", "endfor",
        "fre", "bin", "pos", "neg", "int",
        "sos1", "sos2", "semi", "semiint",
        "min", "max",
        "eque", "equg", "equl", "equn", "equx",
        "equc", "equb", "set", "singleton",
        /* there is an ord in init.inc goe into symtab  */
        "disp", "abort", "exec", "load", "unload",
        "loadpoint", "loadhandle", "loaddc", "unloaddi", "unloadidx",
        "exectool", "gdxload",
        "break", "continue", "sand", "sor",
        "put", "ptl", "phd", "pclear",
        "ppg", "pcl",
        "round", "square", "curly", "imp", "eqv",
        "pbruce", "undf",
        "other" };

// default values for variables and equations
extern std::array<global::gmsspecs::tvarreca, global::gmsspecs::stypsemiint + 1> defrecvar;
extern std::array<global::gmsspecs::tvarreca, ssyequb + 1> defrecequ;

void InitDefaultRecords();

}// namespace gdlib::gmsglob