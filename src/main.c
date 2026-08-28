// #include "common.h"
// #include "asset manager/asset_manager.h"
// #include "game/game.h"
// #include "scenes/main_menu.h"
//
// void dynamic_fps_set(const int target_fps, bool *last_focus_check) {
//     const bool is_focused = IsWindowFocused();
//
//     if (!is_focused && is_focused != *last_focus_check) {
//         SetTargetFPS(20);
//     } else if (is_focused && is_focused != *last_focus_check) {
//         SetTargetFPS(target_fps);
//     }
//
//     *last_focus_check = is_focused;
// }
//
// int main(void) {
//     asset_manager_init();
//
//     Game game;
//
//     GameInit(&game);
//
//     GameSetScene(&game, main_menu_scene(&game.settings), nullptr);
//
//     bool last_focus_check = true;
//     while (!WindowShouldClose()) {
//         BeginDrawing();
//         GameDraw(&game);
//         EndDrawing();
//
//         GameUpdate(&game, GetFrameTime());
//         asset_manager_update();
//
//         if (game.settings.dynamic_fps) dynamic_fps_set(game.settings.target_fps, &last_focus_check);
//     }
//
//     GameShutdown(&game);
//     asset_manager_destroy();
//
//     return 0;
// }

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "volk.h"
#include "graphics.h"
#include "tracy/TracyC.h"

#ifdef _WIN32
typedef struct _LARGE_INTEGER { int64_t QuadPart; } LARGE_INTEGER;
__declspec(dllimport) int __stdcall QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount);
__declspec(dllimport) int __stdcall QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency);

i64 base = 0;
LARGE_INTEGER timerFrequency = {0};

void init_time() {
    LARGE_INTEGER time = {0};
    QueryPerformanceCounter(&time);
    QueryPerformanceFrequency(&timerFrequency);
    base = time.QuadPart;
}

double GetTime(void) {
    LARGE_INTEGER now = {0};
    QueryPerformanceCounter(&now);
    const double time = (double)(now.QuadPart - base)/(double)timerFrequency.QuadPart;

    return time;
}

#elifdef __linux__
    TraceLog(LOG_FATAL, "Can't create a GetTime() implementation for linux")
#else
    TraceLog(LOG_FATAL, "Can't create a GetTime() implementation")
#endif

Vertex vertices[] = {
    {{-1, -1, 0}, {1, 0, 0, 1}},
    {{1, -1, 0}, {0, 1, 0, 1}},
    {{1, 1, 0}, {0, 0, 1, 1}},
    {{-1, 1, 0}, {1, 1, 1, 1}}
};

u16 indices[] = {
    0, 1, 2, 2, 3, 0
};

void MainLoop(App *app);

int main() {
    App app = {0};

    #if DEBUG
        TraceLog(LOG_WARNING, "Debug build");
    #endif

    InitVulkan(&app);
    InitWindow(&app);
    InitGfx(&app);
    init_time();

    MainLoop(&app);

    Cleanup(&app);

    return 0;
}

void BeginRenderPass(App *app, VkCommandBuffer cmdBuffer) {
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    const VkRenderPassBeginInfo renderPassInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = app->renderPass,
        .framebuffer = app->window.swapChainFramebuffers[app->window.activeImageIndex],
        .renderArea = {
            .offset = {0, 0},
            .extent = app->window.swapChainExtent
        },
        .clearValueCount = 1,
        .pClearValues = &clearColor
    };

    vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->graphicsPipeline);

    const VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)app->window.swapChainExtent.width,
        .height = (float)app->window.swapChainExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

    const VkRect2D scissor = {
        .offset = {0, 0},
        .extent = app->window.swapChainExtent
    };
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
}

void print_matrix(Matrix matrix) {
    printf("%f %f %f %f\n", matrix.m0, matrix.m1, matrix.m2, matrix.m3);
    printf("%f %f %f %f\n", matrix.m4, matrix.m5, matrix.m6, matrix.m7);
    printf("%f %f %f %f\n", matrix.m8, matrix.m9, matrix.m10, matrix.m11);
    printf("%f %f %f %f\n", matrix.m12, matrix.m13, matrix.m14, matrix.m15);
}

