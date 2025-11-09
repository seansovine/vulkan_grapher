# Developer Notes

## Validation layer complains

When we run the app we see

```text
vkFlushMappedMemoryRanges(): pMemoryRanges[1].size is VK_WHOLE_SIZE and the mapping end
    (2034 = 0 + 2034) not a multiple of VkPhysicalDeviceLimits::nonCoherentAtomSize (64)
    and not equal to the end of the memory object (2048).
```

And it looks like the source of this is the imGui Vulkan implementation.

_Possible solution:_

We can try upgrading the imGui library we're using.
