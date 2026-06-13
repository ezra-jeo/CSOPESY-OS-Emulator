#define STB_IMAGE_IMPLEMENTATION
#include "Texture.h"
#include <stb_image.h>
#include <GLFW/glfw3.h>
#include <cstdio>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

namespace core {

Texture::~Texture() {
    if (id != 0) glDeleteTextures(1, &id);
}

Texture::Texture(Texture&& other) noexcept : id(other.id), w(other.w), h(other.h) {
    other.id = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if (this != &other) {
        if (id != 0) glDeleteTextures(1, &id);
        id = other.id; w = other.w; h = other.h;
        other.id = 0;
    }
    return *this;
}

// NOTE: stb_image here only decodes trusted assets bundled with the app.
// stb_image has had heap-overflow CVEs on malformed input — if this ever loads
// user-supplied images, treat the path/data as untrusted and sandbox/validate.
Texture loadTexture(const char* path) {
    Texture t;
    int channels;
    unsigned char* data = stbi_load(path, &t.w, &t.h, &channels, 4);
    if (!data) {
        fprintf(stderr, "[Texture] Failed to load \"%s\": %s\n", path, stbi_failure_reason());
        return t;
    }

    glGenTextures(1, &t.id);
    glBindTexture(GL_TEXTURE_2D, t.id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t.w, t.h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    stbi_image_free(data);
    return t;
}

} // namespace core
