cmake_minimum_required(VERSION 3.16)
project(gdxnative)
set(CMAKE_CXX_STANDARD 17)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

set(mylibs dl pthread)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGC_NO_MUTEX")
if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /W3 /EHs /D_CRT_SECURE_NO_WARNINGS")
	set(mylibs "")
endif (MSVC)

if (UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wreturn-type -Wmissing-declarations")
endif (UNIX)

# Makefile in devel/products/src/gdxio takes care of acquiring sources for zlib, skip in CMake builds so far
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNO_ZLIB -DGAMSBUILD")

set(common
		gclgms.h gclgms.c
		# Only needed for -DGAMSBUILD
		palmcc.h palmcc.c
		gmsstrm.h gmsstrm.cpp
		gmsdata.h
		datastorage.h
		strhash.h
		gmsobj.h gmsobj.cpp
		gmsheapnew.h gmsheapnew.cpp
		utils.h utils.cpp
		gxfile.h gxfile.cpp
		cwrap.h cwrap.cpp
)

set(tests
		tests/doctestmain.cpp
		tests/gxfiletests.cpp
		tests/utilstests.cpp
		tests/gmsobjtests.cpp
		tests/gmsdatatests.cpp
		tests/strhashtests.cpp
		tests/gmsheapnewtests.cpp
)

add_library(gdxcclib64 SHARED ${common})
set_property(TARGET gdxcclib64 PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(gdxnativeshared STATIC ${common})
add_executable(gdxnativetest ${tests})
target_link_libraries(gdxnativetest gdxnativeshared ${mylibs})
