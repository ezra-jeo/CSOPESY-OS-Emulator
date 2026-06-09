#pragma once
#include "WindowManager.h"

namespace core { class Application; }
namespace shell { class Desktop; class Taskbar; }
namespace apps  { class TaskManager; class FileExplorerApp; class SystemInfoApp; }

namespace compositor {

class Compositor {
public:
    explicit Compositor(core::Application& app);
    ~Compositor();

    // Called once per frame inside the ImGui frame.
    void render();

private:
    core::Application& app_;
    WindowManager      wm_;

    // Non-owning pointers into wm_ (wm_ owns them via unique_ptr)
    apps::TaskManager*    taskManager_{ nullptr };
    apps::FileExplorerApp* fileExplorer_{ nullptr };
    apps::SystemInfoApp*  sysInfo_{ nullptr };
};

} // namespace compositor
