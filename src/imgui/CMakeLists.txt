# Build a library from the subset of ImGui that we use.

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/thirdparty/imgui)
set(IMGUI_SRCS
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)
add_library(imgui-renderer ${IMGUI_SRCS})

# Include thirdparty dir so we can use includes relative to imgui for clarity.
target_include_directories(imgui-renderer PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/..)
