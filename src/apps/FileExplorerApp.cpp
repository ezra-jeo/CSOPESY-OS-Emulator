#include "FileExplorerApp.h"
#include <imgui.h>

namespace apps {

static const char* kDirs[]  = { "C:\\", "Documents", "Downloads", "System32", "Users", "Temp" };
static const char* kFiles[] = {
    "csopesy.exe", "README.txt", "config.ini", "log.txt",
    "wallpaper.bmp", "font.ttf",  "report.pdf", "notes.txt"
};

FileExplorerApp::FileExplorerApp() : compositor::Window("File Explorer") {}

void FileExplorerApp::draw() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowSize({560, 360}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(
        {io.DisplaySize.x * 0.3f, io.DisplaySize.y * 0.25f},
        ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(title_.c_str(), &open_)) { ImGui::End(); return; }

    // ── Address / search bar ──────────────────────────────────────────────
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##srch", "Search files...", searchBuf_, sizeof(searchBuf_));
    ImGui::Separator();

    // ── Two-panel layout ─────────────────────────────────────────────────
    float panelH = ImGui::GetContentRegionAvail().y - 24;

    // Left: directory tree
    ImGui::BeginChild("##dirs", {140, panelH}, true);
    ImGui::TextColored({0.6f,0.8f,1.0f,1.0f}, "This PC");
    ImGui::Separator();
    for (int i = 0; i < (int)(sizeof(kDirs)/sizeof(*kDirs)); ++i) {
        bool sel = (selectedDir_ == i);
        if (ImGui::Selectable(kDirs[i], sel))
            selectedDir_ = i;
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Right: file list
    ImGui::BeginChild("##files", {0, panelH}, true);
    ImGui::TextColored({0.6f,0.8f,1.0f,1.0f}, "%s", kDirs[selectedDir_]);
    ImGui::Separator();
    for (int i = 0; i < (int)(sizeof(kFiles)/sizeof(*kFiles)); ++i) {
        bool sel = (selectedFile_ == i);
        if (ImGui::Selectable(kFiles[i], sel))
            selectedFile_ = i;
    }
    ImGui::EndChild();

    // ── Status bar ────────────────────────────────────────────────────────
    ImGui::Separator();
    if (selectedFile_ >= 0)
        ImGui::TextDisabled("Selected: %s", kFiles[selectedFile_]);
    else
        ImGui::TextDisabled("%d items", (int)(sizeof(kFiles)/sizeof(*kFiles)));

    ImGui::End();
}

} // namespace apps
