# Dynamic library / shared object
add_library(gdxcclib64 SHARED ${gdx-core} generated/gdxcclib.cpp)
if (UNIX)
    target_compile_options(gdxcclib64 PRIVATE -fvisibility=hidden)
endif ()
target_include_directories(gdxcclib64 PRIVATE ${inc-dirs})
if (APPLE)
    set(cclib-link-options "-Bdynamic")
elseif (UNIX) # Linux
    set(cclib-link-options "-Bdynamic -Wl,-Bsymbolic")
else () # Windows
    set(cclib-link-options "")
endif ()
target_link_libraries(gdxcclib64 ${mylibs} ${cclib-link-options})
set_property(TARGET gdxcclib64 PROPERTY POSITION_INDEPENDENT_CODE ON)

# Static library
add_library(gdx-static STATIC ${gdx-core})
target_include_directories(gdx-static PRIVATE ${inc-dirs})
set_property(TARGET gdx-static PROPERTY POSITION_INDEPENDENT_CODE ON)

set(NO_TESTS OFF CACHE BOOL "Skip building unit tests")
if(NOT NO_TESTS)
# Unit test suite (against statically compiled GDX)
add_executable(gdxtest ${test-deps} ${tests})
target_include_directories(gdxtest PRIVATE ${inc-dirs})
target_link_libraries(gdxtest gdx-static ${mylibs})

# Unit test suite (against GDX dynamic library)
add_executable(gdxwraptest src/tests/doctestmain.cpp src/tests/gdxtests.cpp generated/gdxcc.c)
target_link_libraries(gdxwraptest ${mylibs})
target_include_directories(gdxwraptest PRIVATE src generated src/gdlib)
target_compile_options(gdxwraptest PRIVATE -DGXFILE_CPPWRAP -DGC_NO_MUTEX)
endif()

# Quickly run "include what you use" (https://include-what-you-use.org/) over project
#[[find_program(iwyu_path NAMES include-what-you-use iwyu REQUIRED)
set_property(TARGET gdxcclib64 PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
set_property(TARGET gdxtest PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})]]

set(NO_EXAMPLES OFF CACHE BOOL "Skip building example programs/executables")
if(NOT NO_EXAMPLES)
# Standalone GDX example program 1
add_executable(xp_example1 ${gdx-core} src/examples/xp_example1.cpp)
target_include_directories(xp_example1 PRIVATE ${inc-dirs})
target_link_libraries(xp_example1 ${mylibs})

# Standalone GDX example program 2
add_executable(xp_example2 ${gdx-core} src/examples/xp_example2.cpp generated/optcc.c)
target_include_directories(xp_example2 PRIVATE ${inc-dirs})
target_link_libraries(xp_example2 ${mylibs})

# Standalone GDX example program 3 (associative "supply.demand" str -> double (level))
add_executable(xp_associative ${gdx-core} src/examples/xp_associative.cpp)
target_include_directories(xp_associative PRIVATE ${inc-dirs})
target_link_libraries(xp_associative ${mylibs})

# Standalone GDX example program 4 (associative {supply,demand} vector -> double (level))
add_executable(xp_associative_vec ${gdx-core} src/examples/xp_associative_vec.cpp)
target_include_directories(xp_associative_vec PRIVATE ${inc-dirs})
target_link_libraries(xp_associative_vec ${mylibs})

# Standalone GDX example program 5 (data write)
# Needs path to GAMS distribution in environment variable GAMSDIR
# (either directly or as first entry of semicolon separated list)
if (DEFINED ENV{GAMSDIR})
    set(ENTRIES $ENV{GAMSDIR})
    string(FIND "${ENTRIES}" ";" index)
    if (index GREATER -1) # contains multiple semicolon separated entries
        string(REPLACE ";" "\n" ENTRIES "${ENTRIES}")
        string(REGEX MATCH "^([^\n]*)" FIRST_ENTRY "${ENTRIES}")
        string(REPLACE "\\" "/" FIRST_ENTRY "${FIRST_ENTRY}")
        set(cur-apifiles-path ${FIRST_ENTRY})
    else () # just one entry
        set(cur-apifiles-path ${ENTRIES})
    endif ()
    string(APPEND cur-apifiles-path "/apifiles/C/api/")
    if (EXISTS ${cur-apifiles-path}gmdcc.c)
        message(STATUS "Found expert-level API sources and headers in ${cur-apifiles-path}")
        add_executable(xp_dataWrite ${gdx-core} src/examples/xp_dataWrite.cpp ${cur-apifiles-path}gmdcc.c)
        target_include_directories(xp_dataWrite PRIVATE ${inc-dirs} ${cur-apifiles-path})
        target_link_libraries(xp_dataWrite ${mylibs})
        if (UNIX)
            target_compile_options(xp_dataWrite PRIVATE -DGC_NO_MUTEX)
        endif ()
    endif ()
endif ()

endif()

set(NO_TOOLS OFF CACHE BOOL "Skip building GDX tools")
if(NOT NO_TOOLS)

# Library for gdxdump, gdxdiff and gdxmerge
add_library(gdxtools-library
    generated/gdxcc.h
    generated/gdxcc.c

    src/gdlib/dblutil.hpp
    src/gdlib/dblutil.cpp
    src/gdlib/gmsobj.hpp
    src/gdlib/gmsobj.cpp

    src/tools/library/common.hpp
    src/tools/library/common.cpp
    src/tools/library/short_string.hpp
    src/tools/library/short_string.cpp
    src/tools/library/cmdpar.hpp
    src/tools/library/cmdpar.cpp
)
target_include_directories(gdxtools-library PRIVATE ${inc-dirs})
target_link_libraries(gdxtools-library gdx-static)
if (UNIX)
    target_link_libraries(gdxtools-library ${CMAKE_DL_LIBS})
endif ()

# gdxdump
add_executable(gdxdump
        src/tools/gdxdump/gdxdump.hpp
    src/tools/gdxdump/gdxdump.cpp
)
target_include_directories(gdxdump PRIVATE ${inc-dirs})
target_link_libraries(gdxdump gdxtools-library)

# gdxdiff
add_executable(gdxdiff
    src/tools/gdxdiff/gdxdiff.hpp
    src/tools/gdxdiff/gdxdiff.cpp
    src/rtl/p3process.cpp
)
target_include_directories(gdxdiff PRIVATE ${inc-dirs})
target_link_libraries(gdxdiff gdxtools-library)

# gdxmerge
add_executable(gdxmerge
    src/tools/gdxmerge/gdxmerge.hpp
    src/tools/gdxmerge/gdxmerge.cpp
)
target_include_directories(gdxmerge PRIVATE ${inc-dirs})
target_link_libraries(gdxmerge gdxtools-library)

endif(NOT NO_TOOLS)
