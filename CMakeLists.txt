cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Setup project
project(seam-carving)
set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE STRING "" FORCE)

# OpenGL
find_package(OpenGL REQUIRED)
if (NOT DEFINED OPENGL_FOUND OR NOT ${OPENGL_FOUND})
	message(FATAL_ERROR "Could not find OpenGL on the system!")
endif()

# Setup GFLW
set(GLFW_BUILD_DOCS CACHE BOOL OFF)
set(GLFW_BUILD_EXAMPLES CACHE BOOL OFF)
set(GLFW_BUILD_TESTS CACHE BOOL OFF)
set(GLFW_INSTALL CACHE BOOL OFF)
add_subdirectory(glfw)

# Setup IMGUI
set(IMGUI_DIR imgui)
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

# Setup FreeImage image loader
if ("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
	set(ARCH "x32")
else()
	set(ARCH "x64")
endif()
add_library(free_image SHARED IMPORTED)
set(FREE_IMAGE_FILE_DIR "${CMAKE_SOURCE_DIR}/FreeImage/bin/${ARCH}")
set_target_properties(free_image PROPERTIES
	IMPORTED_IMPLIB ${FREE_IMAGE_FILE_DIR}/FreeImage.lib
	IMPORTED_LOCATION ${FREE_IMAGE_FILE_DIR}/FreeImage.dll
)

# Application sources
file(GLOB SOURCES
	src/*.cpp
	src/*.h
)
add_executable(seam ${SOURCES})
target_include_directories(seam PUBLIC
	src
	FreeImage/include
)
target_link_libraries(seam PUBLIC
	imgui
	OpenGL::GL
	free_image
)
add_custom_command(TARGET seam POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"$<TARGET_RUNTIME_DLLS:seam>" # Files
	"$<TARGET_FILE_DIR:seam>" # Directory
	COMMENT "Copying dlls to build directory..."
	VERBATIM
)


# Set custom default path for installation
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "" FORCE)
endif()

install(TARGETS seam
	DESTINATION $<CONFIG>/bin
)
install(FILES "$<TARGET_RUNTIME_DLLS:seam>"
	DESTINATION $<CONFIG>/bin
)
