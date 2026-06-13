#include "SystemInfoApp.h"
#include <imgui.h>
#include <cmath>
#include <cstdio>

namespace apps {

SystemInfoApp::SystemInfoApp() : compositor::Window("System Information") {}

void SystemInfoApp::draw() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowSize({440, 340}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(
        {io.DisplaySize.x * 0.55f, io.DisplaySize.y * 0.2f},
        ImGuiCond_FirstUseEver);

    if (focusRequested_) { ImGui::SetNextWindowFocus(); focusRequested_ = false; }
    if (!ImGui::Begin(title_.c_str(), &open_)) { focused_ = false; ImGui::End(); return; }
    focused_ = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    // ── OS card ───────────────────────────────────────────────────────────
    ImGui::TextColored({0.4f,0.8f,1.0f,1.0f}, "CSOPESY Operating System");
    ImGui::TextDisabled("Version 1.0.0  |  Build 2026.06.09");
    ImGui::Separator();

    // ── Hardware meters (animated) ────────────────────────────────────────
    float t = (float)ImGui::GetTime();
    float cpu = 0.22f + 0.08f * sinf(t * 0.7f);
    float mem = 0.58f + 0.05f * sinf(t * 0.4f + 1.0f);
    float dsk = 0.45f;

    char cpuLabel[24], memLabel[24];
    snprintf(cpuLabel, sizeof(cpuLabel), "%.0f%%", cpu * 100.0f);
    snprintf(memLabel, sizeof(memLabel), "%.1f / 8.0 GB", mem * 8.0f);

    ImGui::Spacing();
    ImGui::Text("CPU Usage");
    ImGui::SameLine(120); ImGui::ProgressBar(cpu, {-1, 0}, cpuLabel);

    ImGui::Text("Memory");
    ImGui::SameLine(120); ImGui::ProgressBar(mem, {-1, 0}, memLabel);

    ImGui::Text("Disk (C:)");
    ImGui::SameLine(120); ImGui::ProgressBar(dsk, {-1, 0}, "112 / 256 GB");

    ImGui::Spacing();
    ImGui::Separator();

    // ── Network ───────────────────────────────────────────────────────────
    ImGui::TextColored({0.4f,0.8f,1.0f,1.0f}, "Network");
    ImGui::Text("Adapter:");    ImGui::SameLine(120); ImGui::TextDisabled("Ethernet");
    ImGui::Text("IP Address:"); ImGui::SameLine(120); ImGui::TextDisabled("192.168.1.42");
    ImGui::Text("Status:");     ImGui::SameLine(120);
    ImGui::TextColored({0.3f,1.0f,0.4f,1.0f}, "Connected");
    ImGui::Text("Speed:");      ImGui::SameLine(120); ImGui::TextDisabled("1 Gbps");

    ImGui::Spacing();
    ImGui::Separator();

    // ── Audio ─────────────────────────────────────────────────────────────
    ImGui::TextColored({0.4f,0.8f,1.0f,1.0f}, "Audio");
    ImGui::Text("Volume:");     ImGui::SameLine(120);
    ImGui::SetNextItemWidth(140);
    ImGui::SliderFloat("##vol", &vol_, 0.0f, 100.0f, "%.0f%%");

    ImGui::End();
}

} // namespace apps
