cmake_minimum_required(VERSION 3.16)

project(vir)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/imgui)

file(GLOB_RECURSE VIR_SOURCE_FILES "src/*.cpp")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty thirdparty)

find_package(OpenGL REQUIRED)

set(CXX_FLAGS "-fno-delete-null-pointer-checks -static-libgcc -static-libstdc++ -static -std=c++17 -Wall -Wextra")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

add_library(vir ${VIR_SOURCE_FILES})

target_precompile_headers(vir PUBLIC include/vpch.h)
target_link_libraries(vir PUBLIC thirdparty PUBLIC ${OPENGL_LIBRARIES})
if(UNIX AND NOT APPLE)
	find_package(X11 REQUIRED)
	target_link_libraries(vir PUBLIC ${X11_LIBRARIES} Xtst)
	target_include_directories(vir PUBLIC ${X11_INCLUDE_DIR})
endif()
