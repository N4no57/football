#ifndef FOOTIE_LIFE_INTERNAL_GRAPHICS_H
#define FOOTIE_LIFE_INTERNAL_GRAPHICS_H

#include <error.h>

#include "../graphics.h"


#ifndef MAX_TRACELOG_MSG_LENGTH
#define MAX_TRACELOG_MSG_LENGTH      256        // Max length of one trace-log message
#endif

void checkGLFW(int glfw_error);
void checkVk(VkResult vk_error);
void checkError(Error error);

void createInstance(App *app);
void setupDebugMessenger(App *app);
void createLogicalGPUDeviceAndQueue(App *app);
void createSwapChain(App *app);
void createImageViews(App *app);
void createDescriptorSetLayout(App *app);
void createGraphicsPipeline(App *app);
void createFramebuffers(App *app);
void createCommandPool(App *app);
void createUniformBuffers(App *app);
void createDescriptorPool(App *app);
void createDescriptorSets(App *app);
void createCommandBuffers(App *app);
void createSyncObjects(App *app);

void cleanupSwapChain(const App *app);
void recreateSwapChain(App *app);

u32 findMemoryType(App *app, u32 typeFilter, VkMemoryPropertyFlags properties);

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

#endif //FOOTIE_LIFE_INTERNAL_GRAPHICS_H
