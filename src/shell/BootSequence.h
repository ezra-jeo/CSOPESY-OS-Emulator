#pragma once
#include <imgui.h>

namespace shell {

class BootSequence {
public:
    enum class State { Bios, Splash, Loading, Done };

    void update(float dt);
    void draw();
    bool isDone() const { return state_ == State::Done; }

private:
    State state_{ State::Bios };
    float timer_{ 0.0f };
    float loadProgress_{ 0.0f };

    void skip() { state_ = State::Done; }
};

} // namespace shell
