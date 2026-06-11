#pragma once

namespace core {

// Applies the CSOPESY retro-OS colour theme to ImGuiStyle.
// Call once after StyleColorsDark(), before ImGui_ImplOpenGL3_Init.
void applyTheme();

} // namespace core
