#pragma once

namespace core  { class Application; }
namespace apps  { class TaskManager; class FileExplorerApp; class SystemInfoApp; }

namespace shell {

struct Taskbar {
    static void draw(core::Application& app,
                     apps::TaskManager&    taskMgr,
                     apps::FileExplorerApp& fileExp,
                     apps::SystemInfoApp&  sysInfo);
};

} // namespace shell
