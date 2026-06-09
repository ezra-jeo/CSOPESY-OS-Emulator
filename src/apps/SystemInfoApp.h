#pragma once
#include "compositor/Window.h"

namespace apps {

class SystemInfoApp : public compositor::Window {
public:
    SystemInfoApp();
    void draw() override;
};

} // namespace apps
