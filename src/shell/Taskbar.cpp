#include "Taskbar.h"
#include "core/Application.h"
#include "core/Clock.h"
#include "apps/TaskManager.h"
#include "apps/FileExplorerApp.h"
#include "apps/SystemInfoApp.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace shell {

static constexpr float kHeight = 42.0f;

static void pushButtonColors(bool active) {
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.25f,0.55f,1.0f,1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f,0.65f,1.0f,1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.45f,0.75f,1.0f,1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f,0.25f,0.5f,0.8f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f,0.45f,0.8f,1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.35f,0.55f,1.0f,1.0f));
    }
}

void Taskbar::draw(core::Application& app,
                   apps::TaskManager&     taskMgr,
                   apps::FileExplorerApp& fileExp,
                   apps::SystemInfoApp&   sysInfo)
{
    ImGuiIO& io = ImGui::GetIO();
    float W = io.DisplaySize.x;
    float H = io.DisplaySize.y;

    ImGui::SetNextWindowPos({0, H - kHeight});
    ImGui::SetNextWindowSize({W, kHeight});
    ImGui::SetNextWindowBgAlpha(0.88f);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration  |
        ImGuiWindowFlags_NoMove        |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoNav;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.15f, 0.92f));
    ImGui::Begin("##taskbar", nullptr, flags);
    ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
    ImGui::PopStyleColor();

    ImGui::SetCursorPosY((kHeight - ImGui::GetFrameHeight()) * 0.5f);

    // ── Left cluster: app launcher buttons (highlighted when focused) ─────
    pushButtonColors(fileExp.isFocused());
    if (ImGui::Button(" [F] Files "))   fileExp.isOpen() ? fileExp.requestFocus() : fileExp.toggle();
    ImGui::PopStyleColor(3);
    ImGui::SameLine(0, 4);

    pushButtonColors(sysInfo.isFocused());
    if (ImGui::Button(" [I] Sys Info ")) sysInfo.isOpen() ? sysInfo.requestFocus() : sysInfo.toggle();
    ImGui::PopStyleColor(3);
    ImGui::SameLine(0, 4);

    pushButtonColors(taskMgr.isFocused());
    if (ImGui::Button(" [T] Tasks "))   taskMgr.isOpen() ? taskMgr.requestFocus() : taskMgr.toggle();
    ImGui::PopStyleColor(3);

    // ── Right cluster: clock + PWR ────────────────────────────────────────
    std::string ts = core::now();
    float tsz = ImGui::CalcTextSize(ts.c_str()).x;
    float pwrSz = ImGui::CalcTextSize(" PWR ").x + ImGui::GetStyle().FramePadding.x * 2;
    float rightX = W - tsz - pwrSz - 24.0f;

    ImGui::SameLine(rightX);
    ImGui::TextColored({0.7f, 0.9f, 1.0f, 1.0f}, "%s", ts.c_str());
    ImGui::SameLine(0, 12);

    // ── PWR Button ───────────────────────────────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f,0.1f,0.1f,0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f,0.2f,0.2f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.0f,0.3f,0.3f,1.0f));
    if (ImGui::Button(" PWR "))
        ImGui::OpenPopup("##pwr_confirm");
    ImGui::PopStyleColor(3);

    // Confirmation Modal
    ImGui::SetNextWindowPos({W * 0.5f, H * 0.5f}, ImGuiCond_Always, {0.5f, 0.5f});
    if (ImGui::BeginPopupModal("##pwr_confirm", nullptr,
            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::TextColored({1.0f,0.4f,0.4f,1.0f}, "Shut down CSOPESY?");
        ImGui::Spacing();
        ImGui::SetNextItemWidth(120);
        if (ImGui::Button("Confirm", {120, 0})) {
            ImGui::CloseCurrentPopup();
            app.requestQuit();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", {120, 0}))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::End();
}

} // namespace shell
