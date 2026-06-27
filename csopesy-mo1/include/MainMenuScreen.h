#pragma once
#include "Screen.h"
#include <string>
#include <vector>

class Console;  // system facade: owns config/scheduler/generator/registry + the operations

// The main-menu screen. Interprets the top-level commands (initialize, scheduler-start/stop,
// report-util, screen, exit), delegating system work to Console and returning navigation actions
// (e.g. attaching a ProcessScreen on `screen -s/-r`, quitting on `exit`).
class MainMenuScreen : public Screen {
public:
    explicit MainMenuScreen(Console& console);

    std::string name()   const override { return "main-menu"; }
    std::string prompt() const override;
    ScreenAction handleCommand(const std::vector<std::string>& args) override;

private:
    ScreenAction handleScreen(const std::vector<std::string>& args);
    Console& console;
};
