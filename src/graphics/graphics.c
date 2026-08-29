#include "../graphics.h"
#include "internal_graphics.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector.h>

#include "../volk.h"

#include "tracy/TracyC.h"

static const char *(extensions[]) = {
    VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,

#if DEBUG
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
};
static u32 extension_count = _countof(extensions);

static const char *(validationLayers[]) = {
    "VK_LAYER_KHRONOS_validation",
};
static u32 validationLayerCount = _countof(validationLayers);

static const char *(deviceExtensions[]) = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
static u32 deviceExtensionCount = _countof(validationLayers);


static VkDynamicState dynamicStates[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
};
static u32 dynamicStateCount = _countof(dynamicStates);

#if DEBUG
static constexpr bool enableValidationLayers = true;
#else
static constexpr bool enableValidationLayers = false;
#endif

static TraceLogLevel logTypeLevel = LOG_ALL;

void TraceLog(int logType, const char *text, ...) {
#ifdef TRACE_LOG
    // Message has level below current threshold, don't emit
    if (((TraceLogLevel)logType < logTypeLevel) || (text == nullptr)) return;

    va_list args;
    va_start(args, text);

#if defined(PLATFORM_ANDROID)
    switch (logType) {
        case LOG_TRACE: __android_log_vprint(ANDROID_LOG_VERBOSE, "raylib", text, args); break;
        case LOG_DEBUG: __android_log_vprint(ANDROID_LOG_DEBUG, "raylib", text, args); break;
        case LOG_INFO: __android_log_vprint(ANDROID_LOG_INFO, "raylib", text, args); break;
        case LOG_WARNING: __android_log_vprint(ANDROID_LOG_WARN, "raylib", text, args); break;
        case LOG_ERROR: __android_log_vprint(ANDROID_LOG_ERROR, "raylib", text, args); break;
        case LOG_FATAL: __android_log_vprint(ANDROID_LOG_FATAL, "raylib", text, args); break;
        default: break;
    }
#else
    char buffer[MAX_TRACELOG_MSG_LENGTH] = {0};

    switch (logType) {
        case LOG_TRACE: memcpy(buffer, "TRACE: ", 7); break;
        case LOG_DEBUG: memcpy(buffer, "DEBUG: ", 7); break;
        case LOG_INFO: memcpy(buffer, "INFO: ", 6); break;
        case LOG_WARNING: memcpy(buffer, "WARNING: ", 9); break;
        case LOG_ERROR: memcpy(buffer, "ERROR: ", 7); break;
        case LOG_FATAL: memcpy(buffer, "FATAL: ", 7); break;
        default: break;
    }

    const unsigned int textLength = (unsigned int)strlen(text);
    memcpy(buffer + strlen(buffer), text, (textLength < (MAX_TRACELOG_MSG_LENGTH - 12))? textLength : (MAX_TRACELOG_MSG_LENGTH - 12));
    strcat(buffer, "\n");
    vprintf(buffer, args);
    fflush(stdout);
#endif

    va_end(args);

    if (logType == LOG_FATAL) exit(EXIT_FAILURE);  // If fatal logging, exit program
#else
    (void)logType;
    (void)text;
    (void)logTypeLevel;
#endif
}

const char* VkResultToString(VkResult result)
{
    switch (result)
    {
        case VK_SUCCESS: return "VK_SUCCESS";
        case VK_NOT_READY: return "VK_NOT_READY";
        case VK_TIMEOUT: return "VK_TIMEOUT";
        case VK_EVENT_SET: return "VK_EVENT_SET";
        case VK_EVENT_RESET: return "VK_EVENT_RESET";
        case VK_INCOMPLETE: return "VK_INCOMPLETE";

        case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";

        case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";

        case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_PIPELINE_COMPILE_REQUIRED:
            return "VK_PIPELINE_COMPILE_REQUIRED";

        case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";

        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT";

        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";

        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
            return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";

        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";

        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";

        case VK_THREAD_IDLE_KHR: return "VK_THREAD_IDLE_KHR";
        case VK_THREAD_DONE_KHR: return "VK_THREAD_DONE_KHR";
        case VK_OPERATION_DEFERRED_KHR: return "VK_OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR:
            return "VK_OPERATION_NOT_DEFERRED_KHR";

        case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
            return "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";

        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
            return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";

        default:
            return "VK_UNKNOWN_RESULT";
    }
}

