if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
   return()
endif()

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

SET(CODECOV_PROFILES "Codecov" CACHE STRING "All codecov profiles" FORCE)

if(isMultiConfig)
  foreach(profile in CODECOV_PROFILES)
    if(NOT ${profile} IN_LIST CMAKE_CONFIGURATION_TYPES)
      list(APPEND CMAKE_CONFIGURATION_TYPES ${profile})
    endif()
    endforeach()
else()
   set_property(CACHE CMAKE_BUILD_TYPE APPEND PROPERTY STRINGS ${CODECOV_PROFILES})
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
   set(CODECOV_COMPILE_FLAGS "-fprofile-instr-generate -fcoverage-mapping -O0 -ggdb3 -fno-omit-frame-pointer")
   set(CODECOV_SHARED_FLAGS  "-fprofile-instr-generate")
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU")
   set(CODECOV_COMPILE_FLAGS "--coverage -O0 -ggdb3 -fno-omit-frame-pointer")
   set(CODECOV_SHARED_FLAGS  "--coverage")
endif()


set(CMAKE_C_FLAGS_CODECOV
  "${CMAKE_C_FLAGS_DEBUG} ${CODECOV_COMPILE_FLAGS}" CACHE STRING
  "Flags used by the C compiler for Asan build type or configuration." FORCE)

set(CMAKE_CXX_FLAGS_CODECOV
  "${CMAKE_CXX_FLAGS_DEBUG} ${CODECOV_COMPILE_FLAGS}" CACHE STRING
  "Flags used by the C++ compiler for Asan build type or configuration." FORCE)

set(CMAKE_EXE_LINKER_FLAGS_CODECOV
  "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} ${CODECOV_LINKER_FLAGS}" CACHE STRING
  "Linker flags to be used to create executables for Asan build type." FORCE)

set(CMAKE_SHARED_LINKER_FLAGS_CODECOV
  "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} ${CODECOV_LINKER_FLAGS}" CACHE STRING
  "Linker lags to be used to create shared libraries for Asan build type." FORCE)

