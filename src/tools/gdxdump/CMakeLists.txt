add_executable(gdxdump
    ${CMAKE_SOURCE_DIR}/src/gdlib/dblutil.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gdxdump.cpp
)

target_link_libraries(gdxdump PRIVATE
    gdxtools-library
)
