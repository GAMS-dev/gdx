/* global constants (symbol dimensions, symbol layout, etc.)
 *
 * Copyright (c) GAMS Development Corp. <support@gams.com>
 * Copyright (c) GAMS Software GmbH <support@gams.com>
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

#ifndef _GCLGMS_H_
#define _GCLGMS_H_

#define GLOBAL_MAX_INDEX_DIM  20
#define GLOBAL_UEL_IDENT_SIZE 64  /* implies len of 63 */
#define ITERLIM_INFINITY      2147483647
#define RESLIM_INFINITY       1e10
#define GMS_MAX_SOLVERS       150 /* maximum number of solver that can be stored */

#define GMS_MAX_INDEX_DIM     GLOBAL_MAX_INDEX_DIM
#define GMS_UEL_IDENT_SIZE    GLOBAL_UEL_IDENT_SIZE  /* implies len of 63 */

#define GMS_SSSIZE            256 /* short string size */

#define GMS_VARTYPE_UNKNOWN  0
#define GMS_VARTYPE_BINARY   1
#define GMS_VARTYPE_INTEGER  2
#define GMS_VARTYPE_POSITIVE 3
#define GMS_VARTYPE_NEGATIVE 4
#define GMS_VARTYPE_FREE     5
#define GMS_VARTYPE_SOS1     6
#define GMS_VARTYPE_SOS2     7
#define GMS_VARTYPE_SEMICONT 8
#define GMS_VARTYPE_SEMIINT  9
#define GMS_VARTYPE_MAX      10

/* base value used by GDX to store equation types:
 * returned as userInfo value by gdxSymbolInfoX */
#define GMS_EQU_USERINFO_BASE 53

#define GMS_EQUTYPE_E        0
#define GMS_EQUTYPE_G        1
#define GMS_EQUTYPE_L        2
#define GMS_EQUTYPE_N        3
#define GMS_EQUTYPE_X        4
#define GMS_EQUTYPE_C        5
#define GMS_EQUTYPE_B        6
#define GMS_EQUTYPE_MAX      7

/* a second name for this constant is not wanted but was introduced */
#define GMS_EQUEOFFSET      GMS_EQU_USERINFO_BASE

#define GMS_SETTYPE_DEFAULT    0
#define GMS_SETTYPE_SINGLETON  1
#define GMS_SETTYPE_MAX        2

#define GMS_VAL_LEVEL    0
#define GMS_VAL_MARGINAL 1
#define GMS_VAL_LOWER    2
#define GMS_VAL_UPPER    3
#define GMS_VAL_SCALE    4
#define GMS_VAL_MAX      5

enum gdxSpecValue {
  sv_valund  = 0,
  sv_valna   = 1,
  sv_valpin  = 2,
  sv_valmin  = 3,
  sv_valeps  = 4,
  sv_normal  = 5,
  sv_acronym = 6  };

#define GMS_SVIDX_UNDEF  0
#define GMS_SVIDX_NA     1
#define GMS_SVIDX_PINF   2
#define GMS_SVIDX_MINF   3
#define GMS_SVIDX_EPS    4
#define GMS_SVIDX_NORMAL 5
#define GMS_SVIDX_ACR    6
#define GMS_SVIDX_MAX    7

enum gdxSyType {
  dt_set   = 0,
  dt_par   = 1,
  dt_var   = 2,
  dt_equ   = 3,
  dt_alias = 4  };

#define GMS_DT_SET       0
#define GMS_DT_PAR       1
#define GMS_DT_VAR       2
#define GMS_DT_EQU       3
#define GMS_DT_ALIAS     4
#define GMS_DT_MAX       5

#define GMS_SV_UNDEF  1.0E300   /* undefined                */
#define GMS_SV_NA     2.0E300   /* not available/applicable */
#define GMS_SV_PINF   3.0E300   /* plus infinity            */
#define GMS_SV_MINF   4.0E300   /* minus infinity           */
#define GMS_SV_EPS    5.0E300   /* epsilon                  */
#define GMS_SV_ACR   10.0E300   /* potential/real acronym   */
#define GMS_SV_NAINT 2100000000 /* not available/applicable for integers */

#define STAT_OK    0
#define STAT_NOPT  1
#define STAT_INFES 2
#define STAT_UNBND 3
#define STAT_EVAL  4
#define STAT_UNKNW 5
#define STAT_REDEF 6
#define STAT_DEPND 7
#define STAT_REDIR 8
#define STAT_MAX   9

#define SS_MAX 14
#define MS_MAX 20

typedef double gdxSVals_t[GMS_SVIDX_MAX];
typedef double gdxValues_t[GMS_VAL_MAX];
typedef char   gdxStrIndex_t[GMS_MAX_INDEX_DIM][GMS_SSSIZE];
typedef char*  gdxStrIndexPtrs_t[GMS_MAX_INDEX_DIM];
typedef int    gdxUelIndex_t[GMS_MAX_INDEX_DIM];

#define GDXSTRINDEXPTRS_INIT(idx,idxPtrs) do { int GDXSTRINDEXPTRS_i; for (GDXSTRINDEXPTRS_i=0; GDXSTRINDEXPTRS_i < GMS_MAX_INDEX_DIM; GDXSTRINDEXPTRS_i++) (idxPtrs)[GDXSTRINDEXPTRS_i] = (idx)[GDXSTRINDEXPTRS_i]; } while (0)

