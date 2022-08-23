cmake_minimum_required(VERSION 3.16)
project(gdxnative)
set(CMAKE_CXX_STANDARD 17)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

set(common
		global/gmslibname.h global/gmslibname.cpp
		global/gmsspecs.h global/gmsspecs.cpp
		global/modhead.h global/modhead.cpp

		rtl/p3utils.h rtl/p3utils.cpp
		rtl/sysutils_p3.h rtl/sysutils_p3.cpp
		rtl/p3platform.h rtl/p3platform.cpp
		rtl/p3library.h rtl/p3library.cpp
		rtl/math_p3.h rtl/math_p3.cpp

		gdlib/gmsstrm.h gdlib/gmsstrm.cpp
		gdlib/strutilx.h gdlib/strutilx.cpp
		gdlib/xcompress.h gdlib/xcompress.cpp

		utils.h utils.cpp

		gxfile.h gxfile.cpp
		global/unit.h)

add_executable(gdxnativetest
		tests/doctestmain.cpp
		tests/gxfiletests.cpp
		${common})
target_link_libraries(gdxnativetest dl pthread)