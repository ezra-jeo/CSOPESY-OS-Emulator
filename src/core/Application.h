#pragma once

struct GLFWwindow;

namespace compositor { class Compositor; }

namespace core {

class Application {
public:
    Application();
    ~Application();

    // Enters the main loop; returns when the user requests quit.
    void run();

    // Called by Desktop / Taskbar PWR button to signal shutdown.
    void requestQuit() { running_ = false; }
    bool isRunning() const { return running_; }

private:
    void initGLFW();
    void initImGui();
    void shutdown();

    GLFWwindow* window_{ nullptr };
    bool        running_{ true };
};

} // namespace core
