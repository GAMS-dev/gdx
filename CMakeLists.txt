cmake_minimum_required(VERSION 3.16)
project(gdx)
set(CMAKE_CXX_STANDARD 17)

# ccache and sccache
if(NOT DEFINED CMAKE_C_COMPILER_LAUNCHER AND NOT DEFINED CMAKE_CXX_COMPILER_LAUNCHER)
	find_program(CCACHE_FOUND ccache)
	find_program(SCCACHE_FOUND sccache)
	if (SCCACHE_FOUND) # using sccache on Windows for MSVC cl.exe compatibility!
		message("-- Found sccache at: ${SCCACHE_FOUND}")
		set(CMAKE_C_COMPILER_LAUNCHER ${SCCACHE_FOUND})
		set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE_FOUND})
		if(CMAKE_BUILD_TYPE STREQUAL "Debug")
			string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
			string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
		elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
			string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
			string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
		elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
			string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
			string(REPLACE "/Zi" "/Z7" CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}")
		endif()
	elseif(CCACHE_FOUND)
		message("-- Found ccache at: ${CCACHE_FOUND}")
		set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_FOUND})
		set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_FOUND})
	endif()
endif()

set(mylibs ${CMAKE_DL_LIBS} pthread m)

#set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_SKIP_RPATH FALSE)
set(CMAKE_SKIP_BUILD_RPATH FALSE)

if ((APPLE OR CMAKE_C_COMPILER_ID STREQUAL "Clang") AND CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 14)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-non-prototype")
endif ()

if (APPLE)
	set(CMAKE_BUILD_RPATH @executable_path/.)
else ()
	set(CMAKE_BUILD_RPATH $ORIGIN)
endif (APPLE)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DGC_NO_MUTEX")

if (MSVC OR CMAKE_HOST_WIN32 OR CMAKE_GENERATOR_TOOLSET STREQUAL "ClangCL")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W3 /D_CRT_SECURE_NO_WARNINGS /wd4267 /wd4996")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /EHs /D_CRT_SECURE_NO_WARNINGS /wd4267 /wd4996")
	if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
	else ()
		# use strict floating point mode and not fast, as some 0.0 become eps in GDX files in fast mode for Intel clang
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:precise -Wno-microsoft-include")
	endif ()
	if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-unused-but-set-variable -Wno-unused-variable")
	endif ()
	set(mylibs "")
endif ()

if (UNIX)
	# -fsanitize=undefined -fno-inline
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wreturn-type -Wmissing-declarations -Wno-unknown-pragmas -pedantic -Wextra -Wno-unused-parameter -Wno-deprecated-copy")
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wconversion -funsigned-char") # aggressive signage warnings
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DZ_HAVE_UNISTD_H")
	if (NOT APPLE AND NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat-truncation=0 -Wno-cast-function-type")
	endif ()
	# strictly handle alignment and structure padding around flexible arrays (only newer compilers)
	if ((CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16) OR
	(CMAKE_CXX_COMPILER_ID MATCHES "GCC" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13))
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstrict-flex-arrays=3")
	endif ()
	# ADD common hardening features on Linux
	if (CMAKE_SYSTEM_NAME MATCHES "Linux")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 ")
		add_link_options(LINKER:-z,defs LINKER:-z,now LINKER:-z,relro LINKER:-z,noexecstack)
	endif ()
endif (UNIX)

set(zlib-units
		# ZLIB modules
		# missing? run in project root: git clone https://github.com/madler/zlib zlib
		zlib/adler32.c zlib/compress.c zlib/crc32.c zlib/deflate.c zlib/gzclose.c zlib/gzlib.c zlib/gzread.c
		zlib/gzwrite.c zlib/infback.c zlib/inffast.c zlib/inflate.c zlib/inftrees.c zlib/trees.c zlib/uncompr.c
		zlib/zutil.c
)

set(gdx-core
		# P3 runtime library
		src/rtl/dtoaLoc.h
		src/rtl/dtoaLoc.c
		src/rtl/p3io.hpp
		src/rtl/p3io.cpp
		src/rtl/p3utils.hpp
		src/rtl/p3utils.cpp
		src/rtl/sysutils_p3.hpp
		src/rtl/sysutils_p3.cpp
		src/rtl/p3platform.hpp
		src/rtl/p3platform.cpp
		src/rtl/math_p3.hpp
		src/rtl/math_p3.cpp

		# gdlib essential units
		src/gdlib/gmsstrm.cpp
		src/gdlib/gmsstrm.hpp
		src/gdlib/utils.hpp
		src/gdlib/utils.cpp
		src/gdlib/strutilx.hpp
		src/gdlib/strutilx.cpp
		src/gdlib/strindexbuf.hpp

		# GDX specific units
		src/gxfile.hpp
		src/gxfile.cpp
		src/gdx.hpp

		${zlib-units}
)

set(tests
		src/tests/doctestmain.cpp

		src/tests/gdxtests.hpp
		src/tests/gdxtests.cpp
		src/tests/gxfiletests.cpp

		src/tests/gdlib/gmsdatatests.cpp
		src/tests/gdlib/gmsobjtests.cpp
		src/tests/gdlib/gmsstrmtests.cpp
		src/tests/gdlib/strhashtests.cpp
		src/tests/gdlib/strutilxtests.cpp
		src/tests/gdlib/utilstests.cpp

		src/tests/rtl/p3iotests.cpp
		src/tests/rtl/p3utilstests.cpp
		src/tests/rtl/sysutilsp3tests.cpp
)

set(inc-dirs zlib src generated src/gdlib src/global src/rtl)

include(targets.cmake)
