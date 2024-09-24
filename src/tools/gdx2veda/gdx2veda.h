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

class TMyHashList
{
public:
   int Compare( const library::short_string &PS1, const library::short_string &PS2 );
};

class THashTuple
{
private:
   TMyHashList StringStore;

   library::short_string Encode( int x, int n );
   void Decode( const library::short_string &s, int x, int n );

public:
   THashTuple();
   void Add( int x, int n, int mark );
   int Find( int x, int n, int &mark );
   int GetCount();
   int Get( int i, int &n );
   int GetSorted( int i, int &n );
   void SetText( int i, int t );
   int GetText( int i );
   int GetSortedText( int i );
   void ClearText();
};

class TProjection
{
private:
   library::one_indexed_container<THashTuple> tabs { GMS_MAX_INDEX_DIM };

public:
   TProjection();
   int AddUel( int dimension, int x, int n, int &mark );
   int AddInteger( int dimension, int k );
   int GetUel( int dimension, int i, int &n );
   int GetSortedUel( int dimension, int i, int &n );
};

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
