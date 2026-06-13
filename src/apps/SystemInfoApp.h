#pragma once
#include "compositor/Window.h"

namespace apps {

class SystemInfoApp : public compositor::Window {
public:
    SystemInfoApp();
    void draw() override;

private:
    float vol_{ 72.0f };
};

} // namespace apps
