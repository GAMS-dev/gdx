add_executable(gdxmerge
    ${CMAKE_CURRENT_SOURCE_DIR}/gdxmerge.cpp
)

target_link_libraries(gdxmerge PRIVATE
    gdxtools-library
)
