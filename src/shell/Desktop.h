#pragma once

namespace core { class Application; }

namespace shell {

struct Desktop {
    // Stateless: called once per frame.
    static void draw(core::Application& app);
};

} // namespace shell
