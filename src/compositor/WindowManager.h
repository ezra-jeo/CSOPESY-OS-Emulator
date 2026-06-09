#pragma once
#include <vector>
#include <memory>
#include "Window.h"

namespace compositor {

class WindowManager {
public:
    // Takes ownership of a window; returns raw pointer for callers to keep.
    template<typename T, typename... Args>
    T* add(Args&&... args) {
        auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = ptr.get();
        windows_.push_back(std::move(ptr));
        return raw;
    }

    // Draw all open windows.
    void drawWindows();

private:
    std::vector<std::unique_ptr<Window>> windows_;
};

} // namespace compositor
