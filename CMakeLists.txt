cmake_minimum_required(VERSION 3.16)
project(gdxnative)
set(CMAKE_CXX_STANDARD 17)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

set(mylibs dl pthread)
if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
	set(mylibs "")
endif (MSVC)

set(common
		global/gmslibname.h global/gmslibname.cpp
		global/gmsspecs.h global/gmsspecs.cpp
		global/modhead.h global/modhead.cpp
		global/unit.h
		global/gmsglobx2.h global/gmsglobx2.cpp
		global/gmsglobx1.h global/gmsglobx1.cpp

		rtl/p3utils.h rtl/p3utils.cpp
		rtl/sysutils_p3.h rtl/sysutils_p3.cpp
		rtl/p3platform.h rtl/p3platform.cpp
		rtl/p3library.h rtl/p3library.cpp
		rtl/math_p3.h rtl/math_p3.cpp

		palxxx/gdlaudit.h palxxx/gdlaudit.cpp
		palxxx/paldoorg.h palxxx/paldoorg.cpp

		gdlib/gmsstrm.h gdlib/gmsstrm.cpp
		gdlib/strutilx.h gdlib/strutilx.cpp
		gdlib/xcompress.h gdlib/xcompress.cpp
		gdlib/gmsglob.h gdlib/gmsglob.cpp
		gdlib/gmsdata.h gdlib/gmsdata.cpp

		expertapi/gdxcc.h expertapi/gdxcc.c
		expertapi/gclgms.h expertapi/gclgms.c
		expertapi/gcmt.h expertapi/gcmt.c
		expertapi/palmcc.h expertapi/palmcc.c

		simplegdx/tgx.h simplegdx/tgx.cpp
		simplegdx/twostream.h simplegdx/twostream.cpp
		simplegdx/mistream.h simplegdx/mistream.cpp

		utils.h utils.cpp
		gxfile.h gxfile.cpp
		gdxinterface.h
		xpwrap.cpp xpwrap.h kvbuffers.h gxdefs.cpp gxdefs.h gdlib/datastorage.cpp gdlib/datastorage.h tests/gdxinterfacetests.cpp tests/xp_example1.cpp gdlib/runner.cpp gdlib/runner.h)

add_executable(gdxnativetest
		${common}
		tests/xptests.cpp
		tests/doctestmain.cpp
		tests/gxfiletests.cpp
		tests/xpwraptests.cpp tests/tgxtests.cpp)
target_link_libraries(gdxnativetest ${mylibs})