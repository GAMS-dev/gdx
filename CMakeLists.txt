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
		gdlib/datastorage.cpp gdlib/datastorage.h
		gdlib/runner.cpp gdlib/runner.h

		expertapi/palmcc.h expertapi/palmcc.c
		expertapi/gcmt.h expertapi/gcmt.c

		simplegdx/tgx.h simplegdx/tgx.cpp
		simplegdx/twostream.h simplegdx/twostream.cpp
		simplegdx/mistream.h simplegdx/mistream.cpp

		gdxinterface.h gdxinterface.cpp
		kvbuffers.h
		utils.h utils.cpp
		gxfile.h gxfile.cpp
		xpwrap.cpp xpwrap.h
		gxdefs.cpp gxdefs.h
		yaml.h yaml.cpp
)

set(common_legacy ${common}
	expertapi/gdxcc.h expertapi/gdxcc.c
	expertapi/gclgms.h expertapi/gclgms.c
)

set(common_neo ${common} cwrap/cwrap.h cwrap/cwrap.cpp)

set(tests
		tests/xptests.cpp
		tests/doctestmain.cpp
		tests/gxfiletests.cpp
		tests/xpwraptests.cpp
		tests/tgxtests.cpp
		tests/gdxinterfacetests.cpp
		tests/xp_example1.cpp
		tests/serializetests.cpp
)

add_library(gdxcwrap SHARED ${common_neo})
set_property(TARGET gdxcwrap PROPERTY POSITION_INDEPENDENT_CODE ON)

if(IS_DIRECTORY /usr/include/python3.10)
	add_library(pygdx SHARED pygdx/pygdx.c)
	set_property(TARGET pygdx PROPERTY POSITION_INDEPENDENT_CODE ON)
	target_include_directories(pygdx PUBLIC /usr/include/python3.10)
	target_link_libraries(pygdx gdxcwrap)
endif()

add_library(gdxnativeshared STATIC ${common_legacy})
add_executable(gdxnativetest ${tests})
target_link_libraries(gdxnativetest gdxnativeshared ${mylibs})

#add_executable(unicode pygdx/unicode.cpp)