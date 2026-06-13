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
static constexpr float kBtnW   = 44.0f;
static constexpr float kBtnH   = 28.0f;

// ── Button colour helpers ─────────────────────────────────────────────────────

static void pushButtonColors(bool active) {
    if (active) {
        // XP active: bright royal blue
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.231f, 0.467f, 0.831f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.310f, 0.549f, 0.894f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.141f, 0.365f, 0.710f, 1.0f));
    } else {
        // XP inactive: darker navy
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.122f, 0.255f, 0.522f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.192f, 0.353f, 0.647f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.255f, 0.443f, 0.753f, 1.0f));
    }
}

// ── Icon draw functions (ImDrawList, centred on c) ────────────────────────────

// Folder icon — manila yellow rectangle with a small top-left tab
static void drawFolder(ImDrawList* dl, ImVec2 c) {
    const ImU32 col = IM_COL32(240, 200, 70, 245);
    dl->AddRectFilled({c.x - 9.0f, c.y - 6.0f}, {c.x - 0.5f, c.y - 2.5f}, col, 1.0f); // tab
    dl->AddRectFilled({c.x - 9.0f, c.y - 4.5f}, {c.x + 9.0f, c.y + 6.0f}, col, 1.0f); // body
}

// Info icon — blue circle with white dot + stem forming an "i"
static void drawInfo(ImDrawList* dl, ImVec2 c) {
    dl->AddCircleFilled(c, 8.0f, IM_COL32(25, 110, 215, 245));
    dl->AddCircle(c, 8.0f, IM_COL32(160, 210, 255, 200));
    dl->AddCircleFilled({c.x, c.y - 3.5f}, 1.2f, IM_COL32(255, 255, 255, 255)); // dot
    dl->AddRectFilled({c.x - 1.2f, c.y - 0.5f},
                      {c.x + 1.2f, c.y + 4.0f},
                      IM_COL32(255, 255, 255, 255));                              // stem
}

// Task-list icon — window frame with three horizontal lines
static void drawTaskList(ImDrawList* dl, ImVec2 c) {
    const ImU32 col = IM_COL32(160, 210, 255, 230);
    dl->AddRect({c.x - 9.0f, c.y - 7.0f}, {c.x + 9.0f, c.y + 7.0f}, col, 1.0f);
    for (int i = 0; i < 3; ++i) {
        float y = c.y - 3.0f + i * 3.5f;
        dl->AddRectFilled({c.x - 6.0f, y}, {c.x + 6.0f, y + 1.5f}, col);
    }
}

// ── Combined icon button ──────────────────────────────────────────────────────

static bool iconButton(const char* id, bool active,
                       void (*drawIcon)(ImDrawList*, ImVec2))
{
    pushButtonColors(active);
    bool clicked = ImGui::Button(id, {kBtnW, kBtnH});
    ImGui::PopStyleColor(3);

    ImVec2 bmin   = ImGui::GetItemRectMin();
    ImVec2 bmax   = ImGui::GetItemRectMax();
    ImVec2 center = {(bmin.x + bmax.x) * 0.5f, (bmin.y + bmax.y) * 0.5f};
    drawIcon(ImGui::GetWindowDrawList(), center);
    return clicked;
}

// ── Public draw ───────────────────────────────────────────────────────────────

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
    ImGui::SetNextWindowBgAlpha(0.95f);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration  |
        ImGuiWindowFlags_NoMove        |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoNav;

    // XP taskbar blue — approximately #1F3C78
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.122f, 0.235f, 0.471f, 0.97f));
    ImGui::Begin("##taskbar", nullptr, flags);
    ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
    ImGui::PopStyleColor();

    // Centre buttons vertically
    ImGui::SetCursorPosY((kHeight - kBtnH) * 0.5f);

    // ── Left cluster: icon launcher buttons ───────────────────────────────────
    if (iconButton("##files_btn", fileExp.isFocused(), drawFolder))
        fileExp.isOpen() ? fileExp.requestFocus() : fileExp.toggle();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        ImGui::SetTooltip("File Explorer");

    ImGui::SameLine(0, 6);

    if (iconButton("##sysinfo_btn", sysInfo.isFocused(), drawInfo))
        sysInfo.isOpen() ? sysInfo.requestFocus() : sysInfo.toggle();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        ImGui::SetTooltip("System Info");

    ImGui::SameLine(0, 6);

    if (iconButton("##tasks_btn", taskMgr.isFocused(), drawTaskList))
        taskMgr.isOpen() ? taskMgr.requestFocus() : taskMgr.toggle();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        ImGui::SetTooltip("Task Manager");

    // ── Right cluster: clock + PWR ────────────────────────────────────────────
    std::string ts = core::now();
    float tsz      = ImGui::CalcTextSize(ts.c_str()).x;
    float pwrSz    = ImGui::CalcTextSize(" PWR ").x
                     + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SameLine(W - tsz - pwrSz - 24.0f);
    ImGui::SetCursorPosY((kHeight - ImGui::GetTextLineHeight()) * 0.5f);
    ImGui::TextColored({0.75f, 0.90f, 1.0f, 1.0f}, "%s", ts.c_str());

    ImGui::SameLine(0, 12);
    ImGui::SetCursorPosY((kHeight - kBtnH) * 0.5f);
    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.60f, 0.10f, 0.10f, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.20f, 0.20f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1.00f, 0.30f, 0.30f, 1.00f));
    if (ImGui::Button(" PWR ", {0, kBtnH}))
        app.requestQuit();
    ImGui::PopStyleColor(3);

    ImGui::End();
}

} // namespace shell
