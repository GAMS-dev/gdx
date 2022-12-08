cmake_minimum_required(VERSION 3.16)
project(gdxnative)
set(CMAKE_CXX_STANDARD 17)

#set(CMAKE_SHARED_LIBRARY_PREFIX "")

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

set(mylibs dl pthread)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGC_NO_MUTEX")
if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
	set(mylibs "")
endif (MSVC)

# -DYAML
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGAMSBUILD")

set(common
		expertapi/gclgms.h expertapi/gclgms.c

		rtl/p3utils.h rtl/p3utils.cpp
		rtl/sysutils_p3.h rtl/sysutils_p3.cpp

		# Only needed for -DGAMSBUILD
		expertapi/palmcc.h expertapi/palmcc.c

		gdlib/gmsstrm.h gdlib/gmsstrm.cpp
		# TODO: Replace with std::filesystem, ... standard library stuff
		gdlib/strutilx.h gdlib/strutilx.cpp

		gdlib/gmsdata.h gdlib/gmsdata.cpp
		gdlib/datastorage.cpp gdlib/datastorage.h
		gdlib/strhash.h gdlib/strhash.cpp

		gdxinterface.h gdxinterface.cpp
		utils.h utils.cpp
		gxfile.h gxfile.cpp

		# TODO: gclgms.h/c
		gxdefs.cpp gxdefs.h

		# Do not compile into production DLL: ifdef YAMLDEBUG
		yaml.h yaml.cpp

		# ZLIB modules
		zlib/adler32.c zlib/compress.c zlib/crc32.c zlib/deflate.c zlib/gzclose.c zlib/gzlib.c zlib/gzread.c zlib/gzwrite.c zlib/infback.c zlib/inffast.c zlib/inflate.c zlib/inftrees.c zlib/trees.c zlib/uncompr.c zlib/zutil.c
)

set(common_legacy ${common}
	expertapi/gdxcc.h expertapi/gdxcc.c
	xpwrap.cpp xpwrap.h
)

set(common_neo ${common} cwrap/cwrap.h cwrap/cwrap.cpp)

set(tests
		tests/xptests.cpp
		tests/doctestmain.cpp
		tests/gxfiletests.cpp
		tests/xpwraptests.cpp
		#tests/tgxtests.cpp
		tests/gdxinterfacetests.cpp
		tests/xp_example1.cpp
		tests/serializetests.cpp
)

add_library(gdxcclib64 SHARED ${common_neo})
set_property(TARGET gdxcclib64 PROPERTY POSITION_INDEPENDENT_CODE ON)

if(IS_DIRECTORY /usr/include/python3.10)
	add_library(pygdx SHARED pygdx/pygdx.c)
	set_property(TARGET pygdx PROPERTY POSITION_INDEPENDENT_CODE ON)
	target_include_directories(pygdx PUBLIC /usr/include/python3.10)
	target_link_libraries(pygdx gdxcclib64)
endif()

add_library(gdxnativeshared STATIC ${common_legacy})
add_executable(gdxnativetest ${tests})
target_link_libraries(gdxnativetest gdxnativeshared ${mylibs})

#add_executable(unicode pygdx/unicode.cpp)