void checkGLFW(int glfw_error) {
    const char *description;

    if (glfw_error == GLFW_TRUE) return;

    glfwGetError(&description);
    TraceLog(LOG_FATAL, "%s\n", description);
    exit(1);
}

void checkVk(VkResult vk_error) {
    if (vk_error == VK_SUCCESS) return;

    TraceLog(LOG_FATAL, "%s\n", VkResultToString(vk_error));
    exit(1);
}

void checkError(Error error) {
    if (error == ERROR_OK) return;

    TraceLog(LOG_FATAL, "%s\n", clib_error_message(error));
    exit(1);
}

bool checkValidationLayerSupport() {
    u32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    VkLayerProperties *availableLayers = malloc(sizeof(VkLayerProperties) * layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

    for (u32 i = 0; i < layerCount; i++) {
        if (strcmp(availableLayers[i].layerName, "VK_LAYER_KHRONOS_validation") != 0) continue;

        free(availableLayers);
        return true;
    }

    free(availableLayers);
    return false;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    (void)pUserData;
    (void)messageSeverity;
    (void)messageType;

    TraceLog(LOG_DEBUG, "validation layer: %s", pCallbackData->pMessage);

    return VK_FALSE;
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT *createInfo) {
    *createInfo = (VkDebugUtilsMessengerCreateInfoEXT){
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = 0,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
    };
}

void createInstance(App *app) {
    TraceLog(LOG_INFO, "Creating instance");

    if (enableValidationLayers && !checkValidationLayerSupport()) {
        TraceLog(LOG_FATAL, "validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = nullptr,
        .pApplicationName = "Hello Triangle",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "No Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0
    };

    u32 glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    Vector required_extensions;
    checkError(vector_init(&required_extensions, sizeof(const char**)));

    for (u32 i = 0; i < glfwExtensionCount; i++) {
        checkError(vector_push(&required_extensions, &glfwExtensions[i]));
    }

    for (u32 i = 0; i < extension_count; i++) {
        checkError(vector_push(&required_extensions, &extensions[i]));
    }

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = (u32)required_extensions.length,
        .ppEnabledExtensionNames = required_extensions.data
    };

    if (enableValidationLayers) {
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {0};

        createInfo.enabledLayerCount = validationLayerCount;
        createInfo.ppEnabledLayerNames = validationLayers;

        populateDebugMessengerCreateInfo(&debugCreateInfo);
        createInfo.pNext = &debugCreateInfo;
    }

    checkVk(vkCreateInstance(&createInfo, nullptr, &app->instance));

    volkLoadInstance(app->instance);

    u32 extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    VkExtensionProperties *_extensions = malloc(sizeof(VkExtensionProperties) * extensionCount);

    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, _extensions);

    free(_extensions);
}

void setupDebugMessenger(App *app) {
    if (!enableValidationLayers) return;
    TraceLog(LOG_INFO, "Creating debug messenger");

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(&createInfo);

    if (vkCreateDebugUtilsMessengerEXT(app->instance, &createInfo, nullptr, &app->debugMessenger) != VK_SUCCESS) {
        TraceLog(LOG_FATAL, "failed to set up debug messenger");
    }
}

typedef struct optional_32 {
    u32 value;
    bool has_value;
} optional_32;

typedef struct QueueFamilyIndices {
    optional_32 graphicsFamily;
    optional_32 presentFamily;
} QueueFamilyIndices;

bool queueFamilyIndices_is_complete(const QueueFamilyIndices indices) {
    return indices.graphicsFamily.has_value && indices.presentFamily.has_value;
}

