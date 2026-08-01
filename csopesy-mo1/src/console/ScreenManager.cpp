#include "ScreenManager.h"
#include <iostream>
#include <sstream>
#include <cctype>

void ScreenManager::registerScreen(std::shared_ptr<Screen> screen) {
    registry[screen->name()] = std::move(screen);
}

std::shared_ptr<Screen> ScreenManager::getScreen(const std::string& name) const {
    auto it = registry.find(name);
    return it != registry.end() ? it->second : nullptr;
}

// Whitespace-splits `line` like the old std::istringstream >> loop, except a "..." span
// collapses into a single token (quotes stripped) and \" inside a quoted span unescapes to ".
std::vector<std::string> ScreenManager::tokenize(const std::string& line) {
    std::vector<std::string> out;
    std::string cur;
    bool inQuotes = false;
    bool haveCur  = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (inQuotes) {
            if (c == '\\' && i + 1 < line.size() && line[i + 1] == '"') { cur += '"'; ++i; }
            else if (c == '"') inQuotes = false;
            else cur += c;
        } else {
            if (c == '"') { inQuotes = true; haveCur = true; }
            else if (std::isspace(static_cast<unsigned char>(c))) {
                if (haveCur) { out.push_back(cur); cur.clear(); haveCur = false; }
            } else { cur += c; haveCur = true; }
        }
    }
    if (haveCur) out.push_back(cur);
    return out;
}

void ScreenManager::run(const std::string& initialName) {
    auto initial = getScreen(initialName);
    if (!initial) return;
    stack.push_back(initial);
    initial->onEnter();

    std::string line;
    while (!stack.empty()) {
        std::shared_ptr<Screen> top = stack.back();
        std::cout << top->prompt();
        std::cout.flush();

        if (!std::getline(std::cin, line)) break;   // EOF / closed stdin → exit
        auto args = tokenize(line);
        if (args.empty()) continue;

        ScreenAction action = top->handleCommand(args);
        switch (action.type) {
            case ScreenAction::Stay:
                break;
            case ScreenAction::Push:
                stack.push_back(action.next);
                action.next->onEnter();
                break;
            case ScreenAction::Pop:
                stack.pop_back();
                if (!stack.empty()) stack.back()->onEnter();
                break;
            case ScreenAction::Quit:
                stack.clear();
                break;
        }
    }
}
