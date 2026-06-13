#include "Application.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include "compositor/Compositor.h"
#include "core/Theme.h"

#include <stdexcept>
#include <cstdio>

namespace core {

Application::Application() {
    initGLFW();
    initImGui();
}

Application::~Application() {
    shutdown();
}

void Application::initGLFW() {
    if (!glfwInit())
        throw std::runtime_error("Failed to initialise GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Use primary monitor resolution for a true fullscreen feel
    GLFWmonitor* monitor  = glfwGetPrimaryMonitor();
    const GLFWvidmode* vm = glfwGetVideoMode(monitor);
    int w = vm ? vm->width  : 1280;
    int h = vm ? vm->height : 720;

    window_ = glfwCreateWindow(w, h, "CSOPESY Desktop OS Emulator", monitor, nullptr);
    if (!window_)
        throw std::runtime_error("Failed to create GLFW window");

    // Swallow Alt+F4 / OS close requests — only the PWR button may exit.
    glfwSetWindowCloseCallback(window_, [](GLFWwindow* w) {
        glfwSetWindowShouldClose(w, GLFW_FALSE);
    });

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // vsync
}

void Application::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    core::applyTheme();

    // Extend glyph coverage to include box-drawing (U+2500–U+257F) and
    // block-element (U+2580–U+259F) characters used in the boot splash logo.
    static const ImWchar kGlyphRanges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin-1 Supplement
        0x2500, 0x259F, // Box Drawing + Block Elements
        0,
    };
    ImGuiIO& io2 = ImGui::GetIO();
    if (ImFont* f = io2.Fonts->AddFontFromFileTTF(
            "assets/fonts/LiberationMono-Bold.ttf", 15.0f, nullptr, kGlyphRanges))
        (void)f;

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void Application::run() {
    compositor::Compositor compositor(*this);
    double lastTime = glfwGetTime();

    while (running_ && !glfwWindowShouldClose(window_)) {
        glfwPollEvents();

        double now = glfwGetTime();
        float dt = static_cast<float>(now - lastTime);
        lastTime = now;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        compositor.render(dt);

        ImGui::Render();

        int display_w, display_h;
        glfwGetFramebufferSize(window_, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window_);
    }
}

void Application::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (window_) glfwDestroyWindow(window_);
    glfwTerminate();
}

} // namespace core
