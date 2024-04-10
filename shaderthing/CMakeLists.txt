cmake_minimum_required(VERSION 3.16)

project(shaderthing)



include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../vir/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/imgui)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../vir vir)

if(WIN32)
    set(CXX_FLAGS "-fno-delete-null-pointer-checks -static-libgcc -static-libstdc++ -static -std=c++17 -Wall -Wextra -Wno-unused-parameter")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/data)
    list(APPEND SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/data/shaderthing_win32.rc)
else()
    set(CXX_FLAGS "-fno-delete-null-pointer-checks -static-libgcc -static-libstdc++ -std=c++17 -Wall -Wextra -Wno-unused-parameter")
endif()
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

add_executable(shaderthing ${SOURCE_FILES})

target_precompile_headers(shaderthing REUSE_FROM vir)
target_link_libraries(shaderthing PUBLIC vir)
