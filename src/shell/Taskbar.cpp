#include "Taskbar.h"
#include "core/Application.h"
#include "core/Clock.h"
#include "apps/TaskManager.h"
#include "apps/FileExplorerApp.h"
#include "apps/SystemInfoApp.h"

#include <imgui.h>

namespace shell {

static constexpr float kHeight = 42.0f;

static void appButton(const char* label, compositor::Window& win) {
    bool focused = win.isFocused() && win.isOpen();

    if (focused) {
        // Brighter colour when this app's window has focus
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.28f,0.50f,0.90f,1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.38f,0.62f,1.00f,1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.48f,0.72f,1.00f,1.00f));
    }

    if (ImGui::Button(label))
        win.requestFocus();

    if (focused)
        ImGui::PopStyleColor(3);
}

void Taskbar::draw(core::Application& app,
                   apps::TaskManager&    taskMgr,
                   apps::FileExplorerApp& fileExp,
                   apps::SystemInfoApp&  sysInfo)
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

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.04f, 0.12f, 0.96f));
    ImGui::Begin("##taskbar", nullptr, flags);
    ImGui::PopStyleColor();

    ImGui::SetCursorPosY((kHeight - ImGui::GetFrameHeight()) * 0.5f);

    // ── Left cluster: app launcher buttons ───────────────────────────────
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.14f,0.24f,0.50f,0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f,0.40f,0.78f,1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.32f,0.54f,1.00f,1.00f));

    appButton(" [F] Files ",    fileExp);
    ImGui::SameLine(0, 4);
    appButton(" [I] Sys Info ", sysInfo);
    ImGui::SameLine(0, 4);
    appButton(" [T] Tasks ",    taskMgr);

    ImGui::PopStyleColor(3);

    // ── Right cluster: clock + PWR ────────────────────────────────────────
    std::string ts = core::now();
    float tsz = ImGui::CalcTextSize(ts.c_str()).x;
    float pwrSz = ImGui::CalcTextSize(" PWR ").x + ImGui::GetStyle().FramePadding.x * 2;
    float rightX = W - tsz - pwrSz - 24.0f;

    ImGui::SameLine(rightX);
    ImGui::TextColored({0.7f, 0.9f, 1.0f, 1.0f}, "%s", ts.c_str());
    ImGui::SameLine(0, 12);

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f,0.1f,0.1f,0.9f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f,0.2f,0.2f,1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.0f,0.3f,0.3f,1.0f));
    if (ImGui::Button(" PWR "))
        ImGui::OpenPopup("##tbpwrconfirm");
    ImGui::PopStyleColor(3);

    // PWR confirmation modal (anchored to taskbar)
    ImGui::SetNextWindowPos({W * 0.5f, H - kHeight}, ImGuiCond_Always, {0.5f, 1.0f});
    ImGui::SetNextWindowSize({280, 110}, ImGuiCond_Always);
    if (ImGui::BeginPopupModal("##tbpwrconfirm", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove)) {
        ImGui::Spacing();
        ImGui::SetCursorPosX((280 - ImGui::CalcTextSize("Shut down CSOPESY?").x) * 0.5f);
        ImGui::Text("Shut down CSOPESY?");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        float bw = 90.0f;
        ImGui::SetCursorPosX((280 - bw * 2 - 12) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.6f,0.1f,0.1f,0.9f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f,0.2f,0.2f,1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.0f,0.3f,0.3f,1.0f));
        if (ImGui::Button("Shut Down", {bw, 0})) {
            ImGui::CloseCurrentPopup();
            app.requestQuit();
        }
        ImGui::PopStyleColor(3);
        ImGui::SameLine(0, 12);
        if (ImGui::Button("Cancel", {bw, 0}))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::End();
}

} // namespace shell
