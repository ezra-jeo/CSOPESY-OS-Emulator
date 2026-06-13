#include "Theme.h"
#include <imgui.h>

namespace core {

void applyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();

    s.WindowRounding    = 3.0f;
    s.ChildRounding     = 2.0f;
    s.FrameRounding     = 2.0f;
    s.GrabRounding      = 2.0f;
    s.PopupRounding     = 3.0f;
    s.ScrollbarRounding = 2.0f;
    s.TabRounding       = 2.0f;

    s.WindowPadding  = {10, 8};
    s.FramePadding   = { 6, 3};
    s.ItemSpacing    = { 8, 5};
    s.ScrollbarSize  = 12.0f;

    // Windows XP Luna Blue — adapted for dark mode
    // Key reference colours:  #0A246A (title dark), #1259B0 (title light),
    //                          #316AC5 (selection), #1F3C78 (taskbar)
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg]          = {0.051f, 0.102f, 0.220f, 0.96f};
    c[ImGuiCol_ChildBg]           = {0.035f, 0.071f, 0.157f, 0.80f};
    c[ImGuiCol_PopupBg]           = {0.063f, 0.110f, 0.235f, 0.98f};
    c[ImGuiCol_Border]            = {0.141f, 0.275f, 0.557f, 0.70f};
    c[ImGuiCol_FrameBg]           = {0.059f, 0.129f, 0.290f, 0.80f};
    c[ImGuiCol_FrameBgHovered]    = {0.102f, 0.192f, 0.400f, 0.80f};
    c[ImGuiCol_FrameBgActive]     = {0.141f, 0.255f, 0.510f, 0.90f};
    c[ImGuiCol_TitleBg]           = {0.039f, 0.141f, 0.416f, 1.00f}; // #0A246A
    c[ImGuiCol_TitleBgActive]     = {0.071f, 0.349f, 0.690f, 1.00f}; // #1259B0
    c[ImGuiCol_TitleBgCollapsed]  = {0.039f, 0.141f, 0.416f, 0.80f};
    c[ImGuiCol_MenuBarBg]         = {0.063f, 0.141f, 0.318f, 1.00f};
    c[ImGuiCol_ScrollbarBg]       = {0.020f, 0.047f, 0.118f, 0.60f};
    c[ImGuiCol_ScrollbarGrab]     = {0.141f, 0.318f, 0.643f, 0.80f};
    c[ImGuiCol_ScrollbarGrabHovered] = {0.220f, 0.435f, 0.757f, 1.00f};
    c[ImGuiCol_ScrollbarGrabActive]  = {0.290f, 0.537f, 0.882f, 1.00f};
    c[ImGuiCol_CheckMark]         = {0.800f, 0.900f, 1.000f, 1.00f};
    c[ImGuiCol_SliderGrab]        = {0.192f, 0.416f, 0.773f, 1.00f}; // #316AC5
    c[ImGuiCol_SliderGrabActive]  = {0.290f, 0.537f, 0.882f, 1.00f};
    c[ImGuiCol_Button]            = {0.141f, 0.318f, 0.643f, 0.85f};
    c[ImGuiCol_ButtonHovered]     = {0.220f, 0.435f, 0.800f, 1.00f};
    c[ImGuiCol_ButtonActive]      = {0.071f, 0.349f, 0.690f, 1.00f};
    c[ImGuiCol_Header]            = {0.106f, 0.243f, 0.518f, 0.70f};
    c[ImGuiCol_HeaderHovered]     = {0.180f, 0.357f, 0.659f, 0.85f};
    c[ImGuiCol_HeaderActive]      = {0.220f, 0.435f, 0.757f, 1.00f};
    c[ImGuiCol_Separator]         = {0.141f, 0.275f, 0.557f, 0.50f};
    c[ImGuiCol_Tab]               = {0.059f, 0.176f, 0.400f, 0.86f};
    c[ImGuiCol_TabHovered]        = {0.192f, 0.416f, 0.741f, 1.00f};
    c[ImGuiCol_TabActive]         = {0.141f, 0.349f, 0.647f, 1.00f};
    c[ImGuiCol_TabUnfocused]      = {0.039f, 0.102f, 0.235f, 0.90f};
    c[ImGuiCol_TabUnfocusedActive]= {0.086f, 0.216f, 0.447f, 1.00f};
    c[ImGuiCol_TableHeaderBg]     = {0.071f, 0.157f, 0.345f, 1.00f};
    c[ImGuiCol_TableBorderStrong] = {0.141f, 0.275f, 0.557f, 1.00f};
    c[ImGuiCol_TableBorderLight]  = {0.102f, 0.200f, 0.416f, 0.70f};
    c[ImGuiCol_TableRowBg]        = {0.000f, 0.000f, 0.000f, 0.00f};
    c[ImGuiCol_TableRowBgAlt]     = {0.059f, 0.118f, 0.251f, 0.30f};
    c[ImGuiCol_TextSelectedBg]    = {0.192f, 0.416f, 0.773f, 0.35f};
    c[ImGuiCol_NavHighlight]      = {0.350f, 0.588f, 0.937f, 1.00f};
}

} // namespace core
