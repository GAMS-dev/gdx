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

#include <map>
#include <string>
#include <array>
#include <set>

// ==============================================================================================================
// Interface
// ==============================================================================================================
namespace global::ctvproc
{
enum tprocname
{
   procnone,
   proclp,
   procmip,
   procrmip,
   procnlp,
   procmcp,
   procmpec,
   procrmpec,
   proccns,
   procdnlp,
   procrminlp,
   procminlp,
   procqcp,
   procmiqcp,
   procrmiqcp,
   procemp,
   proccount
};

const int procnameslength = 16;
const std::array<std::string, procnameslength> procnames = {
        "NONE", "LP", "MIP", "RMIP", "NLP", "MCP", "MPEC", "RMPEC", "CNS", "DNLP", "RMINLP", "MINLP", "QCP", "MIQCP", "RMIQCP", "EMP" };

const std::set<tprocname> pshortform = { procmcp, proccns },
                          pdiscrete = { procmip, procrmip, procmpec, procrmpec, procrminlp, procminlp, procmiqcp, procrmiqcp, procemp },
                          pnonlinear = { procnlp, procmcp, procmpec, procrmpec, proccns, procdnlp, procrminlp, procminlp, procqcp, procmiqcp, procrmiqcp, procemp },
                          pcontinuous = { procnlp, procmcp, procmpec, proccns, procrminlp, procminlp, procqcp, procmiqcp, procrmiqcp, procemp },
                          pdiscontinuous = { procdnlp, procrminlp, procminlp, procemp },
                          pdrelaxed = { procrmip, procrmpec, procrminlp, procrmiqcp },
                          pdtrylinear = { procnlp, procdnlp, procrminlp, procminlp, procqcp, procmiqcp, procrmiqcp },
                          pdskiplist = { procnone },
                          pdpairs = { procmcp, procmpec, procrmpec, proccns, procemp };

const std::map<tprocname, std::string> proctexts = {
        { procnone, "NONE" },
        { proclp, "Linear Programming" },
        { procmip, "Mixed-Integer Programming" },
        { procrmip, "Relaxed Mixed-Integer Programming" },
        { procnlp, "Non-Linear Programming" },
        { procmcp, "Mixed Complementarity Problems" },
        { procmpec, "Mathematical Programs with Equilibrium Constraints" },
        { procrmpec, "Relaxed Mathematical Programs with Equilibrium Constraints" },
        { proccns, "Constrained Nonlinear Systems" },
        { procdnlp, "Non-Linear Programming with Discontinuous Derivatives" },
        { procrminlp, "Relaxed Mixed-Integer Non-Linear Programming" },
        { procminlp, "Mixed-Integer Non-Linear Programming" },
        { procqcp, "Quadratically Constrained Programs" },
        { procmiqcp, "Mixed Integer Quadratically Constrained Programs" },
        { procrmiqcp, "Relaxed Mixed Integer Quadratically Constrained Programs" },
        { procemp, "Extended Mathematical Programs" } };

tprocname getprocname( const std::string &id );
std::string getproctext( tprocname p );

extern tprocname proctype;
}// namespace global::ctvproc
