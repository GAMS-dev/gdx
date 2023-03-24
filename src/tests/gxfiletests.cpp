#include "doctest.h"// for ResultBuilder, Expressi...
#include "../gxfile.h"

using namespace gdx;

namespace gdx::tests::gxfiletests
{
TEST_SUITE_BEGIN( "GDX object tests private" );

TEST_CASE( "Test checking if an identifier string is well formed" )
{
   REQUIRE_FALSE( IsGoodIdent( "" ) );
   REQUIRE( IsGoodIdent( "x" ) );
   REQUIRE( IsGoodIdent( "x6" ) );
   REQUIRE_FALSE( IsGoodIdent( "6x" ) );
   REQUIRE_FALSE( IsGoodIdent( "_" ) );
   REQUIRE( IsGoodIdent( "x_" ) );
   std::string longestValidUEL( 63, 'x' ), tooLong( 64, 'x' );
   REQUIRE( IsGoodIdent( longestValidUEL.c_str() ) );
   REQUIRE_FALSE( IsGoodIdent( tooLong.c_str() ) );
}

TEST_CASE( "Test 'can be quoted' function" )
{
   REQUIRE_FALSE( CanBeQuoted( nullptr ) );
   REQUIRE( CanBeQuoted( "abc" ) );
   REQUIRE( CanBeQuoted( "a\"bc" ) );
   REQUIRE( CanBeQuoted( "a'bc" ) );
   REQUIRE_FALSE( CanBeQuoted( "a'b\"c" ) );
}

TEST_CASE( "Test checking for good UEL string" )
{
   REQUIRE_FALSE( GoodUELString( nullptr, 0 ) );
   REQUIRE( GoodUELString( "abc", 3 ) );
   std::string tooLong( 64, 'x' );
   REQUIRE_FALSE( GoodUELString( tooLong.c_str(), tooLong.size() ) );
   std::string stillOk( 63, 'x' );
   REQUIRE( GoodUELString( stillOk.c_str(), stillOk.size() ) );
}

TEST_CASE( "Test making a good explanatory text function" )
{
   REQUIRE_FALSE( MakeGoodExplText( nullptr ) );

   char explTxt1[256] = "'Test 1' and \"Test 2\"\t";
   std::string expectedExplTxt1 { "'Test 1' and 'Test 2'?" };
   REQUIRE_EQ( expectedExplTxt1.length(), MakeGoodExplText( explTxt1 ) );
   REQUIRE( !strcmp( expectedExplTxt1.c_str(), explTxt1 ) );

   char explTxt2[256] = "\"Test 1\"\n and 'Test 2'";
   std::string expectedExplTxt2 { R"("Test 1"? and "Test 2")" };
   REQUIRE_EQ( expectedExplTxt2.length(), MakeGoodExplText( explTxt2 ) );
   REQUIRE( !strcmp( expectedExplTxt2.c_str(), explTxt2 ) );
}

TEST_CASE( "Test TIntegerMapping" )
{
   TIntegerMapping im;
   REQUIRE( im.empty() );
   im.SetMapping( 3, 5 );
   REQUIRE_GE( im.size(), 3 );
   REQUIRE_FALSE( im.empty() );
   REQUIRE_EQ( 3, im.GetHighestIndex() );
   REQUIRE_EQ( 5, im.GetMapping( 3 ) );
   im.SetMapping( 2048, 8 );
   REQUIRE_GE( im.size(), 2048 );
   REQUIRE_EQ( 2048, im.GetHighestIndex() );
   REQUIRE_FALSE( im.empty() );
   REQUIRE_EQ( 8, im.GetMapping( 2048 ) );
   REQUIRE_EQ( 5, im.GetMapping( 3 ) );
}

TEST_SUITE_END();

}// namespace gdx::tests::gxfiletests