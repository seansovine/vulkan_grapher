file(GLOB HEADERS "*.h" "*.hpp")
file(GLOB SOURCES "*.cpp")

add_library(mesh ${HEADERS} ${SOURCES})
set_target_properties(mesh PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(mesh PUBLIC vulkan-util app spdlog::spdlog mathpresso)
target_include_directories(mesh PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(BUILD_GMSH)
    if(NOT TARGET ${GMSH_SHARED})
        message(FATAL_ERROR "GMSH_SHARED shared library was not found.")
    endif()
    target_link_libraries(mesh PRIVATE ${GMSH_SHARED})
endif()
