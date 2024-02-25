cmake_minimum_required(VERSION 3.16)

# This is a simple CMakeWrapper around the existing Makefiles provided
# for each platform - virmodoetiae

project(nativefiledialogproj)

include(ExternalProject)

# Set the correct makefile to be usedbased on the operating system
if (WIN32)
    set(MAKEFILE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/gmake_windows)
elseif (UNIX)
    set(MAKEFILE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/gmake_linux)
elseif (APPLE)
    set(MAKEFILE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build/gmake_macosx)
else()
    message(FATAL_ERROR "Unsupported operating system")
endif()

ExternalProject_Add(
    nativefiledialogproj
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    BUILD_COMMAND ${CMAKE_MAKE_PROGRAM} nfd -C ${MAKEFILE_DIR} configure=release_x64
    INSTALL_COMMAND ""
    BUILD_ALWAYS TRUE
)

add_library(nativefiledialog STATIC IMPORTED GLOBAL)
add_dependencies(nativefiledialog nativefiledialogproj)
set_target_properties(nativefiledialog PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/build/lib/Release/x64/nfd.lib
)
target_include_directories(nativefiledialog INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)