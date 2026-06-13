#include "Theme.h"
#include <imgui.h>

namespace core {

void applyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();

    s.WindowRounding    = 4.0f;
    s.ChildRounding     = 3.0f;
    s.FrameRounding     = 3.0f;
    s.GrabRounding      = 3.0f;
    s.PopupRounding     = 4.0f;
    s.ScrollbarRounding = 3.0f;
    s.TabRounding       = 3.0f;

    s.WindowPadding  = {10, 8};
    s.FramePadding   = { 6, 3};
    s.ItemSpacing    = { 8, 5};
    s.ScrollbarSize  = 12.0f;

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg]          = {0.06f, 0.08f, 0.16f, 0.96f};
    c[ImGuiCol_ChildBg]           = {0.04f, 0.06f, 0.12f, 0.80f};
    c[ImGuiCol_PopupBg]           = {0.06f, 0.08f, 0.18f, 0.98f};
    c[ImGuiCol_Border]            = {0.20f, 0.35f, 0.60f, 0.60f};
    c[ImGuiCol_FrameBg]           = {0.08f, 0.12f, 0.25f, 0.80f};
    c[ImGuiCol_FrameBgHovered]    = {0.12f, 0.20f, 0.40f, 0.80f};
    c[ImGuiCol_FrameBgActive]     = {0.15f, 0.28f, 0.55f, 0.90f};
    c[ImGuiCol_TitleBg]           = {0.04f, 0.06f, 0.14f, 1.00f};
    c[ImGuiCol_TitleBgActive]     = {0.08f, 0.18f, 0.42f, 1.00f};
    c[ImGuiCol_TitleBgCollapsed]  = {0.04f, 0.06f, 0.14f, 0.80f};
    c[ImGuiCol_MenuBarBg]         = {0.06f, 0.10f, 0.20f, 1.00f};
    c[ImGuiCol_ScrollbarBg]       = {0.02f, 0.04f, 0.10f, 0.60f};
    c[ImGuiCol_ScrollbarGrab]     = {0.20f, 0.35f, 0.65f, 0.80f};
    c[ImGuiCol_ScrollbarGrabHovered] = {0.28f, 0.48f, 0.80f, 1.00f};
    c[ImGuiCol_ScrollbarGrabActive]  = {0.35f, 0.58f, 1.00f, 1.00f};
    c[ImGuiCol_CheckMark]         = {0.40f, 0.80f, 1.00f, 1.00f};
    c[ImGuiCol_SliderGrab]        = {0.28f, 0.55f, 0.90f, 1.00f};
    c[ImGuiCol_SliderGrabActive]  = {0.40f, 0.70f, 1.00f, 1.00f};
    c[ImGuiCol_Button]            = {0.15f, 0.28f, 0.55f, 0.85f};
    c[ImGuiCol_ButtonHovered]     = {0.25f, 0.45f, 0.80f, 1.00f};
    c[ImGuiCol_ButtonActive]      = {0.35f, 0.58f, 1.00f, 1.00f};
    c[ImGuiCol_Header]            = {0.15f, 0.28f, 0.55f, 0.70f};
    c[ImGuiCol_HeaderHovered]     = {0.22f, 0.40f, 0.72f, 0.85f};
    c[ImGuiCol_HeaderActive]      = {0.30f, 0.52f, 0.90f, 1.00f};
    c[ImGuiCol_Separator]         = {0.20f, 0.35f, 0.60f, 0.50f};
    c[ImGuiCol_Tab]               = {0.10f, 0.18f, 0.38f, 0.86f};
    c[ImGuiCol_TabHovered]        = {0.22f, 0.42f, 0.75f, 1.00f};
    c[ImGuiCol_TabActive]         = {0.18f, 0.35f, 0.65f, 1.00f};
    c[ImGuiCol_TabUnfocused]      = {0.06f, 0.10f, 0.22f, 0.90f};
    c[ImGuiCol_TabUnfocusedActive]= {0.12f, 0.22f, 0.45f, 1.00f};
    c[ImGuiCol_TableHeaderBg]     = {0.08f, 0.14f, 0.30f, 1.00f};
    c[ImGuiCol_TableBorderStrong] = {0.20f, 0.35f, 0.60f, 1.00f};
    c[ImGuiCol_TableBorderLight]  = {0.14f, 0.24f, 0.45f, 0.70f};
    c[ImGuiCol_TableRowBg]        = {0.00f, 0.00f, 0.00f, 0.00f};
    c[ImGuiCol_TableRowBgAlt]     = {0.06f, 0.10f, 0.20f, 0.30f};
    c[ImGuiCol_TextSelectedBg]    = {0.25f, 0.50f, 0.85f, 0.35f};
    c[ImGuiCol_NavHighlight]      = {0.40f, 0.70f, 1.00f, 1.00f};
}

} // namespace core
