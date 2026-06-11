#include "SystemInfoApp.h"
#include <imgui.h>
#include <cmath>
#include <cstdio>

namespace apps {

SystemInfoApp::SystemInfoApp() : compositor::Window("System Information") {}

void SystemInfoApp::draw() {
    if (focusRequested_) {
        ImGui::SetNextWindowFocus();
        focusRequested_ = false;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowSize({440, 360}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(
        {io.DisplaySize.x * 0.55f, io.DisplaySize.y * 0.2f},
        ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(title_.c_str(), &open_)) {
        focused_ = false;
        ImGui::End();
        return;
    }
    focused_ = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    // Accumulate time for subtle animation
    static float t = 0.0f;
    t += io.DeltaTime;

    // Gently animated CPU/mem values so the screen feels live
    float cpuVal = 0.28f + 0.06f * std::sin(t * 0.8f) + 0.02f * std::sin(t * 2.3f);
    float memVal = 0.61f + 0.02f * std::sin(t * 0.35f);

    // ── OS card ───────────────────────────────────────────────────────────
    ImGui::TextColored({0.4f,0.8f,1.0f,1.0f}, "CSOPESY Operating System");
    ImGui::TextDisabled("Version 1.0.0  |  Build 2026.06.09");
    ImGui::Separator();

    // ── Hardware meters ───────────────────────────────────────────────────
    ImGui::Spacing();
    ImGui::Text("CPU Usage");
    char cpuLabel[24];
    snprintf(cpuLabel, sizeof(cpuLabel), "%.0f%%", cpuVal * 100.0f);
    ImGui::SameLine(120); ImGui::ProgressBar(cpuVal, {-1, 0}, cpuLabel);

    ImGui::Text("Memory");
    char memLabel[32];
    float memGB = memVal * 8.0f;
    snprintf(memLabel, sizeof(memLabel), "%.1f / 8.0 GB", memGB);
    ImGui::SameLine(120); ImGui::ProgressBar(memVal, {-1, 0}, memLabel);

    ImGui::Text("Disk (C:)");
    ImGui::SameLine(120); ImGui::ProgressBar(0.45f, {-1, 0}, "112 / 256 GB");

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

    // ── Sound ─────────────────────────────────────────────────────────────
    ImGui::TextColored({0.4f,0.8f,1.0f,1.0f}, "Audio");
    ImGui::Text("Volume:");     ImGui::SameLine(120);
    static float vol = 0.72f;
    ImGui::SetNextItemWidth(140);
    ImGui::SliderFloat("##vol", &vol, 0.0f, 1.0f, "%.0f%%");

    ImGui::End();
}

} // namespace apps
