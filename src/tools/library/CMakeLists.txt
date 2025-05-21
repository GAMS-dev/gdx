add_library(gdxtools-library
    ${CMAKE_SOURCE_DIR}/generated/gdxcc.c
    ${CMAKE_SOURCE_DIR}/src/gdlib/dblutil.cpp
    ${CMAKE_SOURCE_DIR}/src/gdlib/gmsobj.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/short_string.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdpar.cpp
)
target_include_directories(gdxtools-library PRIVATE ${inc-dirs})
target_link_libraries(gdxtools-library gdx-static)
if (UNIX)
    target_link_libraries(gdxtools-library ${CMAKE_DL_LIBS})
endif ()
