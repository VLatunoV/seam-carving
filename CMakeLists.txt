cmake_minimum_required(VERSION 3.0...3.29)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Setup GFLW
set(GLFW_BUILD_DOCS CACHE BOOL OFF)
set(GLFW_BUILD_EXAMPLES CACHE BOOL OFF)
set(GLFW_BUILD_TESTS CACHE BOOL OFF)
set(GLFW_INSTALL CACHE BOOL OFF)
add_subdirectory(glfw)

# Setup IMGUI
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
file(GLOB IMGUI_SOURCES
	${IMGUI_DIR}/*.cpp
	${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
	${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
	${IMGUI_DIR}
	${IMGUI_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)

# Setup project
project(seam-carving)
set(CMAKE_CONFIGURATION_TYPES Debug Release)

find_package(OpenGL REQUIRED)
if (NOT DEFINED OPENGL_FOUND OR NOT ${OPENGL_FOUND})
	message(FATAL_ERROR "Could not find OpenGL on the system!")
endif()

file(GLOB SOURCES
	${CMAKE_SOURCE_DIR}/src/*.cpp
	${CMAKE_SOURCE_DIR}/src/*.h
)
add_executable(seam ${SOURCES})
target_include_directories(seam PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(seam PUBLIC
	imgui
	OpenGL::GL
)
