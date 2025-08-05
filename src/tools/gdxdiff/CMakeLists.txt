add_executable(gdxdiff
    ${CMAKE_SOURCE_DIR}/src/rtl/p3process.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gdxdiff.cpp
)

target_link_libraries(gdxdiff PRIVATE
    gdxtools-library
)
