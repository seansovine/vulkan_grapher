add_subdirectory(imgui)

SET(SRCS main.cpp application.h application.cpp shaderloader.h)

add_executable(renderer-app ${SRCS})
target_include_directories(renderer-app PUBLIC imgui)
target_link_libraries(renderer-app
	glfw
	Vulkan::Vulkan
	imgui-renderer
	glm::glm
)