void MainLoop(App *app) {
    double previous = GetTime();
    double current = 0;

    const Buffer vertexBuffer = CreateBuffer(
        app,
        sizeof(Vertex) * 4,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    const Buffer indexBuffer = CreateBuffer(
        app,
        sizeof(u16) * _countof(indices),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    const VkDeviceSize stagingBuffSize = vertexBuffer.size > indexBuffer.size ? vertexBuffer.size : indexBuffer.size ;

    const Buffer stagingBuffer = CreateBuffer(
        app,
        stagingBuffSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );

    UploadBuffer(app, &stagingBuffer, vertices, vertexBuffer.size);
    CopyBuffer(app, &stagingBuffer, &vertexBuffer, vertexBuffer.size);

    UploadBuffer(app, &stagingBuffer, indices, indexBuffer.size);
    CopyBuffer(app, &stagingBuffer, &indexBuffer, indexBuffer.size);

    DestroyBuffer(app, stagingBuffer);

    float f = 100;
    float n = 0.1f;
    float aspect = (float)app->window.swapChainExtent.width / (float)app->window.swapChainExtent.height;

    Matrix proj = {
        .m0 = f/aspect, .m4 = 0, .m8 = 0, .m12 = 0,
        .m1 = 0, .m5 = f, .m9 = 0, .m13 = 0,
        .m2 = 0, .m6 = 0, .m10 = -f/(f-n), .m14 = -(f*n)/(f-n),
        .m3 = 0, .m7 = 0, .m11 = -1, .m15 = 0
    };

    proj = MatrixIdentity();

    UBO ubo = {
        .model = MatrixIdentity(),
        .view = MatrixLookAt(
            (Vector3){0, 0, 0},
            (Vector3){0, 0, 1},
            (Vector3){0, 1, 0}
        ),
        .projection = proj
    };

    double dt = 0;

    float test = 0;

    while (!glfwWindowShouldClose(app->window.glfwWindow)) {
        TracyCZoneN(const ctx, "Frame", true);
        TracyCZoneN(const draw, "Draw", true);
        TracyCZoneN(const begin, "Begin Frame", true);
        VkCommandBuffer cmdBuffer = BeginFrame(app);
        TracyCZoneEnd(begin);

        BeginRenderPass(app, cmdBuffer);

        TracyCZoneN(const tri, "Draw Triangle", true);

        memcpy(app->window.mappedUniformBuffers[app->window.frameInFlightIndex], &ubo, sizeof(UBO));

        VkDeviceSize offset = 0;

        vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer.handle, &offset);
        vkCmdBindIndexBuffer(cmdBuffer,  indexBuffer.handle, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(
            cmdBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            app->pipelineLayout,
            0,
            1,
            &app->window.descriptorSet[app->window.frameInFlightIndex],
            0,
            nullptr
        );

        vkCmdDrawIndexed(cmdBuffer, _countof(indices), 1, 0, 0, 0);

        TracyCZoneEnd(tri);

        TracyCZoneN(const end, "End Frame", true);
        EndFrame(app);
        TracyCZoneEnd(end);
        TracyCZoneEnd(draw);

        TracyCZoneN(const events, "Poll Events", true);
        glfwPollEvents();
        TracyCZoneEnd(events);

        TracyCZoneN(const gdt, "Get Frame Time", true);
        current = GetTime();
        dt = current - previous;
        previous = current;
        TracyCZoneEnd(gdt);

        // nice quick exit feature
        if (glfwGetKey(app->window.glfwWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(app->window.glfwWindow, GLFW_TRUE);
        }
        TracyCZoneEnd(ctx);
    }
    vkDeviceWaitIdle(app->device);

    DestroyBuffer(app, vertexBuffer);
    DestroyBuffer(app, indexBuffer);
}