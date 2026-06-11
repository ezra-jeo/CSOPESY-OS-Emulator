#include "TaskManager.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>

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
    // Pre-fill history buffers with zeroes — they'll fill in naturally
}

void TaskManager::draw() {
    if (focusRequested_) {
        ImGui::SetNextWindowFocus();
        focusRequested_ = false;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowSize({620, 400}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(
        {io.DisplaySize.x * 0.5f - 310, io.DisplaySize.y * 0.5f - 200},
        ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(title_.c_str(), &open_)) {
        focused_ = false;
        ImGui::End();
        return;
    }
    focused_ = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    // Accumulate dt and update performance history
    float dt = io.DeltaTime;
    simTime_ += dt;
    plotAccum_ += dt;
    if (plotAccum_ >= 0.1f) {
        plotAccum_ = 0.0f;
        // Simulate gently varying CPU/memory
        float cpu = 28.0f + 8.0f * std::sin(simTime_ * 0.7f) + 4.0f * std::sin(simTime_ * 1.9f);
        float mem = 61.0f + 3.0f * std::sin(simTime_ * 0.3f);
        cpuHist_[histOffset_] = cpu;
        memHist_[histOffset_] = mem;
        histOffset_ = (histOffset_ + 1) % 90;
    }

    ImGui::TextColored({0.4f,0.8f,1.0f,1.0f}, "CSOPESY Task Manager");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 80);
    ImGui::TextDisabled("Processes: %d", (int)processes_.size());
    ImGui::Separator();

    if (ImGui::BeginTabBar("tmtabs")) {
        if (ImGui::BeginTabItem("Processes")) {
            drawProcesses();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Performance")) {
            drawPerformance();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void TaskManager::drawProcesses() {
    constexpr ImGuiTableFlags tflags =
        ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Sortable;

    if (ImGui::BeginTable("procs", 4, tflags, {0, 270})) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Process",   ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthStretch, 2.0f, 0);
        ImGui::TableSetupColumn("CPU %",     ImGuiTableColumnFlags_WidthStretch, 1.0f, 1);
        ImGui::TableSetupColumn("Memory MB", ImGuiTableColumnFlags_WidthStretch, 1.0f, 2);
        ImGui::TableSetupColumn("Status",    ImGuiTableColumnFlags_WidthStretch, 1.0f, 3);
        ImGui::TableHeadersRow();

        // Sort if dirty
        if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs()) {
            if (specs->SpecsDirty && !processes_.empty()) {
                const ImGuiTableColumnSortSpecs& col = specs->Specs[0];
                bool asc = (col.SortDirection == ImGuiSortDirection_Ascending);
                switch (col.ColumnUserID) {
                case 0: std::sort(processes_.begin(), processes_.end(),
                            [asc](const ProcessRow& a, const ProcessRow& b){
                                return asc ? a.name < b.name : a.name > b.name; });
                        break;
                case 1: std::sort(processes_.begin(), processes_.end(),
                            [asc](const ProcessRow& a, const ProcessRow& b){
                                return asc ? a.cpu < b.cpu : a.cpu > b.cpu; });
                        break;
                case 2: std::sort(processes_.begin(), processes_.end(),
                            [asc](const ProcessRow& a, const ProcessRow& b){
                                return asc ? a.memory < b.memory : a.memory > b.memory; });
                        break;
                case 3: std::sort(processes_.begin(), processes_.end(),
                            [asc](const ProcessRow& a, const ProcessRow& b){
                                return asc ? a.status < b.status : a.status > b.status; });
                        break;
                }
                specs->SpecsDirty = false;
                selectedRow_ = -1; // selection invalidated after sort
            }
        }

        int eraseIdx = -1;
        for (int i = 0; i < (int)processes_.size(); ++i) {
            const auto& p = processes_[i];
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);

            bool sel = (selectedRow_ == i);
            ImGui::PushID(i);
            if (ImGui::Selectable(p.name.c_str(), sel,
                    ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap,
                    {0, 0}))
                selectedRow_ = sel ? -1 : i;
            ImGui::PopID();

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

        // End Task button — outside the table, after EndTable
        bool hasSelection = (selectedRow_ >= 0 && selectedRow_ < (int)processes_.size());
        if (!hasSelection) ImGui::BeginDisabled();
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.55f,0.10f,0.10f,0.90f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f,0.18f,0.18f,1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.00f,0.28f,0.28f,1.00f));
        if (ImGui::Button("End Task") && hasSelection)
            eraseIdx = selectedRow_;
        ImGui::PopStyleColor(3);
        if (!hasSelection) ImGui::EndDisabled();

        if (eraseIdx >= 0) {
            processes_.erase(processes_.begin() + eraseIdx);
            selectedRow_ = -1;
        }
    }
}

void TaskManager::drawPerformance() {
    ImGui::Spacing();

    char overlay[32];
    snprintf(overlay, sizeof(overlay), "%.1f%%", cpuHist_[(histOffset_ + 89) % 90]);
    ImGui::TextColored({0.4f,0.8f,1.0f,1.0f}, "CPU Usage");
    ImGui::PlotLines("##cpu", cpuHist_, 90, histOffset_,
                     overlay, 0.0f, 100.0f, {-1, 80});

    ImGui::Spacing();

    snprintf(overlay, sizeof(overlay), "%.1f%%", memHist_[(histOffset_ + 89) % 90]);
    ImGui::TextColored({0.4f,0.8f,1.0f,1.0f}, "Memory Usage");
    ImGui::PlotLines("##mem", memHist_, 90, histOffset_,
                     overlay, 0.0f, 100.0f, {-1, 80});

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextDisabled("System: CSOPESY-X86 @ 3.60 GHz  |  RAM: 8 GB");
}

} // namespace apps
