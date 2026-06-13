#include "BootSequence.h"
#include <imgui.h>

namespace shell {

static constexpr float kBiosDuration    = 2.5f;
static constexpr float kSplashDuration  = 1.5f;
static constexpr float kLoadingDuration = 2.0f;

static const char* kBiosLines[] = {
    "CSOPESY BIOS Revision 1.00",
    "Copyright (C) 2024, CSOPESY Systems Inc.",
    "",
    "CPU: Intel(R) Core(TM) i7  @  3600 MHz",
    "Memory Test : 8192 MB  OK",
    "",
    "Detecting storage devices...",
    "  Drive 0: SSD  256 GB  [OK]",
    "",
    "PCI Device Listing:",
    "  VGA Compatible Controller  [OK]",
    "  Ethernet Controller        [OK]",
    "",
    "Press DEL to enter Setup",
    "",
    "Booting CSOPESY OS...",
};
static constexpr int kBiosLineCount = (int)(sizeof(kBiosLines) / sizeof(kBiosLines[0]));

void BootSequence::update(float dt) {
    timer_ += dt;
    switch (state_) {
        case State::Bios:
            if (timer_ >= kBiosDuration)    { state_ = State::Splash;  timer_ = 0.0f; } break;
        case State::Splash:
            if (timer_ >= kSplashDuration)  { state_ = State::Loading; timer_ = 0.0f; } break;
        case State::Loading:
            if (timer_ >= kLoadingDuration) { state_ = State::Done;    timer_ = 0.0f; } break;
        case State::Done: break;
    }
}

static void beginFullscreenWindow(const char* id) {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({0.0f, 0.0f});
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin(id, nullptr,
        ImGuiWindowFlags_NoDecoration          |
        ImGuiWindowFlags_NoMove                |
        ImGuiWindowFlags_NoSavedSettings       |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNav                 |
        ImGuiWindowFlags_NoInputs);
}

static void endFullscreenWindow() {
    ImGui::PopStyleColor(); // WindowBg
    ImGui::PopStyleVar();   // WindowPadding
    ImGui::End();
}

void BootSequence::draw() {
    ImGuiIO& io = ImGui::GetIO();
    float W = io.DisplaySize.x;
    float H = io.DisplaySize.y;

    switch (state_) {
        case State::Bios: {
            beginFullscreenWindow("##bios");
            ImGui::SetCursorPos({40.0f, 40.0f});
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 1.0f, 0.6f, 1.0f));
            int visible = (int)((timer_ / kBiosDuration) * kBiosLineCount) + 1;
            if (visible > kBiosLineCount) visible = kBiosLineCount;
            for (int i = 0; i < visible; ++i)
                ImGui::Text("%s", kBiosLines[i]);
            if ((int)(ImGui::GetTime() * 2.0) % 2 == 0)
                ImGui::Text("_");
            ImGui::PopStyleColor();
            endFullscreenWindow();
            break;
        }
        case State::Splash: {
            beginFullscreenWindow("##splash");
            const char* title = "CSOPESY OS";
            const char* sub   = "Operating System Emulator";
            const char* ver   = "Version 1.0.0";

            ImGui::SetWindowFontScale(3.0f);
            ImVec2 tsz = ImGui::CalcTextSize(title);
            ImGui::SetCursorPos({(W - tsz.x) * 0.5f, H * 0.35f});
            ImGui::TextColored({0.2f, 0.8f, 1.0f, 1.0f}, "%s", title);
            ImGui::SetWindowFontScale(1.0f);

            ImVec2 ssz = ImGui::CalcTextSize(sub);
            ImGui::SetCursorPos({(W - ssz.x) * 0.5f, H * 0.55f});
            ImGui::TextColored({0.8f, 0.8f, 0.8f, 1.0f}, "%s", sub);

            ImVec2 vsz = ImGui::CalcTextSize(ver);
            ImGui::SetCursorPos({(W - vsz.x) * 0.5f, H * 0.62f});
            ImGui::TextColored({0.5f, 0.5f, 0.5f, 1.0f}, "%s", ver);

            endFullscreenWindow();
            break;
        }
        case State::Loading: {
            beginFullscreenWindow("##loading");
            float progress = timer_ / kLoadingDuration;
            const char* label = "CSOPESY OS";

            ImGui::SetWindowFontScale(2.5f);
            ImVec2 lsz = ImGui::CalcTextSize(label);
            ImGui::SetCursorPos({(W - lsz.x) * 0.5f, H * 0.4f});
            ImGui::TextColored({0.2f, 0.8f, 1.0f, 1.0f}, "%s", label);
            ImGui::SetWindowFontScale(1.0f);

            float barW = W * 0.4f;
            ImGui::SetCursorPos({(W - barW) * 0.5f, H * 0.58f});
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
            ImGui::ProgressBar(progress, {barW, 10.0f}, "");
            ImGui::PopStyleColor();

            endFullscreenWindow();
            break;
        }
        case State::Done:
            break;
    }
}

} // namespace shell
