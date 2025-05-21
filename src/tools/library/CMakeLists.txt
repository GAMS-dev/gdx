add_library(gdxtools-library
    ${CMAKE_SOURCE_DIR}/generated/gdxcc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/short_string.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cmdpar.cpp
    ${base-units-all}
)
target_include_directories(gdxtools-library PRIVATE ${inc-dirs})
target_link_libraries(gdxtools-library gdx-static)
if (UNIX)
    target_link_libraries(gdxtools-library ${CMAKE_DL_LIBS})
endif ()
