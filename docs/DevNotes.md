# Developer Notes

## Some interesting example functions

- `0.1*sin(100*u*v)`

- `0.1*sin(25*u*v) + 0.05*cos(100*u*v)`

Interesting to vary the component weights and exponent here:

- `1 / (pow(4.0*pow(u-0.5,2) + pow(v-0.5,2), 0.75) + 0.75)`

For example:

- `1 / (pow(10.0*pow(u-0.5,2) + pow(v-0.5,2), 0.125) + 0.75)`

- `0.1*sin(10*cos(3*u)*cos(10*v))`

_How does it handle singularities?_ Often, not too badly:

- `sqrt(pow(u - 0.5, 2) + pow(v - 0.5, 2)) * atan2(u - 0.5, v - 0.5) / 4.0`

<p align="center" margin="20px">
	<img src="https://raw.githubusercontent.com/seansovine/page_images/refs/heads/main/screenshots/vulkan_grapher/atan2_material_props_2026-09-26.gif"
		alt="drawing" width="700" style="padding-top: 10px; padding-bottom: 10px"/>
</p>

## Normal computation and surface appearance

We have two ways to compute the normal.

1. Compute triangle normals and average them at each vertex.

2. Compute normal at each vertex directly from the function.

It looks like each has it's strengths and weaknesses. The first one averages normals
from neighboring triangles, so in theory should smooth out the variation in the normals.
The second is more accurate, but potentially amplifies the effects of numerical
approximation errors. We're trying these out to address some lighting artifacts we're
seeing on areas of the graph that are sharply curved and have rounded contours.

**Follow up explanation:** _Specular jaggies._

It looks like I was wrong about the source of the lighting artifacts I was seeing,
such as on the sharp contours of the "exponential of radial sinc" example shown below:

<p align="center" margin="20px">
	<img src="https://raw.githubusercontent.com/seansovine/page_images/refs/heads/main/screenshots/vulkan_grapher/specular_jaggies_2026-09-26.png"
		alt="drawing" width="300" style="padding-top: 10px; padding-bottom: 10px"/>
</p>

If you zoom in on the areas with these lighting artifacts, once the mesh fineness
reaches a certain point, the artifacts disappear. So the problem is not due to
errors in the normals, it's caused by something else.

It seems the problem is that when the "roughness" parameter is turned down low,
so you get a very shiny object, the specular highlights (see [Wikipedia](https://en.wikipedia.org/wiki/Specular_reflection))
in some graphs are concentrated onto thin curves. And when those curves bend sharply,
the GPU's rendering process approximates them to produce a stairstep effect commonly
known as "jaggies". Basically this is caused by a discrete approximation of a shape
by too few pixels that are arranged in a rectangular array, as they are on the screen.

So what we're seeing are "specular jaggies", which are a well-known phenomenon. There are
some advanced techniques that can help reduce their effect. Maybe todo: Try out some of
those techniques.

Or, maybe we can just rename them to "spectacular jaggies", and call it done. :P
