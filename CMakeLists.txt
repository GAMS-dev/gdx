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
	set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -DZ_HAVE_UNISTD_H")
endif (UNIX)

# Makefile in devel/products/src/gdxio takes care of acquiring sources for zlib, skip in CMake builds so far
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(common
		src/gclgms.h
		src/gmsstrm.h src/gmsstrm.cpp
		src/gmsdata.h
		src/datastorage.h
		src/strhash.h
		src/gmsobj.h
        src/utils.h src/utils.cpp
		src/gxfile.h src/gxfile.cpp
		src/gdx.h
		# ZLIB modules
		# missing? run in project root: git clone https://github.com/madler/zlib zlib
		zlib/adler32.c zlib/compress.c zlib/crc32.c zlib/deflate.c zlib/gzclose.c zlib/gzlib.c zlib/gzread.c zlib/gzwrite.c zlib/infback.c zlib/inffast.c zlib/inflate.c zlib/inftrees.c zlib/trees.c zlib/uncompr.c zlib/zutil.c
)

set(tests
		src/tests/doctestmain.cpp
		src/tests/gdxtests.cpp
		src/tests/gxfiletests.cpp
        src/tests/gdxtests.h
		src/tests/utilstests.cpp
		src/tests/gmsobjtests.cpp
		src/tests/gmsdatatests.cpp
		src/tests/strhashtests.cpp
		src/tests/datastoragetests.cpp
)

#add_library(gdxcclib64 SHARED ${common})
#set_property(TARGET gdxcclib64 PROPERTY POSITION_INDEPENDENT_CODE ON)

add_executable(gdxtest ${common} ${tests})
target_include_directories(gdxtest PRIVATE zlib)
target_link_libraries(gdxtest ${mylibs})

add_executable(gdxwraptest src/tests/doctestmain.cpp src/tests/gdxtests.cpp src/tests/dummy/gdxcc.c)
target_link_libraries(gdxwraptest ${mylibs})
target_compile_options(gdxwraptest PRIVATE -DGXFILE_DUMMY -DGC_NO_MUTEX)

# Quickly run "include what you use" (https://include-what-you-use.org/) over project
#find_program(iwyu_path NAMES include-what-you-use iwyu REQUIRED)
#set_property(TARGET gdxcclib64 PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
#set_property(TARGET gdxtest PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
