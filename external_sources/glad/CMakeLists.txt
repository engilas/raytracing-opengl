cmake_minimum_required(VERSION 3.0.2)

add_library(glad-interface INTERFACE)

target_sources(glad-interface
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/glad.c>
    )

target_include_directories(glad-interface
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )
