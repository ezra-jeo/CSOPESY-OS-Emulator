#include "ScreenManager.h"
#include <iostream>
#include <sstream>

void ScreenManager::registerScreen(std::shared_ptr<Screen> screen) {
    registry[screen->name()] = std::move(screen);
}

std::shared_ptr<Screen> ScreenManager::getScreen(const std::string& name) const {
    auto it = registry.find(name);
    return it != registry.end() ? it->second : nullptr;
}

std::vector<std::string> ScreenManager::tokenize(const std::string& line) {
    std::vector<std::string> out;
    std::istringstream iss(line);
    for (std::string tok; iss >> tok; ) out.push_back(tok);
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
