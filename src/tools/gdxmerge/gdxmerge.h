#ifndef GDX_GDXMERGE_H
#define GDX_GDXMERGE_H

#include <ctime>

namespace gdxmerge
{

enum class TProcessPass
{
   rp_do_all,
   rp_scan,
   rp_small,
   rp_big,
   rp_tooBig
};

std::string format_date_time( const std::tm &dt );

int main( int argc, const char *argv[] );

}// namespace gdxmerge

#endif//GDX_GDXMERGE_H
