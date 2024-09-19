#ifndef GDX_GDX2VEDA_H
#define GDX_GDX2VEDA_H

#include <string>

#include "container.h"
#include "vdd.h"
#include "../library/short_string.h"

// Global constants
#include "../../../generated/gclgms.h"
// GDX library interface
#include "../../../generated/gdxcc.h"

namespace gdx2veda
{

void ShortHelp();

void WriteDataLine();

void GetSpecialValues( const gdxHandle_t &PGX );

int main( int argc, const char *argv[] );

}// namespace gdx2veda

#endif//GDX_GDX2VEDA_H