QueueFamilyIndices findQueueFamilies(const App *app, const VkPhysicalDevice device) {
    QueueFamilyIndices indices = {0};

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    VkQueueFamilyProperties *queueFamilies = malloc(sizeof(VkQueueFamilyProperties) * queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

    for (u32 i = 0; i < queueFamilyCount; i++) {
        const VkQueueFamilyProperties *queueFamily = &queueFamilies[i];

        if (queueFamily->queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily.value = i;
            indices.graphicsFamily.has_value = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, app->window.surface, &presentSupport);

        if (presentSupport) {
            indices.presentFamily.value = i;
            indices.presentFamily.has_value = true;
        }
    }

    free(queueFamilies);
    return indices;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
    u32 extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    VkExtensionProperties *availableExtensions = malloc(sizeof(VkExtensionProperties) * extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions);

    bool *extensionSupported = malloc(sizeof(bool) * deviceExtensionCount);
    memset(extensionSupported, 0, sizeof(bool) * deviceExtensionCount);

    for (u32 i = 0; i < extensionCount; i++) {
        for (u32 j = 0; j < deviceExtensionCount; j++) {
            if (strcmp(availableExtensions[i].extensionName, deviceExtensions[j]) == 0) {
                extensionSupported[j] = true;
                TraceLog(LOG_DEBUG, "%d, %d: %s, %s", i, j, availableExtensions[i].extensionName, deviceExtensions[j]);
            }
        }
    }

    bool extensionsSupported = true;

    for (u32 i = 0; i < deviceExtensionCount; i++) {
        extensionsSupported = extensionSupported[i] ? extensionsSupported : false;
    }

    free(availableExtensions);
    free(extensionSupported);
    return extensionsSupported;
}

typedef struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;

    u32 formatCount;
    VkSurfaceFormatKHR *formats;

    u32 presentModeCount;
    VkPresentModeKHR *presentModes;
} SwapChainSupportDetails;

