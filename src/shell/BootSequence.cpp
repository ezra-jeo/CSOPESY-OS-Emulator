#include "BootSequence.h"

#include <imgui.h>
#include <cmath>
#include <cstdio>

namespace shell {

static constexpr float kBiosDuration    = 1.5f;
static constexpr float kSplashDuration  = 1.5f;
static constexpr float kLoadDuration    = 2.0f;

void BootSequence::update(float dt) {
    if (state_ == State::Done) return;

    ImGuiIO& io = ImGui::GetIO();
    // Any key or mouse click skips to desktop
    if (ImGui::IsKeyPressed(ImGuiKey_Space, false)  ||
        ImGui::IsKeyPressed(ImGuiKey_Enter, false)  ||
        ImGui::IsKeyPressed(ImGuiKey_Escape, false) ||
        io.MouseClicked[0]) {
        state_ = State::Done;
        return;
    }

    timer_ += dt;

    switch (state_) {
    case State::Bios:
        if (timer_ >= kBiosDuration) { state_ = State::Splash; timer_ = 0.0f; }
        break;
    case State::Splash:
        if (timer_ >= kSplashDuration) { state_ = State::Loading; timer_ = 0.0f; }
        break;
    case State::Loading:
        if (timer_ >= kLoadDuration) { state_ = State::Done; }
        break;
    default:
        break;
    }
}

void BootSequence::draw() {
    ImGuiIO& io = ImGui::GetIO();
    float W = io.DisplaySize.x;
    float H = io.DisplaySize.y;

    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize({W, H});
    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration   |
        ImGuiWindowFlags_NoMove         |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoNav          |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("##boot", nullptr, flags);
    ImGui::PopStyleColor();

    ImDrawList* dl = ImGui::GetWindowDrawList();

    if (state_ == State::Bios) {
        float y = H * 0.08f;
        auto line = [&](const char* text, ImU32 col = IM_COL32(180,180,180,255)) {
            ImVec2 sz = ImGui::CalcTextSize(text);
            dl->AddText({(W - sz.x) * 0.5f, y}, col, text);
            y += ImGui::GetTextLineHeight() + 2.0f;
        };

        line("CSOPESY Megatrends (R) BIOS v2.1", IM_COL32(255,255,100,255));
        y += 12.0f;
        line("Copyright (C) 2026, CSOPESY Systems, Inc.");
        y += 8.0f;
        line("CPU: CSOPESY-X86 @ 3.60 GHz");
        line("Memory Test: 8192 MB OK");
        y += 8.0f;
        line("Detecting drives...");
        if (timer_ > 0.3f) line("  Primary Master:   CSOPESY-DISK  256 GB");
        if (timer_ > 0.6f) line("  Primary Slave:    None");
        if (timer_ > 0.9f) { y += 8.0f; line("Booting from: CSOPESY OS...", IM_COL32(100,220,100,255)); }

        y += 20.0f;
        line("Press any key or click to skip", IM_COL32(100,100,100,200));

    } else if (state_ == State::Splash) {
        const char* art[] = {
            "  ____  ____   ___  ____  _____ ______  __",
            " / ___||  _ \\ / _ \\|  _ \\| ____/ ___\\ \\/ /",
            "| |    | |_) | | | | |_) |  _| \\___ \\\\  / ",
            "| |___ |  __/| |_| |  __/| |___ ___) /  \\ ",
            " \\____||_|    \\___/|_|   |_____|____/_/\\_\\",
        };
        float totalH = 5 * (ImGui::GetTextLineHeight() + 4.0f);
        float startY = (H - totalH) * 0.38f;
        for (int i = 0; i < 5; ++i) {
            ImVec2 sz = ImGui::CalcTextSize(art[i]);
            dl->AddText({(W - sz.x) * 0.5f, startY + i * (ImGui::GetTextLineHeight() + 4.0f)},
                        IM_COL32(80, 160, 255, 255), art[i]);
        }

        float tagY = startY + totalH + 24.0f;
        const char* tag = "OS Emulator  v1.0.0  |  Build 2026";
        ImVec2 tsz = ImGui::CalcTextSize(tag);
        dl->AddText({(W - tsz.x) * 0.5f, tagY}, IM_COL32(140,200,255,200), tag);

        const char* skip = "Press any key or click to skip";
        ImVec2 ssz = ImGui::CalcTextSize(skip);
        dl->AddText({(W - ssz.x) * 0.5f, H * 0.88f}, IM_COL32(100,100,100,200), skip);

    } else if (state_ == State::Loading) {
        const char* msg = "Loading CSOPESY OS...";
        ImVec2 msz = ImGui::CalcTextSize(msg);
        dl->AddText({(W - msz.x) * 0.5f, H * 0.44f}, IM_COL32(200,220,255,230), msg);

        float progress = (kLoadDuration > 0.0f) ? (timer_ / kLoadDuration) : 1.0f;
        float barW = W * 0.4f;
        float barH = 14.0f;
        float bx = (W - barW) * 0.5f;
        float by = H * 0.52f;
        // Track
        dl->AddRectFilled({bx, by}, {bx + barW, by + barH},
                          IM_COL32(40, 40, 60, 255), 4.0f);
        // Fill
        dl->AddRectFilled({bx, by}, {bx + barW * progress, by + barH},
                          IM_COL32(80, 160, 255, 255), 4.0f);
        // Border
        dl->AddRect({bx, by}, {bx + barW, by + barH},
                    IM_COL32(120, 160, 220, 180), 4.0f);

        char pct[16];
        snprintf(pct, sizeof(pct), "%d%%", (int)(progress * 100));
        ImVec2 psz = ImGui::CalcTextSize(pct);
        dl->AddText({(W - psz.x) * 0.5f, by + barH + 8.0f},
                    IM_COL32(180,200,255,200), pct);
    }

    ImGui::End();
}

} // namespace shell
