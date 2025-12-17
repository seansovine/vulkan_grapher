# Vulkan Grapher

A work in progress 3D function grapher built with C++ + Vulkan + Dear ImGui.

We have previously written a version of the function grapher using C++ + OpenGL in [opengl_examples](https://github.com/seansovine/opengl_examples),
and a more full-featured version using Rust + wgpu + egui in [wgpu_grapher](https://github.com/seansovine/wgpu_grapher).

<p align="center" margin="20px">
	<img src="https://raw.githubusercontent.com/seansovine/page_images/refs/heads/main/screenshots/vulkan_grapher/radial_sinc_blue_2025-12-08.png" alt="drawing" width="700" style="padding-top: 10px; padding-bottom: 10px"/>
</p>

_Graph of radial sinc function._

You can see in the example image that the graph is much rougher in parts where the function --
either its values or its derivatives -- are changing rapidly. To address this we have a form of local
mesh refinement to try to add enough detail in these parts of the function without doing unnecessary
work in other areas of the graph.

## Mesh refinement

<p align="center" margin="20px">
	<img src="https://raw.githubusercontent.com/seansovine/page_images/refs/heads/main/screenshots/vulkan_grapher/radial_sinc_mesh_refinement_2025-12-12.png" alt="drawing" width="700" style="padding-top: 10px; padding-bottom: 10px"/>
</p>

The green coloration in the image is a debugging feature to show where the mesh was refined.
To decide whether to refine a square cell of the mesh, we find the variation of the function
on that cell -- the difference between its max and min values there -- and also an estimate
of the magnitude of its second derivatives. If either of these exceeds a predefined threshold,
then the cell is selected for refinement. In the future we'll add these threshold parameters
to the UI for user selection.

## PBR material shading

<p align="center" margin="20px">
	<img src="https://raw.githubusercontent.com/seansovine/page_images/refs/heads/main/screenshots/vulkan_grapher/radial_sinc_frag_pbr_2_2025-12-16.png" alt="drawing" width="700" style="padding-top: 10px; padding-bottom: 10px"/>
</p>

There is now the option to render the graph surface as a solid with basic PBR material shading.
The current version of this closely follows the examples from the excellent site
[learnopengl.com](https://learnopengl.com/).

The GUI now also features sliders to adjust the surface color and its PBR metallic and roughness
properties. The lighting effects in this shading reveal some of the shortcomings of our
current meshing approach, which we'll work on improving. However, moving the PBR computations
to the fragment shader has also drastically improved this, by reducing the effects of fragment
interpolation.

## To do next

Next we will add these graphing features:

1. Either find or write a mathematical expression parsing library in C/C++.

2. Add better user interaction features like mouse control of the view.

3. Add user control of graphing and render parameters.

4. Keep looking at ways to improve the mesh quality for round surfaces.

## Sources and credits

We are following the tutorial at [vulkan-tutorial.com](https://vulkan-tutorial.com/) and studying
up in the _Vulkan Programming Guide_ by Graham Sellars. The example at [vk-sandbox](https://github.com/tstullich/vk-sandbox)
was helpful for figuring out the Dear ImGui integration. We are also looking at Sascha Willems'
Vulkan-glTF-PBR model viewer for inspiration, which is available [here](https://github.com/SaschaWillems/Vulkan-glTF-PBR) on GitHub.
The official Vulkan docs from Khronos are also pretty good.
