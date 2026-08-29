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

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobj_loader_c.h"

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

// Load text data from file, returns a '\0' terminated string
// NOTE: text chars array should be freed manually
char *LoadFileText(const char *fileName, size_t *count)
{
    char *text = NULL;

    if (fileName != NULL)
    {
        FILE *file = fopen(fileName, "rt");

        if (file != NULL)
        {
            // WARNING: When reading a file as 'text' file,
            // text mode causes carriage return-linefeed translation...
            // ...but using fseek() should return correct byte-offset
            fseek(file, 0, SEEK_END);
            unsigned int size = (unsigned int)ftell(file);
            fseek(file, 0, SEEK_SET);

            if (size > 0)
            {
                text = (char *)calloc(size + 1, sizeof(char));

                if (text != NULL)
                {
                    unsigned int _count = (unsigned int)fread(text, sizeof(char), size, file);

                    // WARNING: \r\n is converted to \n on reading, so,
                    // read bytes count gets reduced by the number of lines
                    if (_count < size) text = (char *)realloc(text, _count + 1);

                    // Zero-terminate the string
                    text[_count] = '\0';

                    if (count != nullptr) *count = _count;

                    TraceLog(LOG_INFO, "FILEIO: [%s] Text file loaded successfully", fileName);
                }
                else TraceLog(LOG_WARNING, "FILEIO: [%s] Failed to allocated memory for file reading", fileName);
            }
            else TraceLog(LOG_WARNING, "FILEIO: [%s] Failed to read text file", fileName);

            fclose(file);
        }
        else TraceLog(LOG_WARNING, "FILEIO: [%s] Failed to open text file", fileName);
    }
    else TraceLog(LOG_WARNING, "FILEIO: File name provided is not valid");

    return text;
}

void MainLoop(App *app) {
    double previous = GetTime();
    double current = 0;

    tinyobj_attrib_t objAttributes = { 0 };
    tinyobj_shape_t *objShapes = nullptr;
    unsigned int objShapeCount = 0;

    tinyobj_material_t *objMaterials = nullptr;
    unsigned int objMaterialCount = 0;

    size_t count;
    char *text = LoadFileText("cube.obj", &count);

    unsigned int flags = TINYOBJ_FLAG_TRIANGULATE;
    int ret = tinyobj_parse_obj(&objAttributes, &objShapes, &objShapeCount, &objMaterials, &objMaterialCount, text, (unsigned int)count, flags);

    free(text);

    Vertex *vertices;
    u16 *indices;

    if (ret != TINYOBJ_SUCCESS) goto quit;

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

    constexpr float fov = 45.0f * DEG2RAD;
    const float aspect =
        (float)app->window.swapChainExtent.width /
        (float)app->window.swapChainExtent.height;

    const Matrix proj = MatrixPerspective(
        fov,
        aspect,
        0.1f,
        1000.0f
    );

    Vector3 pos = {2, -2, 2};

    UBO ubo = {
        .view = MatrixLookAt(
            pos,
            (Vector3){0, 0, 0},
            (Vector3){0, 1, 0}
        ),
        .projection = proj
    };

    memcpy(app->window.mappedUniformBuffers[app->window.frameInFlightIndex], &ubo, sizeof(UBO));

    double dt = 0;

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
            &app->window.descriptorSets[app->window.frameInFlightIndex],
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
    quit:

    vkDeviceWaitIdle(app->device);

    DestroyBuffer(app, vertexBuffer);
    DestroyBuffer(app, indexBuffer);
}