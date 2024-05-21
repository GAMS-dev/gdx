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

#include <string>
#include <array>

#include "delphitypes.h"
#include "../gdlib/utils.h"

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace global::gmsspecs
{
constexpr int MaxNameLen = 63;// starting version 149
constexpr int MaxDim = 20;

constexpr int MaxNameLenV148 = 31;// values before they changed in Ver149
constexpr int MaxDimV148 = 10;

constexpr int BigIndex = 10000000;
constexpr int MAXALGO = 150;// maximum number of solver that can be stored
constexpr int MAXEVALTHREADS = 64;

using tdimension = delphitypes::Bounded<int, 0, MaxDim>;
using TgdxDimension = delphitypes::Bounded<int, 1, MaxDim>;

using TIndex = std::array<int, MaxDim>;
using TStrIndex = std::array<std::string, MaxDim>;
using PTIndex = TIndex *;

// old values up to 133
const double old_valund = 1.0e30; // undefined
const double old_valna = 2.0e30;  // not available/applicable
const double old_valpin = 3.0e30; // plus infinity
const double old_valmin = 4.0e30; // minus infinity
const double old_valeps = 5.0e30; // epsilon
const double old_valacr = 10.0e30;// potential/real acronym

// new values starting with 134
const double valund = 1.0e300; // undefined
const double valna = 2.0e300;  // not available/applicable
const double valpin = 3.0e300; // plus infinity
const double valmin = 4.0e300; // minus infinity
const double valeps = 5.0e300; // epsilon
const double valacr = 10.0e300;// potential/real acronym

const int valnaint = 2100000000;// not available/applicable

const double valiup = 3.0e300;   // upper bound for integers
const double valbig = 1.0e299;   // big positive number
const double valsmall = -1.0e299;// big negative number
const double valtiny = 1.0e-250; // everything smaller could be zero

const double defiterlim = 2.0e9;//default iterlim

// Brief:
//   Enumerated type for special values
// Description:
//   This enumerated type can be used in a Delphi program directly;
//   Programs using the DLL should use the integer values instead
//   This avoids passing enums
enum TgdxSpecialValue : uint8_t
{
   sv_valund,// 0: Undefined
   sv_valna, // 1: Not Available
   sv_valpin,// 2: Positive Infinity
   sv_valmin,// 3: Negative Infinity
   sv_valeps,// 4: Epsilon
   sv_normal,// 5: Normal Value
   sv_acronym// 6: Acronym base value
};

// Brief:
//  Enumerated type for GAMS data types
// Description:
//  This enumerated type can be used in a Delphi program directly;
//  Programs using the DLL should use the integer values instead
//  This avoids passing enums
enum TgdxDataType : uint8_t
{
   dt_set, // 0: Set
   dt_par, // 1: Parameter
   dt_var, // 2: Variable
   dt_equ, // 3: Equation
   dt_alias// 4: Aliased set
};

const int gms_sv_valund = 0,// TgdxSpecialValue
        gms_sv_valna = 1,
          gms_sv_valpin = 2,
          gms_sv_valmin = 3,
          gms_sv_valeps = 4,
          gms_sv_normal = 5,
          gms_sv_acronym = 6;

const int gms_dt_set = 0,// TgdxDataType
        gms_dt_par = 1,
          gms_dt_var = 2,
          gms_dt_equ = 3,
          gms_dt_alias = 4;

enum tgmsvalue : uint8_t
{
   xvreal,
   xvund,
   xvna,
   xvpin,
   xvmin,
   xveps,
   xvacr
};
enum txgmsvalue : uint8_t
{
   vneg,
   vzero,
   vpos,
   vund,
   vna,
   vpin,
   vmin,
   veps,
   vacr
};

tgmsvalue mapval( double x );
txgmsvalue xmapval( double x );

// check loadAll in utilcomp.pas when one of these gets extended
enum tvarstyp : uint8_t
{
   stypunknwn,
   stypbin,
   stypint,
   styppos,
   stypneg,
   stypfre,
   stypsos1,
   stypsos2,
   stypsemi,
   stypsemiint
};
enum tsetstyp : uint8_t
{
   stypsetm,
   stypsets
};

const utils::bsSet<tvarstyp, stypsemiint+1>  varstypX { styppos, stypneg, stypfre },
                                             varstypI { stypbin, stypint, stypsos1, stypsos2, stypsemi, stypsemiint };

const std::array varstyptxt {
        "unknown ", "binary  ", "integer ", "positive", "negative", "free    ", "sos1    ", "sos2    ", "semicont", "semiint " };

enum tequstyp : uint8_t
{
   styeque,
   styequg,
   styequl,
   styequn,
   styequx,
   styequc,
   styequb
};

// initialized in gmsglob.cppm using tssymbol
extern std::array<int, styequb + 1> equstypInfo;

//                   0        1            2        3           4
enum tvarvaltype : uint8_t
{
   vallevel,
   valmarginal,
   vallower,
   valupper,
   valscale
};

using tvarreca = std::array<double, valscale+1>;

const std::array<std::string, valscale+1> sufftxt { "L", "M", "LO", "UP", "SCALE" };

const int nlconst_one = 1,
          nlconst_ten = 2,
          nlconst_tenth = 3,
          nlconst_quarter = 4,
          nlconst_half = 5,
          nlconst_two = 6,
          nlconst_four = 7,
          nlconst_zero = 8,
          nlconst_oosqrt2pi = 9,// 1/sqrt(2*pi)
          nlconst_ooln10 = 10,    // 1/ln(10)
          nlconst_ooln2 = 11,     // 1/ln(2)
          nlconst_pi = 12,        // pi
          nlconst_pihalf = 13,    // pi/2
          nlconst_Sqrt2 = 14,     // sqrt(2)
          nlconst_three = 15,
          nlconst_five = 16;

const int EQU_USERINFO_BASE = 53;

// check loadAll in utilcomp.pas when this gets extended
const int   equ_E = 0,
            equ_G = 1,
            equ_L = 2,
            equ_N = 3,
            equ_X = 4,
            equ_C = 5,
            equ_B = 6;

const std::array<std::string, 7> equstyp {
        " =E= ",
        " =G= ",
        " =L= ",
        " =N= ",
        " =X= ",
        " =C= ",
        " =B= "
};

const std::array<char, 7> equctyp {
        'E', 'G', 'L', 'N', 'X', 'C', 'B'
};

const int   var_X  = 0,
            var_B  = 1,
            var_I  = 2,
            var_S1 = 3,
            var_S2 = 4,
            var_SC = 5;

const int numsolm = 13;   // number of solver status messages
const int nummodm = 19;   // number of model status messages
const int numsolprint = 5;// following four from 0...numxxxx

const int numhandlestat = 3;
const int numsolvelink = 7;
const int numsolveopt = 2;
const int numplatformcode = 4;

#if defined(__IN_CPPMEX__)
   #include "../generated/gmsspecs.inc.h"
#endif

}// namespace global::gmsspecs
