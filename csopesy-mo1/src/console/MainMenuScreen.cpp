#include "MainMenuScreen.h"
#include "ProcessScreen.h"
#include "Console.h"
#include "Process.h"
#include <iostream>
#include <memory>
#include <set>

namespace {
    const char* R  = "\033[0m";
    const char* B  = "\033[1m";
    const char* LG = "\033[92m";
    const char* CY = "\033[96m";
    const char* GR = "\033[90m";
    const char* YL = "\033[93m";
}

MainMenuScreen::MainMenuScreen(Console& console) : console(console) {}

std::string MainMenuScreen::prompt() const {
    return std::string(B) + LG + "user@csopesy" + R + ":" + B + CY + "~" + R + "$ ";
}

ScreenAction MainMenuScreen::handleCommand(const std::vector<std::string>& args) {
    const std::string& cmd = args[0];

    if (cmd == "exit") {
        std::cout << "\n" << GR << "  Shutting down CSOPESY OS...\n" << R << "\n";
        return ScreenAction::quit();
    }
    if (cmd == "initialize") { console.cmdInitialize(); return ScreenAction::stay(); }

    // Recognized post-init commands; everything else is "command not found".
    static const std::set<std::string> known = {
        "screen", "scheduler-start", "scheduler-stop", "report-util" };

    if (!console.isInitialized()) {
        std::cout << GR << "error: " << R << "run initialize first\n";
        return ScreenAction::stay();
    } else if (known.find(cmd) == known.end()) {
        std::cout << GR << "csosh: " << R
                  << "command not found: " << YL << cmd << R << "\n";
        return ScreenAction::stay();
    }

    if (cmd == "screen")          return handleScreen(args);
    if (cmd == "scheduler-start") { console.cmdSchedulerStart(); return ScreenAction::stay(); }
    if (cmd == "scheduler-stop")  { console.cmdSchedulerStop();  return ScreenAction::stay(); }
    if (cmd == "report-util")     { console.cmdReportUtil();     return ScreenAction::stay(); }
    return ScreenAction::stay();
}

ScreenAction MainMenuScreen::handleScreen(const std::vector<std::string>& args) {
    if (args.size() >= 2 && args[1] == "-ls") {
        console.printProcessList(std::cout, true);
        return ScreenAction::stay();
    }
    if (args.size() >= 3 && args[1] == "-s") {  // create (or attach to existing) and enter
        auto proc = console.getOrCreateProcess(args[2]);
        return ScreenAction::push(std::make_shared<ProcessScreen>(proc));
    }
    if (args.size() >= 3 && args[1] == "-r") {  // re-attach; must exist and not be finished
        auto proc = console.findProcess(args[2]);
        if (!proc || proc->isFinished()) {
            std::cout << "Process " << args[2] << " not found.\n";
            return ScreenAction::stay();
        }
        return ScreenAction::push(std::make_shared<ProcessScreen>(proc));
    }
    std::cout << GR << "  usage: screen -ls | screen -s <name> | screen -r <name>\n" << R;
    return ScreenAction::stay();
}
