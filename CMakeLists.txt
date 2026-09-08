cmake_minimum_required(VERSION 4.2)
project(footie_life C CXX)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 17)

set(VULKAN_SDK_VER "1.4.357.0")

include_directories(dependencies/clib/include)
link_directories(dependencies/clib/bin)

include_directories(dependencies/glfw)
link_directories(dependencies/glfw)

include_directories(C:/VulkanSDK/1.4.357.0/include)
link_directories(dependencies/vulkan)

add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)

file(GLOB_RECURSE SOURCES
        "src/*.c"
        "src/*.h"
)

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
    add_executable(footie_life WIN32 ${SOURCES})
else()
    add_executable(footie_life
            ${SOURCES}
            dependencies/tracy/TracyClient.cpp
    )

    target_compile_definitions(footie_life PRIVATE
            DEBUG=1
            TRACE_LOG
            TRACY_ENABLE
    )

    target_include_directories(footie_life PRIVATE
            dependencies/tracy
    )
endif()

target_link_libraries(footie_life
        vector
        glfw3
        vulkan-1

        gdi32
        winmm
        ws2_32
        dbghelp
)