static const char* const gmsGdxTypeText[GMS_DT_MAX] = {
   "Set",
   "Parameter",
   "Variable",
   "Equation",
   "Alias"
};

static const char* const gmsVarTypeText[GMS_VARTYPE_MAX] = {
   "Unknown",
   "Binary",
   "Integer",
   "Positive",
   "Negative",
   "Free",
   "Sos1",
   "Sos2",
   "Semicont",
   "Semiint"
};

static const char* const gmsValTypeText[GMS_VAL_MAX] = {
   ".l",
   ".m",
   ".lo",
   ".up",
   ".scale"
};

static const char* const gmsSVText[GMS_SVIDX_MAX] = {
   "UNdef",
   "NA",
   "+Inf",
   "-Inf",
   "Eps",
   "0",
   "AcroN"
};

static const double gmsSpecialValues[GMS_SVIDX_MAX] = {
   GMS_SV_UNDEF,
   GMS_SV_NA,
   GMS_SV_PINF,
   GMS_SV_MINF,
   GMS_SV_EPS,
   0.0,
   GMS_SV_ACR
};

static const double gmsDefRecVar[GMS_VARTYPE_MAX][GMS_VAL_MAX] = {
   /* .l   .m          .lo          .ub  .scale */
   { 0.0, 0.0,         0.0,         0.0, 1.0},    /* unknown */
   { 0.0, 0.0,         0.0,         1.0, 1.0},    /* binary */
   { 0.0, 0.0,         0.0, GMS_SV_PINF, 1.0},    /* integer */
   { 0.0, 0.0,         0.0, GMS_SV_PINF, 1.0},    /* positive */
   { 0.0, 0.0, GMS_SV_MINF,         0.0, 1.0},    /* negative */
   { 0.0, 0.0, GMS_SV_MINF, GMS_SV_PINF, 1.0},    /* free */
   { 0.0, 0.0,         0.0, GMS_SV_PINF, 1.0},    /* sos1 */
   { 0.0, 0.0,         0.0, GMS_SV_PINF, 1.0},    /* sos2 */
   { 0.0, 0.0,         1.0, GMS_SV_PINF, 1.0},    /* semicont */
   { 0.0, 0.0,         1.0, GMS_SV_PINF, 1.0}     /* semiint */
};

static const double gmsDefRecEqu[GMS_EQUTYPE_MAX][GMS_VAL_MAX] = {
   /* .l   .m          .lo          .ub  .scale */
   { 0.0, 0.0,         0.0,         0.0, 1.0},    /* =e= */
   { 0.0, 0.0,         0.0, GMS_SV_PINF, 1.0},    /* =g= */
   { 0.0, 0.0, GMS_SV_MINF,         0.0, 1.0},    /* =l= */
   { 0.0, 0.0, GMS_SV_MINF, GMS_SV_PINF, 1.0},    /* =n= */
   { 0.0, 0.0,         0.0,         0.0, 1.0},    /* =x= */
   { 0.0, 0.0,         0.0, GMS_SV_PINF, 1.0},    /* =c= */
   { 0.0, 0.0,         0.0,         0.0, 1.0}     /* =b= */
};

static const char* const rcStat[STAT_MAX] = {
   "     ",
   " NOPT",
   "INFES",
   "UNBND",
   " ****",
   "UNKNW",
   "REDEF",
   "DEPND",
   "REDIR"
};

static const char* const solveStatusTxt[SS_MAX] = {
   "NA                      ",
   "Normal Completion       ",
   "Iteration Interrupt     ",
   "Resource Interrupt      ",
   "Terminated By Solver    ",
   "Evaluation Interrupt    ",
   "Capability Problems     ",
   "Licensing Problems      ",
   "User Interrupt          ",
   "Setup Failure           ",
   "Solver Failure          ",
   "Internal Solver Failure ",
   "Solve Processing Skipped",
   "System Failure          "
};

static const char* const modelStatusTxt[MS_MAX] = {
   "NA                      ",
   "Optimal                 ",
   "Locally Optimal         ",
   "Unbounded               ",
   "Infeasible              ",
   "Locally Infeasible      ",
   "Intermediate Infeasible ",
   "Feasible Solution       ",
   "Integer Solution        ",
   "Intermediate Non-Integer",
   "Integer Infeasible      ",
   "Licensing Problem       ",
   "Error Unknown           ",
   "Error No Solution       ",
   "No Solution Returned    ",
   "Solved Unique           ",
   "Solved                  ",
   "Solved Singular         ",
   "Unbounded - No Solution ",
   "Infeasible - No Solution"
};

/* extract an equation type in [GMS_EQUTYPE_E,GMS_EQUTYPE_MAX)
 * from the userInfo value stored for an equation symbol,
 * or a negative value if the equation type was not stored
 */
#define gmsFixEquType(userInfo) ((userInfo) - GMS_EQU_USERINFO_BASE >= GMS_EQUTYPE_MAX ? -1 : (userInfo) - GMS_EQU_USERINFO_BASE)

/* extract a variable type in [GMS_VARTYPE_UNKNOWN,GMS_VARTYPE_MAX)
 * from the userInfo value stored for a variable symbol,
 * or a negative value if the variable type was not stored
 */
#define gmsFixVarType(userInfo) ((userInfo) >= GMS_VARTYPE_MAX ? -1 : (userInfo))

#endif /* ifndef _GCLGMS_H_ */