SwapChainSupportDetails querySwapChainSupport(const App *app, const struct VkPhysicalDevice_T *device) {
    SwapChainSupportDetails details = {0};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR((VkPhysicalDevice)device, app->window.surface, &details.capabilities);

    vkGetPhysicalDeviceSurfaceFormatsKHR((VkPhysicalDevice)device, app->window.surface, &details.formatCount, nullptr);

    if (details.formatCount != 0) {
        details.formats = malloc(sizeof(VkSurfaceFormatKHR) * details.formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR((VkPhysicalDevice)device, app->window.surface, &details.formatCount, details.formats);
    }

    vkGetPhysicalDeviceSurfacePresentModesKHR((VkPhysicalDevice)device, app->window.surface, &details.presentModeCount, nullptr);

    if (details.presentModeCount != 0) {
        details.presentModes = malloc(sizeof(VkPresentModeKHR) * details.presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR((VkPhysicalDevice)device, app->window.surface, &details.presentModeCount, details.presentModes);
    }

    return details;
}

bool isDeviceSuitable(const App *app, const struct VkPhysicalDevice_T *device) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;

    vkGetPhysicalDeviceProperties((VkPhysicalDevice)device, &deviceProperties);
    vkGetPhysicalDeviceFeatures((VkPhysicalDevice)device, &deviceFeatures);

    const bool extensionsSupported = checkDeviceExtensionSupport((VkPhysicalDevice)device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        const SwapChainSupportDetails swapChainSupport = querySwapChainSupport(app, device);
        swapChainAdequate = swapChainSupport.formats != nullptr && swapChainSupport.presentModes != nullptr;

        free(swapChainSupport.formats);
        free(swapChainSupport.presentModes);
    }

    const bool result =
            deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
            queueFamilyIndices_is_complete(findQueueFamilies(app, (VkPhysicalDevice)device)) &&
            extensionsSupported &&
            swapChainAdequate;

    return result;
}

void pickPhysicalDevice(App *app) {
    TraceLog(LOG_INFO, "Picking GPU");
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(app->instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        TraceLog(LOG_FATAL, "Failed to find GPUs with Vulkan support");
    }

    VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * deviceCount);
    vkEnumeratePhysicalDevices(app->instance, &deviceCount, devices);

    // TODO: make ts smarter like bro this is ASS
    for (u32 i = 0; i < deviceCount; i++) {
        const struct VkPhysicalDevice_T *device = devices[i];
        if (isDeviceSuitable(app, (VkPhysicalDevice)device)) {
            physicalDevice = (VkPhysicalDevice)device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        free(devices);
        TraceLog(LOG_FATAL, "Failed to find a suitable GPU");
    }

    app->physicalDevice = physicalDevice;

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

    TraceLog(LOG_INFO, "Found GPU: %s", deviceProperties.deviceName);
    free(devices);
}

void createLogicalDevice(App *app) {
    const QueueFamilyIndices indices = findQueueFamilies(app, app->physicalDevice);
    float queuePriority = 1.0f;

    u32 queueCreateInfoCount = 1;

    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = indices.graphicsFamily.value,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    VkDeviceQueueCreateInfo presentQueueCreateInfo;

    if (indices.graphicsFamily.value != indices.presentFamily.value) {
        queueCreateInfoCount++;

        presentQueueCreateInfo = (VkDeviceQueueCreateInfo){
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = indices.presentFamily.value,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority
        };
    }

    VkDeviceQueueCreateInfo *queueCreateInfoList = malloc(sizeof(VkDeviceQueueCreateInfo) * queueCreateInfoCount);
    memcpy(&queueCreateInfoList[0], &queueCreateInfo, sizeof(VkDeviceQueueCreateInfo));

    if (queueCreateInfoCount == 2) {
        memcpy(&queueCreateInfoList[1], &presentQueueCreateInfo, sizeof(VkDeviceQueueCreateInfo));
    }

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(app->physicalDevice, &deviceFeatures);

    const VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueCreateInfoCount = queueCreateInfoCount,
        .pQueueCreateInfos = queueCreateInfoList,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = deviceExtensionCount,
        .ppEnabledExtensionNames = deviceExtensions,
        .pEnabledFeatures = &deviceFeatures
    };

    checkVk(vkCreateDevice(app->physicalDevice, &createInfo, nullptr, &app->device));
    vkGetDeviceQueue(app->device, indices.graphicsFamily.value, 0, &app->graphicsQueue);
    vkGetDeviceQueue(app->device, indices.presentFamily.value, 0, &app->presentQueue);

    free(queueCreateInfoList);
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const VkSurfaceFormatKHR *availableFormats, const u32 availableFormatCount) {
    for (u32 i = 0; i < availableFormatCount; i++) {
        const VkSurfaceFormatKHR availableFormat = availableFormats[i];
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const VkPresentModeKHR *availablePresentModes, const u32 availablePresentModeCount) {
    for (u32 i = 0; i < availablePresentModeCount; i++) {
        if (availablePresentModes[i] == VK_PRESENT_MODE_FIFO_KHR) {
            return availablePresentModes[i];
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

u32 clamp(const u32 value, const u32 min, const u32 max) {
    if (value < min) {
        return min;
    }

    if (value > max) {
        return max;
    }

    return value;
}

VkExtent2D chooseSwapExtent(const App *app, const VkSurfaceCapabilitiesKHR *capabilities) {
    if (capabilities->currentExtent.width != UINT32_MAX) {
        return capabilities->currentExtent;
    }

    int width, height;
    glfwGetWindowSize(app->window.glfwWindow, &width, &height);

    VkExtent2D actualExtent = {
        .width = (u32)width,
        .height = (u32)height
    };

    actualExtent.width = clamp(actualExtent.width, capabilities->minImageExtent.width, capabilities->maxImageExtent.width);
    actualExtent.height = clamp(actualExtent.height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height);

    return actualExtent;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto const func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void createLogicalGPUDeviceAndQueue(App *app) {
    pickPhysicalDevice(app);
    createLogicalDevice(app);
}

void createSwapChain(App *app) {
    const SwapChainSupportDetails swapChainSupport = querySwapChainSupport(app, app->physicalDevice);

    const VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats, swapChainSupport.formatCount);
    const VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes, swapChainSupport.presentModeCount);
    const VkExtent2D extent = chooseSwapExtent(app, &swapChainSupport.capabilities);

    app->window.imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && app->window.imageCount > swapChainSupport.capabilities.maxImageCount) {
        app->window.imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapChainCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext = nullptr,
        .flags = 0,
        .surface = app->window.surface,
        .minImageCount = app->window.imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .preTransform = swapChainSupport.capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    const QueueFamilyIndices indices = findQueueFamilies(app, app->physicalDevice);
    const u32 queueFamilyIndices[] = {indices.graphicsFamily.value, indices.presentFamily.value};

    if (indices.graphicsFamily.value != indices.presentFamily.value) {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }

    checkVk(vkCreateSwapchainKHR(app->device, &swapChainCreateInfo, nullptr, &app->window.swapChain));

    app->window.images = malloc(sizeof(VkImage) * app->window.imageCount);
    vkGetSwapchainImagesKHR(app->device, app->window.swapChain, &app->window.imageCount, app->window.images);

    app->window.swapChainImageFormat = surfaceFormat.format;
    app->window.swapChainExtent = extent;

    free(swapChainSupport.formats);
    free(swapChainSupport.presentModes);
}

void createImageViews(App *app) {
    app->window.imageViews = malloc(sizeof(VkImageView) * app->window.imageCount);

    for (size_t i = 0; i < app->window.imageCount; i++) {
        VkImageViewCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .image = app->window.images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = app->window.swapChainImageFormat,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        checkVk(vkCreateImageView(app->device, &createInfo, nullptr, &app->window.imageViews[i]));
    }
}

static Vector readFileBinary(const char *path) {
    Vector file_buff;
    checkError(vector_init(&file_buff, sizeof(char)));

    FILE *file = fopen(path, "rb");
    fseek(file, 0, SEEK_END);

    const size_t file_size = (size_t)ftell(file);
    checkError(vector_reserve(&file_buff, file_size));

    fseek(file, 0, SEEK_SET);

    fread(file_buff.data, sizeof(char), file_size, file);

    fclose(file);

    file_buff.length = file_buff.capacity;

    return file_buff;
}

VkShaderModule createShaderModule(const App *app, const Vector code) {
    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = code.length,
        .pCode = (u32 *)code.data
    };

    VkShaderModule shaderModule;
    checkVk(vkCreateShaderModule(app->device, &createInfo, nullptr, &shaderModule));

    return shaderModule;
}

void createRenderPass(App *app) {
    VkAttachmentDescription colorAttachment = {
        .flags = 0,
        .format = app->window.swapChainImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .flags = 0,
        .pipelineBindPoint = 0,
        .inputAttachmentCount = 0,
        .pInputAttachments = nullptr,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pResolveAttachments = nullptr,
        .pDepthStencilAttachment =nullptr ,
        .preserveAttachmentCount = 0,
        .pPreserveAttachments = nullptr
    };

    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = 0
    };

    VkRenderPassCreateInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    checkVk(vkCreateRenderPass(app->device, &renderPassInfo, nullptr, &app->renderPass));
}

static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    return bindingDescription;
}

