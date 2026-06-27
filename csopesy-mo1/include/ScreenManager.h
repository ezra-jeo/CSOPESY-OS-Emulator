#pragma once
#include "Screen.h"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// Owns the single stdin loop and two structures:
//   • registry — named screens you register (e.g. the main menu); look up by name().
//   • stack    — the active attach/detach navigation; the top screen has input focus.
// Dynamic screens (e.g. a per-process screen) are created on demand and pushed, not pre-registered.
class ScreenManager {
public:
    void registerScreen(std::shared_ptr<Screen> screen);          // keyed by screen->name()
    std::shared_ptr<Screen> getScreen(const std::string& name) const;

    // Push the registered screen `initialName` and run until the stack empties (Quit / EOF).
    void run(const std::string& initialName);

private:
    static std::vector<std::string> tokenize(const std::string& line);

    std::unordered_map<std::string, std::shared_ptr<Screen>> registry;
    std::vector<std::shared_ptr<Screen>>                     stack;
};
