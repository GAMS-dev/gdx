#ifndef GDX_GDXDIFF_H
#define GDX_GDXDIFF_H

#include <array>
#include <string>
#include <cstdint>
#include <string_view>

#include "../library/common.h"
#include "../library/short_string.h"

// GDX library interface
#include "../../../generated/gdxcc.h"

namespace gdxdiff
{

using namespace std::literals::string_literals;

enum class ErrorCode : uint8_t
{
   ERR_DIFFERENT = 1,
   ERR_USAGE,
   ERR_NOFILE,
   ERR_READGDX,
   ERR_LOADDLL,
   ERR_WRITEGDX,
   ERR_RENAME
};

enum class FldOnly : uint8_t
{
   fld_maybe,
   fld_yes,
   fld_no,
   fld_never
};

enum class TStatusCode : uint8_t
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
enum class KP : uint8_t
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
   kp_ignoreOrd,
   kp_skip_id
};

constexpr std::array StatusText {
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
        c_ins1 { "ins1" },
        c_ins2 { "ins2" },
        c_dif1 { "dif1" },
        c_dif2 { "dif2" };

// Names of gdx::tvarvaltype(s)
const std::array GamsFieldNames {
        "Level"s,
        "Marginal"s,
        "Lower"s,
        "Upper"s,
        "Scale"s };

std::string ValAsString( const gdxHandle_t &PGX, double V );

void FatalErrorExit( int ErrNr );

void FatalError( const std::string &Msg, int ErrNr );

void FatalError2( const std::string &Msg1, const std::string &Msg2, int ErrNr );

void CheckGDXError( const gdxHandle_t &PGX );

void OpenGDX( const library::short_string &fn, gdxHandle_t &PGX );

void registerDiffUELs();

void CompareSy( int Sy1, int Sy2 );

bool GetAsDouble( const library::short_string &S, double &V );

void Usage( const library::AuditLine &AuditLine );

// void CopyAcronyms( const gdxHandle_t &PGX );

void CheckFile( library::short_string &fn );

int main( int argc, const char *argv[] );

}// namespace gdxdiff

#endif//GDX_GDXDIFF_H
