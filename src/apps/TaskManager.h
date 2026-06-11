#pragma once
#include "compositor/Window.h"
#include <vector>
#include <string>

namespace apps {

struct ProcessRow {
    std::string name;
    float       cpu;
    int         memory;
    std::string status;
};

class TaskManager : public compositor::Window {
public:
    TaskManager();
    void draw() override;

private:
    void drawProcesses();
    void drawPerformance();

    std::vector<ProcessRow> processes_;
    int   selectedRow_{ -1 };

    // Performance rolling buffers (sampled at ~10 Hz)
    float cpuHist_[90]{};
    float memHist_[90]{};
    int   histOffset_{ 0 };
    float plotAccum_{ 0.0f };
    float simTime_{ 0.0f };
};

} // namespace apps
