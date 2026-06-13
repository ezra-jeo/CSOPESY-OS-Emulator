#pragma once

namespace shell {

// Phase 5 — BIOS text crawl → CSOPESY ASCII splash → loading bar → desktop.
// Stub: not wired into the main loop yet.
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
