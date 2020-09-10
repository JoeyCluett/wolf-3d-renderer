#pragma once

#include <iostream>
#include <SDL/SDL.h>
#include "texture.h"

struct block_t {

    int type;

    static const int b_texture = 0;
    static const int b_color = 1;

    union {
        texture_t* tex;
        unsigned int color;
    };

    // useless default ctor
    block_t(void) {};

    block_t(texture_t* tex);
    block_t(unsigned int color);

};

block_t::block_t(texture_t* tex) {
    this->type = block_t::b_texture;
    this->tex = tex;
}

block_t::block_t(unsigned int color) {
    this->type = block_t::b_color;
    this->color = color;
}

