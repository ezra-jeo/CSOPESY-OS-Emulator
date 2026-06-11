#pragma once

namespace core {

struct Texture {
    unsigned int id{ 0 };
    int w{ 0 }, h{ 0 };
    bool valid() const { return id != 0; }
};

Texture loadTexture(const char* path);

} // namespace core
