#pragma once
#include <string>

namespace compositor {

class Window {
public:
    explicit Window(std::string title) : title_(std::move(title)) {}
    virtual ~Window() = default;

    virtual void draw() = 0;

    bool isOpen()  const { return open_; }
    bool isFocused() const { return focused_; }
    void setOpen(bool v)   { open_ = v; }
    void toggle()          { open_ = !open_; }

    void requestFocus() { open_ = true; focusRequested_ = true; }

    const std::string& title() const { return title_; }

protected:
    std::string title_;
    bool open_{ false };
    bool focused_{ false };
    bool focusRequested_{ false };
};

} // namespace compositor
