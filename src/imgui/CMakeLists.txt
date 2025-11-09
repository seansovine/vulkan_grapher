set(IMGUI_SRCS
    imgui.cpp
    imgui_draw.cpp
    imgui_impl_glfw.cpp
    imgui_impl_vulkan.cpp
    imgui_widgets.cpp
)
add_library(imgui-renderer ${IMGUI_SRCS})
target_include_directories(imgui-renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
