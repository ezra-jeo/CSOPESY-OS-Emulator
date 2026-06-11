#include "FileExplorerApp.h"
#include <imgui.h>
#include <cctype>
#include <cstring>

namespace apps {

static const char* kDirs[]  = { "C:\\", "Documents", "Downloads", "System32", "Users", "Temp" };
static const char* kFiles[] = {
    "csopesy.exe", "README.txt", "config.ini", "log.txt",
    "wallpaper.bmp", "font.ttf",  "report.pdf", "notes.txt",
    "boot.ini", "kernel.sys", "drivers.dat", "setup.exe",
};

static bool ciContains(const char* haystack, const char* needle) {
    if (!needle || needle[0] == '\0') return true;
    for (const char* h = haystack; *h; ++h) {
        const char* n = needle;
        const char* hh = h;
        while (*n && *hh && std::tolower((unsigned char)*hh) == std::tolower((unsigned char)*n))
            { ++hh; ++n; }
        if (*n == '\0') return true;
    }
    return false;
}

FileExplorerApp::FileExplorerApp() : compositor::Window("File Explorer") {}

void FileExplorerApp::draw() {
    if (focusRequested_) {
        ImGui::SetNextWindowFocus();
        focusRequested_ = false;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowSize({560, 380}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(
        {io.DisplaySize.x * 0.3f, io.DisplaySize.y * 0.25f},
        ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(title_.c_str(), &open_)) {
        focused_ = false;
        ImGui::End();
        return;
    }
    focused_ = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    // ── Address / search bar ──────────────────────────────────────────────
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##srch", "Search files...", searchBuf_, sizeof(searchBuf_));
    ImGui::Separator();

    // ── Two-panel layout ─────────────────────────────────────────────────
    float panelH = ImGui::GetContentRegionAvail().y - 28;

    // Left: directory tree
    ImGui::BeginChild("##dirs", {140, panelH}, true);
    ImGui::TextColored({0.6f,0.8f,1.0f,1.0f}, "This PC");
    ImGui::Separator();
    for (int i = 0; i < (int)(sizeof(kDirs)/sizeof(*kDirs)); ++i) {
        bool sel = (selectedDir_ == i);
        if (ImGui::Selectable(kDirs[i], sel)) {
            selectedDir_ = i;
            selectedFile_ = -1;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Right: file list, filtered by searchBuf_
    ImGui::BeginChild("##files", {0, panelH}, true);
    ImGui::TextColored({0.6f,0.8f,1.0f,1.0f}, "%s", kDirs[selectedDir_]);
    ImGui::Separator();

    int visibleCount = 0;
    for (int i = 0; i < (int)(sizeof(kFiles)/sizeof(*kFiles)); ++i) {
        if (!ciContains(kFiles[i], searchBuf_)) continue;
        ++visibleCount;
        bool sel = (selectedFile_ == i);
        ImGui::PushID(i);
        if (ImGui::Selectable(kFiles[i], sel, 0, {0, 0}))
            selectedFile_ = sel ? -1 : i;
        ImGui::PopID();
    }
    if (visibleCount == 0)
        ImGui::TextDisabled("No matches.");
    ImGui::EndChild();

    // ── Status bar ────────────────────────────────────────────────────────
    ImGui::Separator();
    if (selectedFile_ >= 0 && ciContains(kFiles[selectedFile_], searchBuf_))
        ImGui::TextDisabled("Selected: %s", kFiles[selectedFile_]);
    else
        ImGui::TextDisabled("%d item(s)", visibleCount);

    ImGui::End();
}

} // namespace apps
