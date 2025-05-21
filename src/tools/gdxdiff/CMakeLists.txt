add_executable(gdxdiff
    ${CMAKE_CURRENT_SOURCE_DIR}/gdxdiff.cpp
)
target_include_directories(gdxdiff PRIVATE ${inc-dirs})
target_link_libraries(gdxdiff gdxtools-library)
