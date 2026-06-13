#include "FileExplorerApp.h"
#include <imgui.h>
#include <cctype>

namespace apps {

static const char* kDirs[] = { "C:\\", "Documents", "Downloads", "System32", "Users", "Temp" };
static const char* kDirFiles[][8] = {
    { "csopesy.exe", "README.txt",     "config.ini",  nullptr },   // C:
    { "report.pdf",  "notes.txt",      "thesis.docx", nullptr },   // Documents
    { "installer.exe","wallpaper.bmp", "archive.zip", nullptr },   // Downloads
    { "kernel32.dll", "ntdll.dll",     "cmd.exe",     nullptr },   // System32
    { "Admin",        "Guest",         "DefaultUser", nullptr },   // Users
    { "tmp001.tmp",   "tmp002.tmp",    nullptr },                   // Temp
};

// Derive the file count from the nullptr terminator so the data has a single
// source of truth.
static int countFiles(const char** files) {
    int n = 0;
    while (files[n]) ++n;
    return n;
}

FileExplorerApp::FileExplorerApp() : compositor::Window("File Explorer") {}

void FileExplorerApp::draw() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowSize({560, 360}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(
        {io.DisplaySize.x * 0.3f, io.DisplaySize.y * 0.25f},
        ImGuiCond_FirstUseEver);

    if (focusRequested_) { ImGui::SetNextWindowFocus(); focusRequested_ = false; }
    if (!ImGui::Begin(title_.c_str(), &open_)) { focused_ = false; ImGui::End(); return; }
    focused_ = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    // ── Address / search bar ──────────────────────────────────────────────
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##srch", "Search files...", searchBuf_, sizeof(searchBuf_));
    ImGui::Separator();

    auto containsCI = [](const char* hay, const char* needle) {
        if (!needle || needle[0] == '\0') return true;
        for (; *hay; ++hay) {
            const char* h = hay; const char* n = needle;
            while (*h && *n && std::tolower((unsigned char)*h) == std::tolower((unsigned char)*n))
                ++h, ++n;
            if (!*n) return true;
        }
        return false;
    };

    // ── Two-panel layout ─────────────────────────────────────────────────
    float panelH = ImGui::GetContentRegionAvail().y - 24;
    if (panelH < 1.0f) panelH = 1.0f;

    const char** files = kDirFiles[selectedDir_];
    int count          = countFiles(files);

    // Left: directory tree
    ImGui::BeginChild("##dirs", {140, panelH}, true);
    ImGui::TextColored({0.6f,0.8f,1.0f,1.0f}, "This PC");
    ImGui::Separator();
    for (int i = 0; i < (int)(sizeof(kDirs)/sizeof(*kDirs)); ++i) {
        bool sel = (selectedDir_ == i);
        if (ImGui::Selectable(kDirs[i], sel)) {
            selectedDir_  = i;
            selectedFile_ = -1;
            files  = kDirFiles[selectedDir_];
            count  = countFiles(files);
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Right: file list filtered by searchBuf_
    ImGui::BeginChild("##files", {0, panelH}, true);
    ImGui::TextColored({0.6f,0.8f,1.0f,1.0f}, "%s", kDirs[selectedDir_]);
    ImGui::Separator();
    int visibleCount = 0;
    for (int i = 0; i < count; ++i) {
        if (!files[i]) continue;
        if (!containsCI(files[i], searchBuf_)) continue;
        bool sel = (selectedFile_ == i);
        if (ImGui::Selectable(files[i], sel))
            selectedFile_ = (sel ? -1 : i);
        ++visibleCount;
    }
    if (visibleCount == 0)
        ImGui::TextDisabled("No results");
    ImGui::EndChild();

    // ── Status bar ────────────────────────────────────────────────────────
    ImGui::Separator();
    if (selectedFile_ >= 0 && selectedFile_ < count && files[selectedFile_])
        ImGui::TextDisabled("Selected: %s", files[selectedFile_]);
    else
        ImGui::TextDisabled("%d items", visibleCount);

    ImGui::End();
}

} // namespace apps
