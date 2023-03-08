cmake_minimum_required(VERSION 3.16)
project(gdx)
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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNO_ZLIB")

set(common
		src/gclgms.h src/gclgms.c
		src/gmsstrm.h src/gmsstrm.cpp
		src/gmsdata.h
		src/datastorage.h
		src/strhash.h
		src/gmsobj.h src/gmsobj.cpp
		src/gmsheapnew.h src/gmsheapnew.cpp
		src/utils.h src/utils.cpp
		src/gxfile.h src/gxfile.cpp
		src/cwrap.h src/cwrap.cpp
)

set(tests
		src/tests/doctestmain.cpp
		src/tests/gxfiletests.cpp
		src/tests/utilstests.cpp
		src/tests/gmsobjtests.cpp
		src/tests/gmsdatatests.cpp
		src/tests/strhashtests.cpp
		src/tests/gmsheapnewtests.cpp
)

add_library(gdxcclib64 SHARED ${common})
set_property(TARGET gdxcclib64 PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(gdxshared STATIC ${common})
add_executable(gdxtest ${tests})
target_link_libraries(gdxtest gdxshared ${mylibs})
