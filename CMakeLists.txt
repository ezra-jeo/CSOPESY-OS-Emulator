cmake_minimum_required(VERSION 3.16)
project(csopesy VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Dependencies via FetchContent ──────────────────────────────────────────────
include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.6
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        013ac3beddff3dbffafd5177e7972067cd2b5083
    GIT_SHALLOW    FALSE
)
FetchContent_MakeAvailable(stb)

# Build Dear ImGui as a static library
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)

# ── OpenGL ─────────────────────────────────────────────────────────────────────
find_package(OpenGL REQUIRED)

# ── Application sources ────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/core/Application.cpp
    src/core/Clock.cpp
    src/core/Texture.cpp
    src/compositor/Compositor.cpp
    src/compositor/WindowManager.cpp
    src/shell/Desktop.cpp
    src/shell/Taskbar.cpp
    src/shell/BootSequence.cpp
    src/apps/TaskManager.cpp
    src/apps/FileExplorerApp.cpp
    src/apps/SystemInfoApp.cpp
)

add_executable(csopesy ${SOURCES})

target_include_directories(csopesy PRIVATE src ${stb_SOURCE_DIR})
target_link_libraries(csopesy PRIVATE imgui OpenGL::GL glfw)

# Copy assets next to the executable
add_custom_command(TARGET csopesy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:csopesy>/assets"
)
