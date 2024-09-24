#ifndef GDX_GDX2VEDA_H
#define GDX_GDX2VEDA_H

#include <string>
#include <fstream>

#include "container.h"
#include "vdd.h"
#include "../library/short_string.h"

// Global constants
#include "../../../generated/gclgms.h"
// GDX library interface
#include "../../../generated/gdxcc.h"

namespace gdx2veda
{

std::string StripExt( const std::string &fln );

std::string DQuotedStr( const std::string &s );

void WriteHeader( std::ofstream &f, const std::string &key, const std::string &value );

void ShortHelp();

void WriteDataLine();

void GetSpecialValues( const gdxHandle_t &PGX );

bool IsASpecialValue( double v, library::short_string &MappedValue, bool &IsAString );

void WriteValue( std::ofstream &f, double v );

std::string WritePV( char sepchar );

int main( int argc, const char *argv[] );

}// namespace gdx2veda

#endif//GDX_GDX2VEDA_H
