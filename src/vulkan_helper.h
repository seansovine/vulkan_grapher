#ifndef VULKAN_HELPER_H_
#define VULKAN_HELPER_H_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdint>
#include <cstring>
#include <set>
#include <stdexcept>
#include <vector>

class VulkanHelper {
public:
    static bool checkDeviceExtensions(VkPhysicalDevice device, const std::vector<const char *> deviceExtensions) {
        uint32_t extensionsCount = 0;
        if (vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Unable to enumerate device extensions!");
        }

        std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, availableExtensions.data());

        std::set<std::string> requiredDeviceExtensions(deviceExtensions.begin(), deviceExtensions.end());
        for (const auto &extension : availableExtensions) {
            requiredDeviceExtensions.erase(extension.extensionName);
        }

        return requiredDeviceExtensions.empty();
    }

    static bool checkValidationLayerSupport(const std::vector<const char *> validationLayers) {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char *layerName : validationLayers) {
            bool layerFound = false;
            for (const auto &layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }
        return true;
    }
};

#endif // VULKAN_HELPER_H_
