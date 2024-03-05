cmake_minimum_required(VERSION 3.16)
project(gdx)
set(CMAKE_CXX_STANDARD 17)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

set(mylibs dl pthread m)

set(CMAKE_SKIP_RPATH FALSE)
set(CMAKE_SKIP_BUILD_RPATH FALSE)

if (APPLE)
	set(CMAKE_BUILD_RPATH @executable_path/.)
	if(CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 14)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-non-prototype")
	endif ()
else()
	set(CMAKE_BUILD_RPATH $ORIGIN)
endif (APPLE)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGC_NO_MUTEX")
if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /W3 /EHs /D_CRT_SECURE_NO_WARNINGS /wd4267 /wd4996")
	set(mylibs "")
endif (MSVC)

if (UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wreturn-type -Wmissing-declarations -Wno-unknown-pragmas")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DZ_HAVE_UNISTD_H")
	if(NOT APPLE)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat-truncation=0")
	endif (NOT APPLE)
endif (UNIX)

set(common
		src/gmsstrm.h src/gmsstrm.cpp
		src/gmsdata.h
		src/batchalloc.h
		src/datastorage.h
		src/strhash.h
		src/gmsobj.h
        src/utils.h src/utils.cpp
		src/gxfile.h src/gxfile.cpp
		src/gdx.h
		# ZLIB modules
		# missing? run in project root: git clone https://github.com/madler/zlib zlib
		zlib/adler32.c zlib/compress.c zlib/crc32.c zlib/deflate.c zlib/gzclose.c zlib/gzlib.c zlib/gzread.c
		zlib/gzwrite.c zlib/infback.c zlib/inffast.c zlib/inflate.c zlib/inftrees.c zlib/trees.c zlib/uncompr.c
		zlib/zutil.c
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

set(inc-dirs zlib src generated)

# Dynamic library / shared object
add_library(gdxcclib64 SHARED ${common} generated/gdxcclib.cpp )
if(UNIX)
	target_compile_options(gdxcclib64 PRIVATE -fvisibility=hidden)
endif()
target_include_directories(gdxcclib64 PRIVATE ${inc-dirs})
if (APPLE)
	set(cclib-link-options "-Bdynamic")
elseif(UNIX) # Linux
	set(cclib-link-options "-Bdynamic -Wl,-Bsymbolic")
else() # Windows
	set(cclib-link-options "")
endif()
target_link_libraries(gdxcclib64 ${mylibs} ${cclib-link-options})
set_property(TARGET gdxcclib64 PROPERTY POSITION_INDEPENDENT_CODE ON)

# Static library
add_library(gdx-static STATIC ${common})
target_include_directories(gdx-static PRIVATE ${inc-dirs})
set_property(TARGET gdx-static PROPERTY POSITION_INDEPENDENT_CODE ON)

# Unit test suite (against statically compiled GDX)
add_executable(gdxtest ${tests})
target_include_directories(gdxtest PRIVATE ${inc-dirs})
target_link_libraries(gdxtest gdx-static ${mylibs})

# Unit test suite (against GDX dynamic library)
add_executable(gdxwraptest src/tests/doctestmain.cpp src/tests/gdxtests.cpp generated/gdxcc.c)
target_link_libraries(gdxwraptest ${mylibs})
target_include_directories(gdxwraptest PRIVATE src generated)
target_compile_options(gdxwraptest PRIVATE -DGXFILE_CPPWRAP -DGC_NO_MUTEX)

# Quickly run "include what you use" (https://include-what-you-use.org/) over project
#find_program(iwyu_path NAMES include-what-you-use iwyu REQUIRED)
#set_property(TARGET gdxcclib64 PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
#set_property(TARGET gdxtest PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})

# Standalone GDX example program 1
add_executable(xp_example1 ${common} src/examples/xp_example1.cpp)
target_include_directories(xp_example1 PRIVATE ${inc-dirs})
target_link_libraries(xp_example1 ${mylibs})

# Standalone GDX example program 2
add_executable(xp_example2 ${common} src/examples/xp_example2.cpp generated/optcc.c)
target_include_directories(xp_example2 PRIVATE ${inc-dirs})
target_link_libraries(xp_example2 ${mylibs})

# Standalone GDX example program 3 (associative "supply.demand" str -> double (level))
add_executable(xp_associative ${common} src/examples/xp_associative.cpp)
target_include_directories(xp_associative PRIVATE ${inc-dirs})
target_link_libraries(xp_associative ${mylibs})

# Standalone GDX example program 4 (associative {supply,demand} vector -> double (level))
add_executable(xp_associative_vec ${common} src/examples/xp_associative_vec.cpp)
target_include_directories(xp_associative_vec PRIVATE ${inc-dirs})
target_link_libraries(xp_associative_vec ${mylibs})

# Standalone GDX example program 5 (data write)
# Needs path to GAMS distribution in environment variable GAMSDIR
# (either directly or as first entry of semicolon separated list)
if(DEFINED ENV{GAMSDIR})
	set(ENTRIES $ENV{GAMSDIR})
	string(FIND "${ENTRIES}" ";" index)
	if(index GREATER -1) # contains multiple semicolon separated entries
		string(REPLACE ";" "\n" ENTRIES "${ENTRIES}")
		string(REGEX MATCH "^([^\n]*)" FIRST_ENTRY "${ENTRIES}")
		string(REPLACE "\\" "/" FIRST_ENTRY "${FIRST_ENTRY}")
		set(cur-apifiles-path ${FIRST_ENTRY})
	else() # just one entry
		set(cur-apifiles-path ${ENTRIES})
	endif()
	string(APPEND cur-apifiles-path "/apifiles/C/api/")
	if(EXISTS ${cur-apifiles-path}gmdcc.c)
		message(STATUS "Found expert-level API sources and headers in ${cur-apifiles-path}")
		add_executable(xp_dataWrite ${common} src/examples/xp_dataWrite.cpp ${cur-apifiles-path}gmdcc.c)
		target_include_directories(xp_dataWrite PRIVATE ${inc-dirs} ${cur-apifiles-path})
		target_link_libraries(xp_dataWrite ${mylibs})
		if(UNIX)
			target_compile_options(xp_dataWrite PRIVATE -DGC_NO_MUTEX)
		endif()
	endif()
endif()
