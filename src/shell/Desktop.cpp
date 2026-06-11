#include "Desktop.h"
#include "core/Application.h"
#include "core/Clock.h"
#include "core/Texture.h"

#include <imgui.h>
#include <GLFW/glfw3.h>

namespace shell {

void Desktop::draw(core::Application& /*app*/) {
    ImGuiIO& io = ImGui::GetIO();
    float W = io.DisplaySize.x;
    float H = io.DisplaySize.y;

    // Fullscreen, no decoration, below everything, no input capture
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize({W, H});
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove       |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoInputs;

    ImGui::Begin("##desktop", nullptr, flags);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p0{0, 0}, p1{W, H};

    // Try to load wallpaper once; fall back to gradient if absent
    static core::Texture wallpaper = core::loadTexture("assets/wallpapers/wallpaper.png");

    if (wallpaper.valid()) {
        dl->AddImage((ImTextureID)(uintptr_t)wallpaper.id, p0, p1);
    } else {
        // Deep navy → mid blue gradient fallback
        dl->AddRectFilledMultiColor(p0, p1,
            IM_COL32( 10,  20,  60, 255),
            IM_COL32( 10,  20,  60, 255),
            IM_COL32( 30,  80, 160, 255),
            IM_COL32( 30,  80, 160, 255));

        for (float y = 0; y < H; y += 4.0f)
            dl->AddLine({0, y}, {W, y}, IM_COL32(255,255,255, 6));
    }

    // ── OS label ──────────────────────────────────────────────────────────
    const char* label = "CSOPESY OS";
    ImVec2 lsz = ImGui::CalcTextSize(label);
    dl->AddText({(W - lsz.x) * 0.5f, H * 0.42f},
                IM_COL32(255,255,255,30), label);

    // ── Live clock (top-right corner) ────────────────────────────────────
    std::string ts = core::now();
    ImVec2 tsz = ImGui::CalcTextSize(ts.c_str());
    float clockX = W - tsz.x - 16.0f;
    float clockY = 8.0f;
    dl->AddText({clockX + 1, clockY + 1}, IM_COL32(0,0,0,180), ts.c_str());
    dl->AddText({clockX,     clockY},     IM_COL32(220,240,255,230), ts.c_str());

    ImGui::End();
}

} // namespace shell
