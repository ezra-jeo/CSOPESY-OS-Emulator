#include "Compositor.h"

#include "core/Application.h"
#include "shell/Desktop.h"
#include "shell/Taskbar.h"
#include "apps/TaskManager.h"
#include "apps/FileExplorerApp.h"
#include "apps/SystemInfoApp.h"

namespace compositor {

Compositor::Compositor(core::Application& app) : app_(app) {
    taskManager_  = wm_.add<apps::TaskManager>();
    fileExplorer_ = wm_.add<apps::FileExplorerApp>();
    sysInfo_      = wm_.add<apps::SystemInfoApp>();

    // Loaded here (GL context is current) and owned for the Compositor's
    // lifetime, which ends inside Application::run() before glfwTerminate().
    wallpaper_ = core::loadTexture("assets/wallpapers/wallpaper.jpg");
}

Compositor::~Compositor() = default;

void Compositor::render(float dt) {
    if (!boot_.isDone()) {
        boot_.update(dt);
        boot_.draw();
        return;
    }

    // Layer 1: Desktop (fullscreen base — must be drawn first)
    shell::Desktop::draw(wallpaper_);

    // Layer 2: Floating app windows
    wm_.drawWindows();

    // Layer 3: Taskbar (drawn last so it sits on top of everything)
    shell::Taskbar::draw(app_, *taskManager_, *fileExplorer_, *sysInfo_);
}

} // namespace compositor