static VkVertexInputAttributeDescription *getAttributeDescriptions(u32 *count) {
    static VkVertexInputAttributeDescription description[2] = {
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(Vertex, pos)
        },
        {
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32A32_SFLOAT,
            .offset = offsetof(Vertex, color)
        }
    }; // TODO some magic BS or smth idk


    *count = 2;
    return description;
}

void createDescriptorSetLayout(App *app) {
    const VkDescriptorSetLayoutBinding uboLayoutBinding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr
    };

    const VkDescriptorSetLayoutCreateInfo layoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 1,
        .pBindings = &uboLayoutBinding
    };

    checkVk(vkCreateDescriptorSetLayout(app->device, &layoutInfo, nullptr, &app->descriptorSetLayout));
}

void createGraphicsPipeline(App *app) {
    Vector vertShaderCode = readFileBinary("assets/shaders/vert.spv");
    Vector fragShaderCode = readFileBinary("assets/shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(app, vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(app, fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main",
        .pSpecializationInfo = nullptr // for passing constants to the shader module
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
        .pName = "main",
        .pSpecializationInfo = nullptr // for passing constants to the shader module
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertShaderStageInfo,
        fragShaderStageInfo,
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = dynamicStateCount,
        .pDynamicStates = dynamicStates
    };

    u32 attributeDescriptionCount;
    VkVertexInputBindingDescription bindingDescription = getBindingDescription();
    VkVertexInputAttributeDescription *attributeDescriptions = getAttributeDescriptions(&attributeDescriptionCount);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = attributeDescriptionCount,
        .pVertexAttributeDescriptions = attributeDescriptions
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)app->window.swapChainExtent.width,
        .height = (float)app->window.swapChainExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = app->window.swapChainExtent
    };

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    // TODO depth buffer via "VkPipelineDepthStencilStateCreateInfo"

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT |
                VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT |
                VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants = {
            0.0f,
            0.0f,
            0.0f,
            0.0f
        }
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = &app->descriptorSetLayout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

    checkVk(vkCreatePipelineLayout(app->device, &pipelineLayoutInfo, nullptr, &app->pipelineLayout));

    createRenderPass(app);

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pTessellationState = nullptr,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &colorBlending,
        .pDynamicState = &dynamicState,
        .layout = app->pipelineLayout,
        .renderPass = app->renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    checkVk(vkCreateGraphicsPipelines(app->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &app->graphicsPipeline));

    vkDestroyShaderModule(app->device, fragShaderModule, nullptr);
    vkDestroyShaderModule(app->device, vertShaderModule, nullptr);
}

