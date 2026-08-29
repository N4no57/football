#ifndef FOOTIE_LIFE_GRAPHICS_H
#define FOOTIE_LIFE_GRAPHICS_H

#include <ints.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_NONE
#include <glfw3.h>

#include "maths/maths.h"

#ifndef MAX_FRAMES_IN_FLIGHT
#define MAX_FRAMES_IN_FLIGHT 2
#endif

#include "logging.h"

typedef struct Buffer {
    VkBuffer handle;
    VkDeviceMemory memory;
    VkDeviceSize size;
} Buffer;

typedef struct Window {
    GLFWwindow *glfwWindow; // native window
    VkSurfaceKHR surface; // abstraction over native window
    VkSurfaceCapabilitiesKHR surfaceCapabilities;

    VkSwapchainKHR swapChain; // a bunch of images that we swap (double buffering but actually its n buffering)
    VkImage *images; // swap chain images
    u32 imageCount;
    VkImageView *imageViews; // count is the same is image count

    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    u32 activeImageIndex;
    u32 frameInFlightIndex;

    // size the same as image count which isn't always the same as the number of frames in flight
    VkFramebuffer *swapChainFramebuffers;
    VkSemaphore *presentationReadySemaphore;

    VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];

    VkSemaphore imageAvailableSemaphore[MAX_FRAMES_IN_FLIGHT];
    VkFence frameFinishedFence[MAX_FRAMES_IN_FLIGHT];

    Buffer uniformBuffers[MAX_FRAMES_IN_FLIGHT];
    void *mappedUniformBuffers[MAX_FRAMES_IN_FLIGHT];

    VkDescriptorSet descriptorSets[MAX_FRAMES_IN_FLIGHT];
} Window;

typedef struct App {
    Window window;
    VkInstance instance;

    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;

    VkCommandPool commandPool;

    VkRenderPass renderPass;

    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkDebugUtilsMessengerEXT debugMessenger;
} App;

typedef struct Vertex {
    Vector3 pos;
    Vector4 color;
} Vertex;

typedef struct UBO {
    Matrix view;
    Matrix projection;
    Matrix reserved00;
    Matrix reserved01;
} UBO;

void InitVulkan(App *app);
void InitWindow(App *app);
void InitGfx(App *app);

Buffer CreateBuffer(App *app, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
void DestroyBuffer(const App *app, Buffer buffer);

void *MapBuffer(const App *app, const Buffer *buffer);
void UnmapBuffer(const App *app, const Buffer *buffer);

void UploadBuffer(const App *app, const Buffer *buffer, const void *data, VkDeviceSize size);
void CopyBuffer(const App *app, const Buffer *src, const Buffer *dst, VkDeviceSize size);

VkCommandBuffer BeginFrame(App *app);
void EndFrame(App *app);

void Cleanup(const App *app);

#endif //FOOTIE_LIFE_GRAPHICS_H
