#pragma once
#include <string>
#include <vector>
#include <memory>

// One interactive console screen — the Linux `screen`-style multiplexer model.
// ScreenManager runs a single input loop and delegates each command line to the active screen,
// which returns a ScreenAction telling the manager what to do next (stay / attach / detach / quit).
class Screen;

struct ScreenAction {
    enum Type { Stay, Push, Pop, Quit };
    Type                    type = Stay;
    std::shared_ptr<Screen> next;          // the screen to attach, for Push

    static ScreenAction stay()                          { return { Stay, nullptr }; }
    static ScreenAction push(std::shared_ptr<Screen> s) { return { Push, std::move(s) }; }
    static ScreenAction pop()                           { return { Pop,  nullptr }; }
    static ScreenAction quit()                          { return { Quit, nullptr }; }
};

class Screen {
public:
    virtual ~Screen() = default;

    virtual std::string name()   const = 0;   // registry key / identifier
    virtual std::string prompt() const = 0;   // shell prompt shown before each input line

    virtual void onEnter() {}                 // called when this screen becomes the active one

    // Interpret one tokenized command line; return what the manager should do next.
    virtual ScreenAction handleCommand(const std::vector<std::string>& args) = 0;
};
