#include "ProcessScreen.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <algorithm>

namespace {
    const char* R  = "\033[0m";
    const char* B  = "\033[1m";
    const char* LG = "\033[92m";
    const char* CY = "\033[96m";
    const char* WH = "\033[97m";
    const char* GR = "\033[90m";
    const char* YL = "\033[93m";
}

ProcessScreen::ProcessScreen(std::shared_ptr<Process> proc) : proc(std::move(proc)) {}

std::string ProcessScreen::name()   const { return proc->getName(); }
std::string ProcessScreen::prompt() const { return std::string(B) + WH + "root:\\> " + R; }
void        ProcessScreen::onEnter()      { render(); }

ScreenAction ProcessScreen::handleCommand(const std::vector<std::string>& args) {
    if (args[0] == "exit")        return ScreenAction::pop();
    if (args[0] == "process-smi") { render(); return ScreenAction::stay(); }

    std::cout << GR << "csosh: " << R
              << "command not found: " << YL << args[0] << R << "\n";
    return ScreenAction::stay();
}

void ProcessScreen::render() const {
    std::cout << "\033[2J\033[H"; // clear screen
    std::cout << B << WH << "Process name: " << R << proc->getName() << "\n"
              << B << WH << "ID: "           << R << proc->getPID()  << "\n\n";

    std::cout << B << WH << "Logs:\n" << R;
    for (const auto& entry : proc->getLogs())
        std::cout << entry << "\n";
    std::cout << "\n";

    if (proc->isFinished()) {
        std::cout << B << LG << "Finished!\n" << R;
    } else {
        std::cout << B << WH << "Current instruction line: " << R
                  << proc->getCommandCounter() << "\n"
                  << B << WH << "Lines of code: "            << R
                  << proc->getTotalCommands()  << "\n";
    }
    std::cout << "\n";

    // Full instruction listing; the current line is localized by a colored, bracketed number [N].
    std::cout << B << WH << "Instructions:\n" << R;
    auto listing = proc->getInstructionListing();
    const int cur = proc->getCommandCounter();
    const int w   = static_cast<int>(std::to_string(listing.size()).size()) + 2; // room for [..]
    for (std::size_t i = 0; i < listing.size(); ++i) {
        const int  ln   = static_cast<int>(i + 1);
        const bool here = (ln == cur);
        const std::string label = here ? ("[" + std::to_string(ln) + "]") : std::to_string(ln);
        const std::string pad(std::max(0, w - static_cast<int>(label.size())), ' ');
        if (here)
            std::cout << pad << B << CY << label << ": " << listing[i] << R << "\n";
        else
            std::cout << pad << label << ": " << listing[i] << "\n";
    }
    std::cout << "\n";
}
