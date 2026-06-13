#include "TaskManager.h"
#include <imgui.h>
#include <algorithm>
#include <numeric>
#include <cmath>

namespace apps {

TaskManager::TaskManager() : compositor::Window("Task Manager") {
    processes_ = {
        { "csopesy.exe",      12.4f,  128, "Running" },
        { "compositor.exe",    3.1f,   64, "Running" },
        { "desktop.exe",       0.8f,   32, "Running" },
        { "taskbar.exe",       0.3f,   16, "Running" },
        { "clock.exe",         0.1f,    8, "Running" },
        { "fileexplorer.exe",  1.5f,   48, "Running" },
        { "sysinfo.exe",       0.6f,   24, "Running" },
        { "kernel.exe",        5.2f,  256, "Running" },
        { "memmanager.exe",    2.0f,  512, "Running" },
        { "ioscheduler.exe",   1.1f,   40, "Running" },
        { "netdriver.exe",     0.4f,   20, "Sleeping"},
        { "audiomix.exe",      0.2f,   18, "Sleeping"},
        { "wallpaper.exe",     0.0f,    4, "Idle"    },
    };
}

void TaskManager::draw() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowSize({620, 420}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(
        {io.DisplaySize.x * 0.5f - 310, io.DisplaySize.y * 0.5f - 210},
        ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(title_.c_str(), &open_)) { ImGui::End(); return; }

    ImGui::TextColored({0.4f,0.8f,1.0f,1.0f}, "CSOPESY Task Manager");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 80);
    ImGui::TextDisabled("Processes: %d", (int)processes_.size());
    ImGui::Separator();

    if (ImGui::BeginTabBar("tmtabs")) {
        // ── Processes tab ─────────────────────────────────────────────────
        if (ImGui::BeginTabItem("Processes")) {
            constexpr ImGuiTableFlags tflags =
                ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV |
                ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Sortable;

            if (ImGui::BeginTable("procs", 4, tflags, {0, 290})) {
                ImGui::TableSetupScrollFreeze(0, 1);
                ImGui::TableSetupColumn("Process",   ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_DefaultSort, 2.0f);
                ImGui::TableSetupColumn("CPU %",     ImGuiTableColumnFlags_WidthStretch, 1.0f);
                ImGui::TableSetupColumn("Memory MB", ImGuiTableColumnFlags_WidthStretch, 1.0f);
                ImGui::TableSetupColumn("Status",    ImGuiTableColumnFlags_WidthStretch, 1.0f);
                ImGui::TableHeadersRow();

                // Sort
                if (ImGuiTableSortSpecs* ss = ImGui::TableGetSortSpecs()) {
                    if (ss->SpecsDirty && !processes_.empty()) {
                        int col = ss->Specs[0].ColumnIndex;
                        bool asc = ss->Specs[0].SortDirection == ImGuiSortDirection_Ascending;
                        std::sort(processes_.begin(), processes_.end(), [col, asc](const ProcessRow& a, const ProcessRow& b) {
                            bool lt = false;
                            switch (col) {
                            case 0: lt = a.name   < b.name;   break;
                            case 1: lt = a.cpu    < b.cpu;    break;
                            case 2: lt = a.memory < b.memory; break;
                            case 3: lt = a.status < b.status; break;
                            }
                            return asc ? lt : !lt;
                        });
                        ss->SpecsDirty = false;
                        selectedRow_ = -1;
                    }
                }

                for (int i = 0; i < (int)processes_.size(); ++i) {
                    const ProcessRow& p = processes_[i];
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    bool sel = (selectedRow_ == i);
                    if (ImGui::Selectable(p.name.c_str(), sel,
                            ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap,
                            {0, 0}))
                        selectedRow_ = sel ? -1 : i;
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

            ImGui::BeginDisabled(selectedRow_ < 0 || selectedRow_ >= (int)processes_.size());
            if (ImGui::Button("End Task") && selectedRow_ >= 0) {
                processes_.erase(processes_.begin() + selectedRow_);
                selectedRow_ = -1;
            }
            ImGui::EndDisabled();

            ImGui::EndTabItem();
        }

        // ── Performance tab ───────────────────────────────────────────────
        if (ImGui::BeginTabItem("Performance")) {
            // Sample CPU/Mem totals at ~10 Hz
            plotAccum_ += io.DeltaTime;
            if (plotAccum_ >= 0.1f) {
                plotAccum_ = 0.0f;
                float totalCpu = 0.0f;
                for (auto& p : processes_) totalCpu += p.cpu;
                cpuHist_[histOffset_] = std::fmin(totalCpu, 100.0f);
                // simulate mem usage oscillating around 62%
                float t = (float)ImGui::GetTime();
                memHist_[histOffset_] = 55.0f + 8.0f * sinf(t * 0.4f);
                histOffset_ = (histOffset_ + 1) % 90;
            }

            ImGui::TextColored({0.4f,0.8f,1.0f,1.0f}, "CPU Usage");
            char cpuLabel[24];
            snprintf(cpuLabel, sizeof(cpuLabel), "%.1f%%", cpuHist_[(histOffset_ + 89) % 90]);
            ImGui::PlotLines("##cpu", cpuHist_, 90, histOffset_, cpuLabel, 0.0f, 100.0f, {-1, 80});

            ImGui::Spacing();
            ImGui::TextColored({0.4f,0.8f,1.0f,1.0f}, "Memory Usage");
            char memLabel[24];
            snprintf(memLabel, sizeof(memLabel), "%.1f%%", memHist_[(histOffset_ + 89) % 90]);
            ImGui::PlotLines("##mem", memHist_, 90, histOffset_, memLabel, 0.0f, 100.0f, {-1, 80});

            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

} // namespace apps
