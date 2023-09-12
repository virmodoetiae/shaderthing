cmake_minimum_required(VERSION 3.16)

project(thirdparty)

set(CMAKE_BUILD_TYPE_0 "${CMAKE_BUILD_TYPE}")
if (CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_BUILD_TYPE "Release")
endif()

#add_subdirectory(freetype REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw3)

#find_package(OpenCV REQUIRED PATHS opencv/build)
#include_directories(${OpenCV_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/glad/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/glfw3/include)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/opencv/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/glm)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/imgui)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/imguifiledialog)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/imguitexteditor)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/stb)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/freetype/include)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/freetype/include/freetype)
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/giflib)

#add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#add_compile_definitions(IMGUI_DEFINE_MATH_OPERATORS)
#add_compile_definitions(IMGUI_ENABLE_FREETYPE)
set(CXX_FLAGS "-std=c++17 -O3")
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
			#imgui/misc/freetype/imgui_freetype.cpp
			imguifiledialog/ImGuiFileDialog.cpp 
			imguitexteditor/imguitexteditor.cpp
			stb.cpp)

#target_link_libraries(thirdparty PUBLIC freetype)
target_link_libraries(thirdparty PUBLIC glfw) #${OpenCV_LIBS})

set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE_0}")