#pragma once

#include "texture.h"

struct transparent_texture_t {
    texture_t* tex;
    unsigned int opaque_color;
};
