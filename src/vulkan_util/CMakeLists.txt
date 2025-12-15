file(GLOB HEADERS "*.h" "*.hpp")
file(GLOB SOURCES "*.cpp")

add_library(vulkan-util ${HEADERS} ${SOURCES})
set_target_properties(vulkan-util PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(vulkan-util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
