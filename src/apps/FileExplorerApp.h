#pragma once
#include "compositor/Window.h"

namespace apps {

class FileExplorerApp : public compositor::Window {
public:
    FileExplorerApp();
    void draw() override;

private:
    int selectedDir_{ 0 };
    int selectedFile_{ -1 };
    char searchBuf_[128]{};
};

} // namespace apps
