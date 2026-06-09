#include "TaskManager.h"
#include <imgui.h>

namespace apps {

TaskManager::TaskManager() : compositor::Window("Task Manager") {
    processes_ = {
        { "csopesy.exe",     12.4f,  128, "Running" },
        { "compositor.exe",   3.1f,   64, "Running" },
        { "desktop.exe",      0.8f,   32, "Running" },
        { "taskbar.exe",      0.3f,   16, "Running" },
        { "clock.exe",        0.1f,    8, "Running" },
        { "fileexplorer.exe", 1.5f,   48, "Running" },
        { "sysinfo.exe",      0.6f,   24, "Running" },
        { "kernel.exe",       5.2f,  256, "Running" },
        { "memmanager.exe",   2.0f,  512, "Running" },
        { "ioscheduler.exe",  1.1f,   40, "Running" },
        { "netdriver.exe",    0.4f,   20, "Sleeping"},
        { "audiomix.exe",     0.2f,   18, "Sleeping"},
        { "wallpaper.exe",    0.0f,    4, "Idle"    },
    };
}

void TaskManager::draw() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowSize({600, 380}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(
        {io.DisplaySize.x * 0.5f - 300, io.DisplaySize.y * 0.5f - 190},
        ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(title_.c_str(), &open_)) { ImGui::End(); return; }

    // Header bar
    ImGui::TextColored({0.4f,0.8f,1.0f,1.0f}, "CSOPESY Task Manager");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 80);
    ImGui::TextDisabled("Processes: %d", (int)processes_.size());
    ImGui::Separator();

    // Tabs (Phase 3 fill-in: only Processes tab active now)
    if (ImGui::BeginTabBar("tmtabs")) {
        if (ImGui::BeginTabItem("Processes")) {
            constexpr ImGuiTableFlags tflags =
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV |
                ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                ImGuiTableFlags_SizingStretchProp;

            if (ImGui::BeginTable("procs", 4, tflags, {0, 260})) {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("Process",    ImGuiTableColumnFlags_WidthStretch, 2.0f);
                ImGui::TableSetupColumn("CPU %",      ImGuiTableColumnFlags_WidthStretch, 1.0f);
                ImGui::TableSetupColumn("Memory MB",  ImGuiTableColumnFlags_WidthStretch, 1.0f);
                ImGui::TableSetupColumn("Status",     ImGuiTableColumnFlags_WidthStretch, 1.0f);
                ImGui::TableHeadersRow();

                for (auto& p : processes_) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(p.name.c_str());
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.1f", p.cpu);
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%d", p.memory);
                    ImGui::TableSetColumnIndex(3);
                    if (p.status == "Running")
                        ImGui::TextColored({0.3f,1.0f,0.4f,1.0f}, "%s", p.status.c_str());
                    else
                        ImGui::TextDisabled("%s", p.status.c_str());
                }
                ImGui::EndTable();
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Performance")) {
            ImGui::TextDisabled("(Phase 3 — coming soon)");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

} // namespace apps
