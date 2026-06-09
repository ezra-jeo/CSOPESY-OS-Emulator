#include "WindowManager.h"

namespace compositor {

void WindowManager::drawWindows() {
    for (auto& w : windows_) {
        if (w->isOpen())
            w->draw();
    }
}

} // namespace compositor
