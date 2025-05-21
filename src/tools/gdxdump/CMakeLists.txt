add_executable(gdxdump
    ${CMAKE_CURRENT_SOURCE_DIR}/gdxdump.cpp
)
target_include_directories(gdxdump PRIVATE ${inc-dirs})
target_link_libraries(gdxdump gdxtools-library)
