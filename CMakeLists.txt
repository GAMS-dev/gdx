cmake_minimum_required(VERSION 3.16)
project(gdx)
set(CMAKE_CXX_STANDARD 17)

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif (CCACHE_FOUND)

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
    else()
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
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wreturn-type -Wmissing-declarations -Wno-unknown-pragmas -pedantic -Wextra -Wno-unused-parameter -Wno-deprecated-copy -Wno-cast-function-type")
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wconversion -funsigned-char") # aggressive signage warnings
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DZ_HAVE_UNISTD_H")
    if (NOT APPLE AND NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat-truncation=0")
    endif ()
    # strictly handle alignment and structure padding around flexible arrays (only newer compilers)
    if ((CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16) OR
    (CMAKE_CXX_COMPILER_ID MATCHES "GCC" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13))
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstrict-flex-arrays=3")
    endif()
    # ADD common hardening features on Linux
    if (CMAKE_SYSTEM_NAME MATCHES "Linux")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 ")
        add_link_options(LINKER:-z,defs LINKER:-z,now LINKER:-z,relro LINKER:-z,noexecstack)
    endif()
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
    # P3 runtime library
    src/rtl/dtoaLoc.h
    src/rtl/dtoaLoc.c
    src/rtl/p3io.h
    src/rtl/p3io.cpp
    src/rtl/p3utils.h
    src/rtl/p3utils.cpp
    src/rtl/sysutils_p3.h
    src/rtl/sysutils_p3.cpp
    src/rtl/p3platform.h
    src/rtl/p3platform.cpp
    src/rtl/math_p3.h
    src/rtl/math_p3.cpp

    # gdlib essential units
    src/gdlib/gmsstrm.cpp
    src/gdlib/gmsstrm.h
    src/gdlib/utils.h
    src/gdlib/utils.cpp
    src/gdlib/strutilx.h
    src/gdlib/strutilx.cpp
    src/gdlib/strindexbuf.h

    # GDX specific units
    src/gxfile.h
    src/gxfile.cpp
    src/gdx.h

    ${zlib-units}
)

set(test-deps
    # runtime library
    src/rtl/p3library.cpp
    src/rtl/p3library.h
    src/rtl/stdthread.cpp
    src/rtl/stdthread.h

    # global
    src/global/gmslibname.cpp
    src/global/gmslibname.h

    # gdlib
    src/gdlib/charmaps.cpp
    src/gdlib/charmaps.h
    src/gdlib/gmacro.cpp
    src/gdlib/gmacro.h
    src/gdlib/gmsheapnew.cpp
    src/gdlib/gmsheapnew.h
    src/gdlib/gmsonly.h
    src/gdlib/gmsonly.cpp
    src/gdlib/obfuscatestr.cpp
    src/gdlib/obfuscatestr.h
    src/gdlib/strhash.h
    src/gdlib/strhash.cpp
    src/gdlib/xcompress.cpp
    src/gdlib/xcompress.h
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
