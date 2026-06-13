#pragma once

namespace core { struct Texture; }

namespace shell {

struct Desktop {
    // Stateless: called once per frame. Wallpaper is owned by the Compositor.
    static void draw(const core::Texture& wallpaper);
};

} // namespace shell
