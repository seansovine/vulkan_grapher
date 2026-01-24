file(GLOB HEADERS "*.h" "*.hpp")
file(GLOB SOURCES "*.cpp")

add_library(app ${HEADERS} ${SOURCES})
set_target_properties(app PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(app PUBLIC imgui-renderer)
