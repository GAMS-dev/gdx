#ifndef GDX_GDXDIFF_H
#define GDX_GDXDIFF_H

#include <array>
#include <string>

// GDX library interface
#include "../../../generated/gdxcc.h"

namespace gdxdiff
{

enum class ErrorCode
{
   ERR_DIFFERENT = 1,
   ERR_USAGE,
   ERR_NOFILE,
   ERR_READGDX,
   ERR_LOADDLL,
   ERR_WRITEGDX,
   ERR_RENAME
};

enum class FldOnly
{
   fld_maybe,
   fld_yes,
   fld_no,
   fld_never
};

enum class TStatusCode
{
   sc_same,
   sc_notf1,
   sc_notf2,
   sc_dim,
   sc_typ,
   sc_key,
   sc_data,
   sc_dim10,
   sc_dim10_diff,
   sc_domain
};

// TODO: Name correctly
enum class KP
{
   // kp_input,
   kp_eps = 1,
   kp_releps,
   kp_output,
   kp_cmpfld,
   kp_settext,
   kp_ide,
   kp_id,
   kp_fldonly,
   kp_diffonly,
   kp_showdef,
   kp_cmpdomain,
   kp_matrixfile,
   kp_ignoreOrd
};

const std::array<std::string, 10> StatusText {
        "Symbols are identical",
        "Symbol not found in file 1",
        "Symbol not found in file 2",
        "Dimensions are different",
        "Types are different",
        "Keys are different",
        "Data are different",
        "Dim >= maxdim",
        "Dim >= maxdim & different",
        "Domains are different" };

const std::string
        c_ins1 = "ins1",
        c_ins2 = "ins2",
        c_dif1 = "dif1",
        c_dif2 = "dif2";

// Size of gdx::tvarvaltype
constexpr int tvarvaltype_size { 5 };

// Names of gdx::tvarvaltype(s)
const std::array<std::string, tvarvaltype_size> GamsFieldNames {
        "Level",
        "Marginal",
        "Lower",
        "Upper",
        "Scale" };

std::string ValAsString( const gdxHandle_t &PGX, double V );

void FatalErrorExit( int ErrNr );

void FatalError( const std::string &Msg, int ErrNr );

void FatalError2( const std::string &Msg1, const std::string &Msg2, int ErrNr );

void CheckGDXError( const gdxHandle_t &PGX );

void OpenGDX( const std::string &fn, gdxHandle_t &PGX );

void registerDiffUELs();

void CompareSy( int Sy1, int Sy2 );

bool GetAsDouble( const std::string &S, double &V );

void Usage();

// void CopyAcronyms( const gdxHandle_t &PGX );

void CheckFile( std::string &fn );

int main( int argc, const char *argv[] );

}// namespace gdxdiff

#endif//GDX_GDXDIFF_H
