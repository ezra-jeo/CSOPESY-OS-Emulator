#pragma once

namespace core {

// Owns a GL texture handle. Move-only so the handle is never double-freed;
// the destructor releases it (caller must ensure a GL context is still current
// at destruction — see Compositor, which owns its textures within run()).
struct Texture {
    unsigned int id{ 0 };
    int w{ 0 }, h{ 0 };

    Texture() = default;
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool valid() const { return id != 0; }
};

Texture loadTexture(const char* path);

} // namespace core