void createFramebuffers(App *app) {
    app->window.swapChainFramebuffers = malloc(sizeof(VkFramebuffer) * app->window.imageCount);

    for (size_t i = 0; i < app->window.imageCount; i++) {
        VkImageView attachments[] = {
            app->window.imageViews[i],
        };

        VkFramebufferCreateInfo framebufferInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .renderPass = app->renderPass,
            .attachmentCount = 1,
            .pAttachments = attachments,
            .width = app->window.swapChainExtent.width,
            .height = app->window.swapChainExtent.height,
            .layers = 1
        };

        checkVk(vkCreateFramebuffer(app->device, &framebufferInfo, nullptr, &app->window.swapChainFramebuffers[i]));
    }
}

void createCommandPool(App *app) {
    const QueueFamilyIndices queueFamilyIndices = findQueueFamilies(app, app->physicalDevice);

    const VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value
    };

    checkVk(vkCreateCommandPool(app->device, &poolInfo, nullptr, &app->commandPool));
}

void createUniformBuffers(App *app) {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        app->window.uniformBuffers[i] = CreateBuffer(
            app,
            sizeof(UBO),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        app->window.mappedUniformBuffers[i] = MapBuffer(app, &app->window.uniformBuffers[i]);
    }
}

void createDescriptorPool(App *app) {
    const VkDescriptorPoolSize poolSize = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = MAX_FRAMES_IN_FLIGHT
    };

    const VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize
    };

    checkVk(vkCreateDescriptorPool(app->device, &poolInfo, nullptr, &app->descriptorPool));
}

void createDescriptorSets(App *app) {
    VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT];
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        layouts[i] = app->descriptorSetLayout;
    }

    const VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = app->descriptorPool,
        .descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
        .pSetLayouts = layouts,
    };

    checkVk(vkAllocateDescriptorSets(app->device, &allocInfo, app->window.descriptorSets));

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        const VkDescriptorBufferInfo bufferInfo = {
            .buffer = app->window.uniformBuffers[i].handle,
            .offset = 0,
            .range = sizeof(UBO)
        };

        const VkWriteDescriptorSet descriptorWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = app->window.descriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &bufferInfo,
            .pTexelBufferView = nullptr
        };

        vkUpdateDescriptorSets(app->device, 1, &descriptorWrite, 0, nullptr);
    }
}

u32 findMemoryType(App *app, u32 typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(app->physicalDevice, &memProperties);

    for (u32 i = 0; i < memProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    TraceLog(LOG_FATAL, "We dead");
    exit(1);
}

void createCommandBuffers(App *app) {
    const VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = app->commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };

    checkVk(vkAllocateCommandBuffers(app->device, &allocInfo, app->window.commandBuffers));
}

void createSyncObjects(App *app) {
    app->window.presentationReadySemaphore = malloc(sizeof(VkSemaphore) * app->window.imageCount);

    const VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };

    const VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        checkVk(vkCreateSemaphore(app->device, &semaphoreInfo, nullptr, &app->window.imageAvailableSemaphore[i]));
        checkVk(vkCreateFence(app->device, &fenceInfo, nullptr, &app->window.frameFinishedFence[i]));
    }

    for (size_t i = 0; i < app->window.imageCount; i++) {
        checkVk(vkCreateSemaphore(app->device, &semaphoreInfo, nullptr, &app->window.presentationReadySemaphore[i]));
    }
}

void cleanupSwapChain(const App *app) {
    for (size_t i = 0; i < app->window.imageCount; i++) {
        vkDestroyFramebuffer(app->device, app->window.swapChainFramebuffers[i], nullptr);
    }

    for (size_t i = 0; i < app->window.imageCount; i++) {
        vkDestroyImageView(app->device, app->window.imageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(app->device, app->window.swapChain, nullptr);
}

void recreateSwapChain(App *app) {
    int width = 0, height = 0;
    glfwGetFramebufferSize(app->window.glfwWindow, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(app->window.glfwWindow, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(app->device);

    cleanupSwapChain(app);

    createSwapChain(app);
    createImageViews(app);
    createFramebuffers(app);
}
