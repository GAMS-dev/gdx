#ifndef GDX_GDX2VEDA_H
#define GDX_GDX2VEDA_H

#include <string>
#include <fstream>
#include <vector>

#include "container.h"
#include "vdd.h"
#include "../library/short_string.h"

// Global constants
#include "../../../generated/gclgms.h"
// GDX library interface
#include "../../../generated/gdxcc.h"

namespace gdx2veda
{

class MyHashList_t
{
public:
   int Compare( const library::short_string &PS1, const library::short_string &PS2 );
};

class HashTuple_t
{
private:
   MyHashList_t StringStore;

   library::short_string Encode( int x, int n );
   void Decode( const library::short_string &s, int x, int n );

public:
   HashTuple_t();
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

class Projection_t
{
private:
   library::one_indexed_container<HashTuple_t> tabs { GMS_MAX_INDEX_DIM };

public:
   Projection_t();
   int AddUel( int dimension, int x, int n, int &mark );
   int AddInteger( int dimension, int k );
   int GetUel( int dimension, int i, int &n );
   int GetSortedUel( int dimension, int i, int &n );
};

class DataLineMapping_t
{
private:
   library::one_indexed_container<std::vector<int>> v { GMS_MAX_INDEX_DIM };

public:
   DataLineMapping_t();
   void Clear();
   void Insert( int Dimension, int TupleIndex, int EntryIndex );
   void WriteDataLines( int Indices, library::one_indexed_container<library::short_string> &Uels );
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

void CheckLiterals();

bool CheckLiteralFilter( const gdxUelIndex_t &Indices, int N );

double SpecialValueCheck( double d );

int main( int argc, const char *argv[] );

}// namespace gdx2veda

#endif//GDX_GDX2VEDA_H
