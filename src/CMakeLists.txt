add_subdirectory(imgui)

SET(SRCS main.cpp application.cpp shaderloader.h)

add_executable(renderer-app ${SRCS})
target_include_directories(renderer-app PUBLIC imgui)
target_link_libraries(renderer-app glfw Vulkan::Vulkan imgui-renderer)
