cmake_minimum_required(VERSION 3.16)

project(thirdparty)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(CMAKE_BUILD_TYPE_0 "${CMAKE_BUILD_TYPE}")
if (CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# GLFW Setup
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
if(UNIX AND NOT APPLE)
    set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
endif()
add_subdirectory(glfw3)

# GLSLang setup
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)  # No standalone binaries
set(ENABLE_SPIRV ON CACHE BOOL "" FORCE)              # Enable SPIR-V
set(ENABLE_SPVREMAPPER OFF CACHE BOOL "" FORCE)       # No SPIR-V Remapper
set(ENABLE_OPT OFF CACHE BOOL "" FORCE)               # No SPIRV-Tools optimizations
set(ENABLE_HLSL OFF CACHE BOOL "" FORCE)              # No HLSL support
set(ENABLE_GLSLANG_JS OFF CACHE BOOL "" FORCE)        # No JavaScript bindings
add_subdirectory(glslang)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/glad/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/glfw3/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/glm)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/imgui)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/imguitexteditor)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/stb)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/rapidjson)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/portable-file-dialogs)

set(CXX_FLAGS "-static-libgcc -static-libstdc++ -static -std=c++17")
if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CXX_FLAGS "${CXX_FLAGS} -O0")
else()
    set(CXX_FLAGS "${CXX_FLAGS} -O3")
endif()
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CXX_FLAGS}")
add_library(thirdparty 
			glad/src/glad.cpp 
			imgui/backends/imgui_impl_opengl3.cpp 
			imgui/backends/imgui_impl_glfw.cpp 
			imgui/imgui.cpp imgui/imgui_draw.cpp 
			imgui/imgui_tables.cpp 
			imgui/imgui_widgets.cpp 
			imgui/imgui_demo.cpp 
			imgui/misc/cpp/imgui_stdlib.cpp
			imgui/imgui_extensions.cpp
			imgui/imgui_internal_extensions.cpp
			stb/stb.cpp
			portable-file-dialogs/pfd_impl.cpp)

#target_link_libraries(thirdparty PUBLIC glfw shaderc shaderc_util SPIRV-Tools SPIRV-Tools-opt)

target_link_libraries(thirdparty PUBLIC glfw glslang)

set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE_0}")
