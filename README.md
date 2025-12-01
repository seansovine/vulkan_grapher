# Vulkan Grapher

Work in progress to build a Vulkan + Dear ImGui version of our 3D function grapher.

We have previously written a version of the function grapher using C++ + OpenGL in [opengl_examples](https://github.com/seansovine/opengl_examples),
and a more full-featured version using Rust + wgpu + egui in [wgpu_grapher](https://github.com/seansovine/wgpu_grapher).

## What we have now

We have a very bare bones Vulkan renderer that is integrated with a very
basic Dear ImGui user interface overlay. Right now it only shows a rotating triangle
and the UI features a button that lets you toggle the color of one of the vertices.

We have a basic code structure that is starting to take shape.

## To do next

There are some basic rendering features we will include:

1. Staging buffers for more efficient vertex memory use.
2. Multi-sampling.
3. Depth buffering.

We will add these things next.

Then we will start adding the main graphing features:

1. Add code to build the mesh for a function graph and then render it.

2. Either find or write a mathematical expression parsing library in C/C++.

3. Add more sophisticated rendering features, starting with lighting.

4. Add better user interaction features like mouse control of the view.

## Sources and credits

We are following the tutorial at [vulkan-tutorial.com](https://vulkan-tutorial.com/) and studying
up in the _Vulkan Programming Guide_ by Graham Sellars. The example at [vk-sandbox](https://github.com/tstullich/vk-sandbox)
was helpful for figuring out the Dear ImGui integration. We are also looking at Sascha Willems'
Vulkan-glTF-PBR model viewer for inspiration, which is available [here](https://github.com/SaschaWillems/Vulkan-glTF-PBR) on GitHub.
The official Vulkan docs from Khronos are also pretty good.
