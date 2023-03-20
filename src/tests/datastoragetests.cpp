#include "../datastorage.h"
#include "doctest.h"
#include <random>
#include <array>

using namespace std::literals::string_literals;
using namespace gdx::collections::datastorage;

namespace tests::datastoragetests
{

TEST_SUITE_BEGIN( "gdlib::datastorage" );

TEST_CASE( "Simple use of linked data" )
{
   TLinkedData<int, double> ld{ 1, 1 * (int) sizeof( double ) };
   std::vector<int> keys( 1 );
   std::array<double, 1> vals{ 23.0 };
   for( int i{}; i < 4; i++ )
   {
      keys.front() = 4 - i;// insert in reverse order
      auto node = ld.AddItem( keys.data(), vals.data() );
      REQUIRE_EQ( keys.front(), ( (int *) node->RecData )[0] );
      REQUIRE_EQ( 23.0, *( (double *) &node->RecData[(int) sizeof( int )] ) );
   }
   REQUIRE_EQ( 4, ld.Count() );
   auto it = ld.StartRead();// this calls sort!
   for( int i{}; i < 4; i++ )
   {
      bool res = ld.GetNextRecord( &it.value(), keys.data(), vals.data() );
      REQUIRE( ( i == 3 || res ) );
      REQUIRE_EQ( i + 1, keys.front() );
   }
}

TEST_SUITE_END();

}// namespace tests::datastoragetests
