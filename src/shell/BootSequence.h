#pragma once

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
};

} // namespace shell
