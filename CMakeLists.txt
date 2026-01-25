cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

project(renderer)

find_package(Vulkan REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package (glm REQUIRED)

add_subdirectory(thirdparty/spdlog)
add_subdirectory(thirdparty/mathpresso)
add_subdirectory(src)
