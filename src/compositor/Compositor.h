#pragma once
#include "WindowManager.h"
#include "shell/BootSequence.h"

namespace core { class Application; }
namespace apps  { class TaskManager; class FileExplorerApp; class SystemInfoApp; }

namespace compositor {

class Compositor {
public:
    explicit Compositor(core::Application& app);
    ~Compositor();

    void render(float dt);

private:
    core::Application& app_;
    WindowManager      wm_;
    shell::BootSequence boot_;

    // Non-owning pointers into wm_ (wm_ owns them via unique_ptr)
    apps::TaskManager*     taskManager_{ nullptr };
    apps::FileExplorerApp* fileExplorer_{ nullptr };
    apps::SystemInfoApp*   sysInfo_{ nullptr };
};

} // namespace compositor
