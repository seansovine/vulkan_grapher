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
