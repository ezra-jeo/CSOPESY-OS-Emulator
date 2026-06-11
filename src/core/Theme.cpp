#include "Theme.h"
#include <imgui.h>

namespace core {

void applyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();

    s.WindowRounding    = 6.0f;
    s.ChildRounding     = 4.0f;
    s.FrameRounding     = 4.0f;
    s.PopupRounding     = 4.0f;
    s.ScrollbarRounding = 4.0f;
    s.GrabRounding      = 4.0f;
    s.TabRounding       = 4.0f;

    s.WindowPadding  = {10.0f, 8.0f};
    s.FramePadding   = { 6.0f, 4.0f};
    s.ItemSpacing    = { 8.0f, 6.0f};
    s.ScrollbarSize  = 12.0f;

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg]          = {0.06f, 0.07f, 0.14f, 0.96f};
    c[ImGuiCol_ChildBg]           = {0.04f, 0.05f, 0.10f, 0.80f};
    c[ImGuiCol_PopupBg]           = {0.06f, 0.07f, 0.14f, 0.98f};

    c[ImGuiCol_TitleBg]           = {0.04f, 0.06f, 0.18f, 1.00f};
    c[ImGuiCol_TitleBgActive]     = {0.08f, 0.14f, 0.38f, 1.00f};
    c[ImGuiCol_TitleBgCollapsed]  = {0.04f, 0.06f, 0.18f, 0.75f};

    c[ImGuiCol_Header]            = {0.12f, 0.22f, 0.50f, 0.80f};
    c[ImGuiCol_HeaderHovered]     = {0.20f, 0.36f, 0.72f, 0.90f};
    c[ImGuiCol_HeaderActive]      = {0.28f, 0.48f, 0.90f, 1.00f};

    c[ImGuiCol_Button]            = {0.14f, 0.24f, 0.52f, 0.85f};
    c[ImGuiCol_ButtonHovered]     = {0.22f, 0.40f, 0.78f, 1.00f};
    c[ImGuiCol_ButtonActive]      = {0.32f, 0.54f, 1.00f, 1.00f};

    c[ImGuiCol_FrameBg]           = {0.08f, 0.10f, 0.22f, 0.80f};
    c[ImGuiCol_FrameBgHovered]    = {0.14f, 0.18f, 0.36f, 0.90f};
    c[ImGuiCol_FrameBgActive]     = {0.20f, 0.26f, 0.50f, 1.00f};

    c[ImGuiCol_Tab]               = {0.10f, 0.16f, 0.36f, 0.86f};
    c[ImGuiCol_TabHovered]        = {0.20f, 0.36f, 0.72f, 1.00f};
    c[ImGuiCol_TabActive]         = {0.16f, 0.30f, 0.62f, 1.00f};
    c[ImGuiCol_TabUnfocused]      = {0.06f, 0.10f, 0.24f, 0.90f};
    c[ImGuiCol_TabUnfocusedActive]= {0.12f, 0.22f, 0.46f, 1.00f};

    c[ImGuiCol_ScrollbarBg]       = {0.02f, 0.02f, 0.06f, 0.60f};
    c[ImGuiCol_ScrollbarGrab]     = {0.20f, 0.30f, 0.60f, 0.80f};
    c[ImGuiCol_ScrollbarGrabHovered]= {0.28f,0.42f, 0.80f, 1.00f};
    c[ImGuiCol_ScrollbarGrabActive] = {0.36f,0.54f, 1.00f, 1.00f};

    c[ImGuiCol_CheckMark]         = {0.40f, 0.70f, 1.00f, 1.00f};
    c[ImGuiCol_SliderGrab]        = {0.24f, 0.44f, 0.80f, 1.00f};
    c[ImGuiCol_SliderGrabActive]  = {0.34f, 0.58f, 1.00f, 1.00f};

    c[ImGuiCol_Separator]         = {0.20f, 0.30f, 0.56f, 0.60f};
    c[ImGuiCol_SeparatorHovered]  = {0.30f, 0.46f, 0.80f, 0.90f};
    c[ImGuiCol_SeparatorActive]   = {0.40f, 0.60f, 1.00f, 1.00f};

    c[ImGuiCol_ResizeGrip]        = {0.16f, 0.28f, 0.60f, 0.60f};
    c[ImGuiCol_ResizeGripHovered] = {0.24f, 0.42f, 0.80f, 0.90f};
    c[ImGuiCol_ResizeGripActive]  = {0.34f, 0.56f, 1.00f, 1.00f};

    c[ImGuiCol_TableHeaderBg]     = {0.08f, 0.14f, 0.32f, 1.00f};
    c[ImGuiCol_TableBorderStrong] = {0.20f, 0.30f, 0.56f, 1.00f};
    c[ImGuiCol_TableBorderLight]  = {0.12f, 0.18f, 0.36f, 1.00f};
    c[ImGuiCol_TableRowBg]        = {0.00f, 0.00f, 0.00f, 0.00f};
    c[ImGuiCol_TableRowBgAlt]     = {1.00f, 1.00f, 1.00f, 0.04f};
}

} // namespace core
