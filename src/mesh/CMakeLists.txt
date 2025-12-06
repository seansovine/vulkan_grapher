file(GLOB HEADERS "*.h" "*.hpp")
file(GLOB SOURCES "*.cpp")

add_library(mesh ${HEADERS} ${SOURCES})
set_target_properties(mesh PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(mesh PUBLIC vulkan-util)
target_include_directories(mesh PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
