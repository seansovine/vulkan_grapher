add_subdirectory(imgui)

SET(HEADERS
	application.h
	shaderloader.h
	imgui_vulkan_data.h
	uniforms.h
	vertex.h
	vulkan_debug.h
	vulkan_helper.h
	vulkan_wrapper.h
)

SET(SRCS
	main.cpp
	application.cpp
	imgui_vulkan_data.cpp
	vulkan_wrapper.cpp
)

add_executable(renderer-app ${HEADERS} ${SRCS})
target_include_directories(renderer-app PUBLIC imgui)
target_link_libraries(renderer-app
	glfw
	Vulkan::Vulkan
	imgui-renderer
	glm::glm
)
