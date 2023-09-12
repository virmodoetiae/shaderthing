cmake_minimum_required(VERSION 3.16)

project(vir)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/imgui)

file(GLOB_RECURSE VIR_SOURCE_FILES "src/*.cpp")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty thirdparty)

find_package(OpenGL REQUIRED)

set(CXX_FLAGS "-std=c++17 -O0")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CXX_FLAGS}")

add_library(vir ${VIR_SOURCE_FILES})

target_precompile_headers(vir PUBLIC include/vpch.h)
target_link_libraries(vir PUBLIC thirdparty PUBLIC ${OPENGL_LIBRARIES})
