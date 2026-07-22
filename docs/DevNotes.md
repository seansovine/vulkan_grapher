# Developer Notes

## Normal computation and surface appearance

We have two ways to compute the normal.

1. Compute triangle normals and average them at each vertex.

2. Compute normal at each vertex numerically from the function.

It looks like each has it's strengths and weaknesses. The first one
works well in places where the mesh is stretched a lot -- roughly
where the first derivative is large. But zooming in closely we see
that it produces patches on the surface where the normal changes from
one triangle to the next.

The second method looks much better zoomed in, but produces artifacts
in some cases where the mesh is stretched a lot, such as a round-shaped
function in areas where it has a large first derivative.

It seems the problem ultimately comes from using a generic grid to build
a mesh for an arbitrarily-shaped function. One workaround may be to use
an interpolation between the two normals at each vertex, depending on the
properties of the graph there.

## Validation layer complains

When we run the app we see

```text
vkFlushMappedMemoryRanges(): pMemoryRanges[1].size is VK_WHOLE_SIZE and the mapping end
    (2034 = 0 + 2034) not a multiple of VkPhysicalDeviceLimits::nonCoherentAtomSize (64)
    and not equal to the end of the memory object (2048).
```

_Solution:_

We upgraded our version of the ImGui library and that fixed it.

## Some interesting example functions

- `0.1*sin(100*u*v)`

- `0.1*sin(25*u*v) + 0.05*cos(100*u*v)`

Interesting to vary the component weights and exponent here:

- `1 / (pow(4.0*pow(u-0.5,2) + pow(v-0.5,2), 0.75) + 0.75)`

For example:

- `1 / (pow(10.0*pow(u-0.5,2) + pow(v-0.5,2), 0.125) + 0.75)`

- `0.1*sin(10*cos(3*u)*cos(10*v))`
