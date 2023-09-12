cmake_minimum_required(VERSION 3.16)

project(shaderthing)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/imgui)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/include/opencv2)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../vir/include)

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../vir vir)

set(CXX_FLAGS "-std=c++17 -O0")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CXX_FLAGS}")

if(WIN32)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/resources)
    list(APPEND SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/resources/shaderthing_win32.rc)
endif()

add_executable(shaderthing ${SOURCE_FILES})

target_precompile_headers(shaderthing REUSE_FROM vir)
target_link_libraries(shaderthing PUBLIC vir)
