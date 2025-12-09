# Vulkan Grapher

A work in progress 3D function grapher built with C++ + Vulkan + Dear ImGui.

We have previously written a version of the function grapher using C++ + OpenGL in [opengl_examples](https://github.com/seansovine/opengl_examples),
and a more full-featured version using Rust + wgpu + egui in [wgpu_grapher](https://github.com/seansovine/wgpu_grapher).

<p align="center" margin="20px">
	<img src="https://raw.githubusercontent.com/seansovine/page_images/refs/heads/main/screenshots/vulkan_grapher/radial_sinc_blue_2025-12-08.png" alt="drawing" width="700" style="padding-top: 10px; padding-bottom: 10px"/>
</p>

_Graph of radial sinc function._

You can see in the example image that the graph is much rougher in parts where the function --
either its values or its derivatives -- are changing rapidly. We will implement a form of local mesh
refinement to try to add enough detail in these parts of the function without doing unnecessary work
in other areas of the graph.

## What we have now

We have code that renders a function -- which currently must be defined in the code --
using a basic mesh comprised of a square tessellation divided into triangles.

The UI has a feature to toggle rotation of the graph around the y-axis (which is the "up"
axis in most computer graphics APIs).

## To do next

Next we will add these graphing features:

1. Either find or write a mathematical expression parsing library in C/C++.

2. Add more sophisticated rendering features, starting with lighting.

3. Add better user interaction features like mouse control of the view.

4. Add function-based mesh refinement.

## Sources and credits

We are following the tutorial at [vulkan-tutorial.com](https://vulkan-tutorial.com/) and studying
up in the _Vulkan Programming Guide_ by Graham Sellars. The example at [vk-sandbox](https://github.com/tstullich/vk-sandbox)
was helpful for figuring out the Dear ImGui integration. We are also looking at Sascha Willems'
Vulkan-glTF-PBR model viewer for inspiration, which is available [here](https://github.com/SaschaWillems/Vulkan-glTF-PBR) on GitHub.
The official Vulkan docs from Khronos are also pretty good.
