#include <string.h>
#include <stdlib.h>

#include "../graphics.h"
#include "internal_graphics.h"
#include "../volk.h"

#include "tracy/TracyC.h"

#if DEBUG
constexpr bool enableValidationLayers = true;
#else
constexpr bool enableValidationLayers = false;
#endif

void InitVulkan(App *app) {
    TraceLog(LOG_INFO, "Initializing vulkan");
    checkVk(volkInitialize());
    checkGLFW(glfwInit());

    createInstance(app);
    setupDebugMessenger(app);
}

void InitWindow(App *app) {
    TraceLog(LOG_INFO, "Initializing window");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    GLFWwindow *glfwWindow = glfwCreateWindow(1920, 1080, "help me", nullptr, nullptr);
    VkSurfaceKHR surface;
    glfwCreateWindowSurface(app->instance, glfwWindow, nullptr, &surface);

    app->window = (Window){
        .glfwWindow = glfwWindow,
        .surface = surface
    };

    app->window.frameInFlightIndex = 0;
}

void InitGfx(App *app) {
    createLogicalGPUDeviceAndQueue(app);
    createSwapChain(app);
    createImageViews(app);
    createDescriptorSetLayout(app);
    createGraphicsPipeline(app);
    createFramebuffers(app);
    createCommandPool(app);
    createUniformBuffers(app);
    createDescriptorPool(app);
    createDescriptorSets(app);
    createCommandBuffers(app);
    createSyncObjects(app);
}

void Cleanup(const App *app) {
    vkDeviceWaitIdle(app->device);

    cleanupSwapChain(app);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        DestroyBuffer(app, app->window.uniformBuffers[i]);
    }

    vkDestroyDescriptorPool(app->device, app->descriptorPool, nullptr);

    vkDestroyDescriptorSetLayout(app->device, app->descriptorSetLayout, nullptr);

    vkDestroyPipeline(app->device, app->graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(app->device, app->pipelineLayout, nullptr);

    vkDestroyRenderPass(app->device, app->renderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(app->device, app->window.imageAvailableSemaphore[i], nullptr);
        vkDestroyFence(app->device, app->window.frameFinishedFence[i], nullptr);
    }

    for (size_t i = 0; i < app->window.imageCount; i++) {
        vkDestroySemaphore(app->device, app->window.presentationReadySemaphore[i], nullptr);
    }

    vkDestroyCommandPool(app->device, app->commandPool, nullptr);

    vkDestroyDevice(app->device, nullptr);

    if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(app->instance, app->debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(app->instance, app->window.surface, nullptr);
    vkDestroyInstance(app->instance, nullptr);

    glfwDestroyWindow(app->window.glfwWindow);

    glfwTerminate();

    free(app->window.images);
    free(app->window.imageViews);
    free(app->window.swapChainFramebuffers);
}

VkCommandBuffer BeginFrame(App *app) {
    retry_frame:
    TracyCZoneN(fenceTime, "vkWaitForFences", true);
    const VkFence *frameFence = &app->window.frameFinishedFence[app->window.frameInFlightIndex];
    checkVk(vkWaitForFences(app->device, 1, frameFence, true, UINT64_MAX));
    TracyCZoneEnd(fenceTime);

    TracyCZoneN(acquireTime, "vkAcquireNextImage", true);
    VkSemaphore acquiredFlag = app->window.imageAvailableSemaphore[app->window.frameInFlightIndex];
    const VkResult result = vkAcquireNextImageKHR(app->device, app->window.swapChain, UINT64_MAX, acquiredFlag, nullptr, &app->window.activeImageIndex);
    TracyCZoneEnd(acquireTime);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain(app);
        goto retry_frame;
    }

    checkVk(result);
    checkVk(vkResetFences(app->device, 1, frameFence));

    TracyCZoneN(commandTime, "command setup", true);
    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = 0,
        .pInheritanceInfo = nullptr
    };

    VkCommandBuffer cmdBuffer = app->window.commandBuffers[app->window.frameInFlightIndex];
    checkVk(vkResetCommandBuffer(cmdBuffer, 0));
    checkVk(vkBeginCommandBuffer(cmdBuffer, &beginInfo));
    TracyCZoneEnd(commandTime);

    return cmdBuffer;
}

void EndFrame(App *app) {
    const VkFence *frameFence = &app->window.frameFinishedFence[app->window.frameInFlightIndex];
    VkSemaphore acquiredFlag = app->window.imageAvailableSemaphore[app->window.frameInFlightIndex];
    VkCommandBuffer cmdBuffer = app->window.commandBuffers[app->window.frameInFlightIndex];

    vkCmdEndRenderPass(cmdBuffer);
    vkEndCommandBuffer(cmdBuffer);

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &acquiredFlag,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &app->window.presentationReadySemaphore[app->window.activeImageIndex]
    };

    checkVk(vkQueueSubmit(app->graphicsQueue, 1, &submitInfo, *frameFence));

    const VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &app->window.presentationReadySemaphore[app->window.activeImageIndex],
        .swapchainCount = 1,
        .pSwapchains = &app->window.swapChain,
        .pImageIndices = &app->window.activeImageIndex,
        .pResults = nullptr
    };

    vkQueuePresentKHR(app->presentQueue, &presentInfo);

    app->window.frameInFlightIndex = (app->window.frameInFlightIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

Buffer CreateBuffer(App *app, const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties) {
    Buffer buffer;
    buffer.size = size;

    const VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };

    checkVk(vkCreateBuffer(app->device, &bufferInfo, nullptr, &buffer.handle));

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(app->device, buffer.handle, &memoryRequirements);

    const VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = findMemoryType(app, memoryRequirements.memoryTypeBits, properties)
    };

    checkVk(vkAllocateMemory(app->device, &allocInfo, nullptr, &buffer.memory));

    vkBindBufferMemory(app->device, buffer.handle, buffer.memory, 0);

    return buffer;
}

void DestroyBuffer(const App *app, Buffer buffer) {
    vkDestroyBuffer(app->device, buffer.handle, nullptr);
    vkFreeMemory(app->device, buffer.memory, nullptr);

    memset(&buffer, 0, sizeof(Buffer));
}

void *MapBuffer(const App *app, const Buffer *buffer) {
    void *data;
    vkMapMemory(app->device, buffer->memory, 0, buffer->size, 0, &data);
    return data;
}

void UnmapBuffer(const App *app, const Buffer *buffer) {
    vkUnmapMemory(app->device, buffer->memory);
}

void UploadBuffer(const App *app, const Buffer *buffer, const void *data, const VkDeviceSize size) {
    void *buff_map = MapBuffer(app, buffer);
    memcpy(buff_map, data, size);
    UnmapBuffer(app, buffer);
}

void CopyBuffer(const App *app, const Buffer *src, const Buffer *dst, const VkDeviceSize size) {
    const VkCommandBufferAllocateInfo  allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = app->commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkCommandBuffer cmdBuff;
    vkAllocateCommandBuffers(app->device, &allocInfo, &cmdBuff);

    const VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };

    vkBeginCommandBuffer(cmdBuff, &beginInfo);

    const VkBufferCopy copyRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size
    };

    vkCmdCopyBuffer(cmdBuff, src->handle, dst->handle, 1, &copyRegion);

    vkEndCommandBuffer(cmdBuff);

    const VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdBuff,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores = nullptr
    };

    vkQueueSubmit(app->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(app->graphicsQueue);

    vkFreeCommandBuffers(app->device, app->commandPool, 1, &cmdBuff);
}
