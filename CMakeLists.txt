cmake_minimum_required(VERSION 3.0.2)

project(RayTracing_OpenGL)

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
set(ASSETS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    SET(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    SET_PROPERTY(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
        "MinSizeRel" "RelWithDebInfo")
endif()

add_definitions("-DASSETS_DIR=\"${ASSETS_DIR}\"")

if(UNIX)
    add_definitions(-Wall -fvisibility=hidden)
endif()

# OS Definitions
if(UNIX)
    if(APPLE)
        add_definitions(-DOS_MAC)
        set(CMAKE_MACOSX_RPATH ON)
        set(CMAKE_SKIP_BUILD_RPATH  FALSE)
        set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}")
        set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

        list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}" isSystemDir)
        if("${isSystemDir}" STREQUAL "-1")
            set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}")
        endif("${isSystemDir}" STREQUAL "-1")
    else(APPLE)
        add_definitions(-DOS_LNX)
    endif(APPLE)
else(UNIX)
    add_definitions(-DOS_WIN -DNOMINMAX)
endif(UNIX)

set(X11_LIBS "")
if(APPLE)
    find_package(X11 REQUIRED)
    include_directories(${X11_INCLUDE_DIR})
    set(X11_LIBS ${X11_LIBRARIES})
endif(APPLE)

find_package(OpenGL REQUIRED)
find_package(GLFW REQUIRED)

add_subdirectory(external_sources/glad)

include_directories(
    ${OPENGL_INCLUDE_DIR}
    ${GLFW_INCLUDE_DIR}
    "${CMAKE_SOURCE_DIR}/common"
    "${CMAKE_SOURCE_DIR}/external_sources/glm"
    "${CMAKE_SOURCE_DIR}/external_sources/stb_image"
)

file(GLOB src
    common/*.cpp
    src/*.cpp
    src/*.h
    external_sources/stb_image/*.cpp
)

add_executable("rt"
    ${src}
)

target_link_libraries("rt"
    PRIVATE ${OPENGL_LIBRARIES}
    PRIVATE ${GLFW_LIBRARY}
    PRIVATE ${X11_LIBS}
    PRIVATE ${CMAKE_DL_LIBS}
    PRIVATE glad-interface
)

set_target_properties("rt"
    PROPERTIES
    OUTPUT_NAME "rt"
    RUNTIME_OUTPUT_DIRECTORY "rt"
    FOLDER "src")
