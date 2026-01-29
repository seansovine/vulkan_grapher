# Vulkan Grapher

A work-in-progress 3D function grapher built with C++ + Vulkan + Dear ImGui.

We have previously written a version of the function grapher using C++ + OpenGL in [opengl_examples](https://github.com/seansovine/opengl_examples),
and a more full-featured version using Rust + wgpu + egui in [wgpu_grapher](https://github.com/seansovine/wgpu_grapher).

<p align="center" margin="20px">
	<img src="https://raw.githubusercontent.com/seansovine/page_images/refs/heads/main/screenshots/vulkan_grapher/radial_sinc_blue.png" alt="drawing" width="700" style="padding-top: 10px; padding-bottom: 10px"/>
</p>

_Graph of radial sinc function._

## Mouse controls

| Input                    | Action            |
| ------------------------ | ----------------- |
| `click + drag`           | rotate graph      |
| `control + click + drag` | translate graph   |
| `mouse wheel`            | zoom graph        |

## User function input

There is now the option for the user to enter a function expression:

<p align="center" margin="20px">
	<img src="https://raw.githubusercontent.com/seansovine/page_images/refs/heads/main/screenshots/vulkan_grapher/user_sine_product.png" alt="drawing" width="700" style="padding-top: 10px; padding-bottom: 10px"/>
</p>

This input will be parsed and evaluated with the [mathspresso](https://github.com/kobalicek/mathpresso) library
and used to generate a graph.

## Mesh refinement

The basic mesh generated for a graph looks much rougher in parts where the function --
either its values or its derivatives -- are changing rapidly. To address this we have a form of local
mesh refinement to try to add enough detail in these parts of the function without doing unnecessary
work in other areas of the graph. The mesh coloration in the image is a debugging feature to show where
the mesh was refined. The green color indicates one level of refinement, and the orange color indicates
where a second, further level was done.

<p align="center" margin="20px">
	<img src="https://raw.githubusercontent.com/seansovine/page_images/refs/heads/main/screenshots/vulkan_grapher/mesh_refinement_2_2026-01-01.png" alt="drawing" width="700" style="padding-top: 10px; padding-bottom: 10px"/>
</p>

To decide whether to refine a square cell of the mesh, we find the variation of the function
on that cell -- the difference between its max and min values there -- and also an estimate
of the magnitude of its second derivatives. If either of these exceeds a predefined threshold,
then the cell is selected for refinement. In the future we'll add these threshold parameters
to the UI for user selection.

## PBR material shading

<p align="center" margin="20px">
	<img src="https://raw.githubusercontent.com/seansovine/page_images/refs/heads/main/screenshots/vulkan_grapher/graph_pbr_example_2.png" alt="drawing" width="700" style="padding-top: 10px; padding-bottom: 10px"/>
</p>

The surface can be rendered as a wireframe or as a solid with PBR shading using the popular metallic
material model. The PBR shading implementation closely follows the examples from the excellent site
[learnopengl.com](https://learnopengl.com/). The GUI has sliders to adjust the surface color
and PBR metallic and roughness properties.

## Build instructions

There is a CMake build system, but you can use the Makefile as a convenience.

```bash
# Apologies if you need more; need to test from a clean env.
sudo apt install libvulkan-dev libglfw3-dev glslang-tools

make configure
make build

# Compile the shaders.
make shaderc

# Run built application in build/src/renderer-app.
make run
```

## To do next

Some more things I'd like to work on:

1. Keep looking at ways to improve the mesh quality for round surfaces.

2. Look into ways to improve the renderer efficiency.

## Sources and credits

We have learned from the tutorial at [vulkan-tutorial.com](https://vulkan-tutorial.com/) and
the official _Vulkan Programming Guide_ by Graham Sellars.
The example at [vk-sandbox](https://github.com/tstullich/vk-sandbox)
was helpful for figuring out the Dear ImGui integration. For mathematical expression parsing
we use the [mathspresso](https://github.com/kobalicek/mathpresso) library, which is released
under a permissive license. We have also looked at Sascha Willems'
Vulkan-glTF-PBR model viewer for inspiration, which is available [here](https://github.com/SaschaWillems/Vulkan-glTF-PBR).
And we have learned many graphics concepts from the excellent site [learnopengl.com](https://learnopengl.com/).

This software is released under the MIT license.
