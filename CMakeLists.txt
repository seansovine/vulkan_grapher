cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

project(renderer)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(glm REQUIRED)

# Make Gmsh build shared library.
set(ENABLE_BUILD_SHARED ON CACHE BOOL "Force Gmsh shared library." FORCE)
set(ENABLE_BUILD_DYNAMIC ON CACHE BOOL "Force Gmsh shared library." FORCE)
add_subdirectory(thirdparty/gmsh)

# Gmsh's shared librar target is awkwardly named.
set(GMSH_SHARED shared)
if(NOT TARGET ${GMSH_SHARED})
    message(FATAL_ERROR "GMSH_SHARED shared library was not found.")
endif()

# Include CMake's FetchContent module
include(FetchContent)

# Declare iceoryx2 as an external dependency
FetchContent_Declare(
    iceoryx2
    GIT_REPOSITORY https://github.com/eclipse-iceoryx/iceoryx2
    GIT_TAG v0.9.0 # Replace with your desired release/tag
)

# Populate the content so CMake can find the source files
FetchContent_MakeAvailable(iceoryx2)

add_subdirectory(thirdparty/spdlog)
add_subdirectory(thirdparty/mathpresso)
add_subdirectory(src)
