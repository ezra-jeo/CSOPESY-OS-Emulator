#include "Compositor.h"

#include "core/Application.h"
#include "shell/Desktop.h"
#include "shell/Taskbar.h"
#include "apps/TaskManager.h"
#include "apps/FileExplorerApp.h"
#include "apps/SystemInfoApp.h"

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <memory>

namespace compositor {

Compositor::Compositor(core::Application& app) : app_(app) {
    taskManager_  = wm_.add<apps::TaskManager>();
    fileExplorer_ = wm_.add<apps::FileExplorerApp>();
    sysInfo_      = wm_.add<apps::SystemInfoApp>();
}

Compositor::~Compositor() = default;

void Compositor::render() {
    // Layer 1: Desktop (fullscreen base — must be drawn first)
    shell::Desktop::draw(app_);

    // Layer 2: Floating app windows
    wm_.drawWindows();

    // Layer 3: Taskbar (drawn last so it sits on top of everything)
    shell::Taskbar::draw(app_, *taskManager_, *fileExplorer_, *sysInfo_);
}

} // namespace compositor
