#include "BootSequence.h"
#include <imgui.h>
#include <cmath>

namespace shell {

static constexpr float kBiosDur    = 1.8f;
static constexpr float kSplashDur  = 1.8f;
static constexpr float kLoadDur    = 2.0f;

void BootSequence::update(float dt) {
    if (state_ == State::Done) return;

    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsKeyPressed(ImGuiKey_Space) ||
        ImGui::IsKeyPressed(ImGuiKey_Enter) ||
        io.MouseClicked[0]) {
        skip(); return;
    }

    timer_ += dt;

    switch (state_) {
    case State::Bios:
        if (timer_ >= kBiosDur) { state_ = State::Splash; timer_ = 0; }
        break;
    case State::Splash:
        if (timer_ >= kSplashDur) { state_ = State::Loading; timer_ = 0; }
        break;
    case State::Loading:
        loadProgress_ = timer_ / kLoadDur;
        if (timer_ >= kLoadDur) { state_ = State::Done; }
        break;
    default: break;
    }
}

void BootSequence::draw() {
    if (state_ == State::Done) return;

    ImGuiIO& io = ImGui::GetIO();
    float W = io.DisplaySize.x, H = io.DisplaySize.y;

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoInputs;

    ImGui::SetNextWindowPos({0,0});
    ImGui::SetNextWindowSize({W,H});
    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::Begin("##boot", nullptr, flags);
    ImGui::PopStyleColor();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled({0,0}, {W,H}, IM_COL32(0,0,0,255));

    if (state_ == State::Bios) {
        float cy = H * 0.15f;
        float cx = W * 0.08f;
        ImGui::SetCursorPos({cx, cy});
        ImGui::TextColored({0.9f,0.9f,0.9f,1.0f}, "CSOPESY Megatrends BIOS v2026.06");
        ImGui::SetCursorPos({cx, cy + 28});
        ImGui::TextColored({0.7f,0.7f,0.7f,1.0f}, "Copyright (C) 2026, CSOPESY Group. All Rights Reserved.");
        ImGui::SetCursorPos({cx, cy + 56});
        ImGui::TextColored({0.7f,0.7f,0.7f,1.0f}, "CPU: CSOPESY Emulated Core  @ 3.20GHz");
        ImGui::SetCursorPos({cx, cy + 72});
        ImGui::TextColored({0.7f,0.7f,0.7f,1.0f}, "Memory Test: 8192MB OK");
        ImGui::SetCursorPos({cx, cy + 88});
        ImGui::TextColored({0.7f,0.7f,0.7f,1.0f}, "Detecting Primary Master... CSOPESY-DISK-256G");
        ImGui::SetCursorPos({cx, cy + 120});
        ImGui::TextColored({0.5f,0.5f,0.5f,1.0f}, "Press any key to skip...");
    } else if (state_ == State::Splash) {
        const char* logo[] = {
            " ██████╗███████╗ ██████╗ ██████╗ ███████╗███████╗██╗   ██╗",
            "██╔════╝██╔════╝██╔═══██╗██╔══██╗██╔════╝██╔════╝╚██╗ ██╔╝",
            "██║     █████╗  ██║   ██║██████╔╝█████╗  ███████╗ ╚████╔╝ ",
            "██║     ╚════██╗██║   ██║██╔═══╝ ██╔══╝  ╚════██║  ╚██╔╝  ",
            "╚██████╗███████║╚██████╔╝██║     ███████╗███████║   ██║   ",
            " ╚═════╝╚══════╝ ╚═════╝ ╚═╝     ╚══════╝╚══════╝   ╚═╝   ",
        };
        float lineH = 20.0f;
        int lines = (int)(sizeof(logo)/sizeof(*logo));
        float startY = H * 0.35f - lines * lineH * 0.5f;
        for (int i = 0; i < lines; ++i) {
            ImVec2 ts = ImGui::CalcTextSize(logo[i]);
            ImGui::SetCursorPos({(W - ts.x) * 0.5f, startY + i * lineH});
            ImGui::TextColored({0.3f, 0.8f, 1.0f, 1.0f}, "%s", logo[i]);
        }

        ImVec2 ts2 = ImGui::CalcTextSize("Operating System Emulator  v1.0");
        ImGui::SetCursorPos({(W - ts2.x) * 0.5f, H * 0.62f});
        ImGui::TextColored({0.6f,0.6f,0.6f,1.0f}, "Operating System Emulator  v1.0");
        ImVec2 ts3 = ImGui::CalcTextSize("Press any key to skip...");
        ImGui::SetCursorPos({(W - ts3.x) * 0.5f, H * 0.75f});
        ImGui::TextColored({0.4f,0.4f,0.4f,1.0f}, "Press any key to skip...");
    } else if (state_ == State::Loading) {
        float prog = loadProgress_ < 1.0f ? loadProgress_ : 1.0f;
        float barW = W * 0.4f;
        float barH = 8.0f;
        float bx = (W - barW) * 0.5f;
        float by = H * 0.55f;
        dl->AddRect({bx, by}, {bx + barW, by + barH}, IM_COL32(80,160,255,200));
        dl->AddRectFilled({bx, by}, {bx + barW * prog, by + barH}, IM_COL32(60,140,240,255));

        ImVec2 ts = ImGui::CalcTextSize("Loading CSOPESY...");
        ImGui::SetCursorPos({(W - ts.x) * 0.5f, H * 0.48f});
        ImGui::TextColored({0.7f,0.8f,1.0f,1.0f}, "Loading CSOPESY...");
        ImVec2 ts2 = ImGui::CalcTextSize("Press any key to skip...");
        ImGui::SetCursorPos({(W - ts2.x) * 0.5f, H * 0.63f});
        ImGui::TextColored({0.4f,0.4f,0.4f,1.0f}, "Press any key to skip...");
    }

    ImGui::End();
}

} // namespace shell
