cmake_minimum_required(VERSION 3.16)
project(gdxnative)
set(CMAKE_CXX_STANDARD 17)

# dump GDX data also as YAML representation
set(ENABLE_YAML 1)

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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGAMSBUILD")

if(ENABLE_YAML)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DYAML")
endif(ENABLE_YAML)

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

		# ZLIB modules
		zlib/adler32.c zlib/compress.c zlib/crc32.c zlib/deflate.c zlib/gzclose.c zlib/gzlib.c zlib/gzread.c zlib/gzwrite.c zlib/infback.c zlib/inffast.c zlib/inflate.c zlib/inftrees.c zlib/trees.c zlib/uncompr.c zlib/zutil.c
)
if(ENABLE_YAML)
	set(common ${common} yaml.h yaml.cpp)
endif(ENABLE_YAML)

set(common_neo ${common} cwrap.h cwrap.cpp)

set(tests
		tests/doctestmain.cpp
		tests/gxfiletests.cpp
		tests/utilstests.cpp
		tests/gmsobjtests.cpp
		tests/gmsdatatests.cpp
		tests/strhashtests.cpp
		tests/gmsheapnewtests.cpp
)

add_library(gdxcclib64 SHARED ${common_neo})
set_property(TARGET gdxcclib64 PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(gdxnativeshared STATIC ${common})
add_executable(gdxnativetest ${tests})
target_link_libraries(gdxnativetest gdxnativeshared ${mylibs})
