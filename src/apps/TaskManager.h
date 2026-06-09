#pragma once
#include "compositor/Window.h"
#include <vector>
#include <string>

namespace apps {

struct ProcessRow {
    std::string name;
    float       cpu;   // percent
    int         memory; // MB
    std::string status;
};

class TaskManager : public compositor::Window {
public:
    TaskManager();
    void draw() override;

private:
    std::vector<ProcessRow> processes_;
};

} // namespace apps
