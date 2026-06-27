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

    // Instruction listing — a window around the current line (full listings can be 1000s of lines).
    // The current line is localized by a colored, bracketed number [N]; hidden lines above/below
    // are summarized.
    constexpr int CONTEXT = 10;  // lines shown on each side of the current line
    std::cout << B << WH << "Instructions:\n" << R;
    auto listing = proc->getInstructionListing();
    const int total = static_cast<int>(listing.size());
    const int cur   = proc->getCommandCounter();
    const int w     = static_cast<int>(std::to_string(total).size()) + 2; // room for [..]

    int lo, hi;
    if (cur <= 0) {                       // not started yet → show the top of the program
        lo = 1;
        hi = std::min(total, 2 * CONTEXT + 1);
    } else {
        lo = std::max(1, cur - CONTEXT);
        hi = std::min(total, cur + CONTEXT);
    }

    if (lo > 1)
        std::cout << GR << "       ... " << (lo - 1) << " more above\n" << R;
    for (int ln = lo; ln <= hi; ++ln) {
        const std::string& text = listing[ln - 1];
        const bool here = (ln == cur);
        const std::string label = here ? ("[" + std::to_string(ln) + "]") : std::to_string(ln);
        const std::string pad(std::max(0, w - static_cast<int>(label.size())), ' ');
        if (here)
            std::cout << pad << B << CY << label << ": " << text << R << "\n";
        else
            std::cout << pad << label << ": " << text << "\n";
    }
    if (hi < total)
        std::cout << GR << "       ... " << (total - hi) << " more below\n" << R;
    std::cout << "\n";
}
