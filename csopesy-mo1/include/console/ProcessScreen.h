#pragma once
#include "Screen.h"
#include "Process.h"
#include <string>
#include <vector>
#include <memory>

// An attached process screen — the `root:\>` view. Renders the process's name/ID, PRINT logs,
// instruction-line/lines-of-code (or Finished!), and the full instruction listing with the current
// line localized. `process-smi` refreshes the view; `exit` detaches (pops back to the menu).
class ProcessScreen : public Screen {
public:
    explicit ProcessScreen(std::shared_ptr<Process> proc);

    std::string name()   const override;
    std::string prompt() const override;
    void onEnter() override;     // renders once on attach
    ScreenAction handleCommand(const std::vector<std::string>& args) override;

private:
    void render() const;
    std::shared_ptr<Process> proc;
};
