cmake_minimum_required(VERSION 3.16)
project(gdx)
set(CMAKE_CXX_STANDARD 17)

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif (CCACHE_FOUND)

set(mylibs dl pthread m)

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
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /W3 /EHs /D_CRT_SECURE_NO_WARNINGS /wd4267 /wd4996")
    set(mylibs "")
endif (MSVC)

if (UNIX)
    # -fsanitize=undefined
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wreturn-type -Wmissing-declarations -Wno-unknown-pragmas")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DZ_HAVE_UNISTD_H")
    if (NOT APPLE AND NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat-truncation=0")
    endif ()
endif (UNIX)

include(base-units.cmake)

set(zlib-units
        # ZLIB modules
        # missing? run in project root: git clone https://github.com/madler/zlib zlib
        zlib/adler32.c zlib/compress.c zlib/crc32.c zlib/deflate.c zlib/gzclose.c zlib/gzlib.c zlib/gzread.c
        zlib/gzwrite.c zlib/infback.c zlib/inffast.c zlib/inflate.c zlib/inftrees.c zlib/trees.c zlib/uncompr.c
        zlib/zutil.c
)

set(gdx-core
        # gdlib essential units
        src/gdlib/gmsstrm.cpp
        src/gdlib/gmsstrm.h
        src/gdlib/utils.h
        src/gdlib/utils.cpp

        # GDX specific units
        src/gxfile.h
        src/gxfile.cpp
        src/gdx.h

        ${zlib-units}
)

set(tests
        src/tests/doctestmain.cpp

        src/tests/gdxtests.h
        src/tests/gdxtests.cpp
        src/tests/gxfiletests.cpp

        src/tests/gdlib/datastoragetests.cpp
        src/tests/gdlib/glookuptests.cpp
        src/tests/gdlib/gmacrotests.cpp
        src/tests/gdlib/gmsdatatests.cpp
        src/tests/gdlib/gmsheapnewtests.cpp
        src/tests/gdlib/gmsobjtests.cpp
        src/tests/gdlib/gmsstrmtests.cpp
        src/tests/gdlib/obfuscatestrtests.cpp
        src/tests/gdlib/strhashtests.cpp
        src/tests/gdlib/strutilxtests.cpp
        src/tests/gdlib/utilstests.cpp
        src/tests/gdlib/xcompresstests.cpp

        src/tests/rtl/p3iotests.cpp
        src/tests/rtl/p3utilstests.cpp
        src/tests/rtl/stdthreadtests.cpp
        src/tests/rtl/sysutilsp3tests.cpp
)

set(inc-dirs zlib src generated)

include(targets.cmake)
