#ifndef GDX_GDXDUMP_H
#define GDX_GDXDUMP_H

#include <string>
#include <functional>
#include <cstdint>
#include <array>

namespace gdxdump
{

enum class TOutFormat
{
   fmt_none,
   fmt_normal,
   fmt_gamsbas,
   fmt_csv
};

enum class TDblFormat
{
   dbl_none,
   dbl_normal,
   dbl_hexponential,
   dbl_hexBytes
};

char QQ( const std::string &s );

std::string QQCSV( const std::string &s );

// Function is commented out in the Delphi source code
// std::string QUEL(const std::string &S);

// Auxiliary function for WriteQText and WriteQUELText
void WriteQuotedCommon( const std::string &S, const std::function<bool( char )> &isSpecialPredicate, int i = {}, bool G = true );

void WriteQText( const std::string &S, bool checkParenthesis );

void WriteQUELText( const std::string &S );

void WriteUEL( const std::string &S );

void WriteUELTable( const std::string &name );

// Auxiliary function for WrVal
std::string specialValueStrClassic( int i );

union TI64Rec
{
   double x;
   int64_t i64;
   std::array<uint8_t, 8> bytes;
};

// Auxiliary function for DblToStrHex
char hexDigit( uint8_t b );

// Auxiliary function for DblToStrHex and dblDecomp (called in main)
void initDblUtilValues();

// Auxiliary function for WrVal
std::string DblToStrHex( double x );

// Auxiliary function for DblToStrHexponential
void dblDecomp( double x, bool &isNeg, uint32_t &expo, int64_t &mant );

// Auxiliary function for DblToStrHexponential
std::string mFormat( int64_t m );

// Auxiliary function for WrVal
std::string DblToStrHexponential( double x );

void WrVal( double V );

std::string GetUELAsString( int N );

std::string GetUel4CSV( int N );

bool WriteSymbolAsItem( int SyNr, bool DomInfo );

void WriteSymbolsAsSet( bool DomInfo );

// Auxiliary function for assertWithMessage and main
void printErrorMessage( const std::string &message );

// Auxiliary function for WriteSymbol, WriteSymbolCSV and WriteSetText
void assertWithMessage( bool expression, const std::string &message );

// Auxiliary function for WriteSymbol
std::string valTypStrClassic( int i );

// Auxiliary function for WriteSymbol
std::string varTypStrClassic( int i );

void WriteSymbol( int SyNr );

// Auxiliary function for WriteSymbolCSV and WriteSetText
int64_t delphiRound( double x );

void WriteSymbolCSV( int SyNr );

// Auxiliary function for WriteSymbolInfo and WriteDomainInfo
int getIntegerWidth( int number );

void WriteSymbolInfo();

void WriteDomainInfo();

void WriteSetText();

void Usage();

std::string NextParam();

void WriteAcronyms();

int main( int argc, const char *argv[] );

}// namespace gdxdump

#endif//GDX_GDXDUMP_H
