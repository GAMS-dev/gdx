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

# Check compilation of unused infrastructure units
add_library(base-units STATIC ${base-units})
target_include_directories(base-units PRIVATE ${inc-dirs})

# Static library
add_library(gdx-static STATIC ${gdx-core})
target_include_directories(gdx-static PRIVATE ${inc-dirs})
set_property(TARGET gdx-static PROPERTY POSITION_INDEPENDENT_CODE ON)

# Unit test suite (against statically compiled GDX)
add_executable(gdxtest ${base-units} ${tests})
target_include_directories(gdxtest PRIVATE ${inc-dirs})
target_link_libraries(gdxtest gdx-static ${mylibs})

# Unit test suite (against GDX dynamic library)
add_executable(gdxwraptest src/tests/doctestmain.cpp src/tests/gdxtests.cpp generated/gdxcc.c)
target_link_libraries(gdxwraptest ${mylibs})
target_include_directories(gdxwraptest PRIVATE src generated)
target_compile_options(gdxwraptest PRIVATE -DGXFILE_CPPWRAP -DGC_NO_MUTEX)

# Quickly run "include what you use" (https://include-what-you-use.org/) over project
#[[find_program(iwyu_path NAMES include-what-you-use iwyu REQUIRED)
set_property(TARGET gdxcclib64 PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
set_property(TARGET gdxtest PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})]]

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

# Library for gdxdump, gdxdiff and gdxmerge
add_library(gdxtools_library
	src/tools/library/short_string.h
	src/tools/library/short_string.cpp
	src/tools/library/library.h
	src/tools/library/library.cpp
	src/tools/library/cmdpar.h
	src/tools/library/cmdpar.cpp
)

# gdxdump
add_executable(gdxdump ${common}
	src/tools/gdxdump/gdxdump.h
	src/tools/gdxdump/gdxdump.cpp
	src/tools/gdxdump/main.cpp
)
target_include_directories(gdxdump PRIVATE ${inc-dirs})
target_link_libraries(gdxdump ${mylibs} gams-base gdxtools_library)

# gdxdiff
add_executable(gdxdiff ${common}
	src/tools/gdxdiff/gdxdiff.h
	src/tools/gdxdiff/gdxdiff.cpp
	src/tools/gdxdiff/main.cpp
)
target_include_directories(gdxdiff PRIVATE ${inc-dirs})
target_link_libraries(gdxdiff ${mylibs} gams-base gdxtools_library)

# gdxmerge
add_executable(gdxmerge ${common}
	src/tools/gdxmerge/gdxmerge.h
	src/tools/gdxmerge/gdxmerge.cpp
	src/tools/gdxmerge/main.cpp
)
target_include_directories(gdxmerge PRIVATE ${inc-dirs})
target_link_libraries(gdxmerge ${mylibs} gams-base gdxtools_library)
