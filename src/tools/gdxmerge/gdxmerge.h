#ifndef GDX_GDXMERGE_H
#define GDX_GDXMERGE_H

#include <ctime>

namespace gdxmerge
{

enum class TProcessPass
{
   rpDoAll,
   rpScan,
   rpSmall,
   rpBig,
   rpTooBig
};

std::string format_date_time( const std::tm &dt );

int main( int argc, const char *argv[] );

}// namespace gdxmerge

#endif//GDX_GDXMERGE_H
