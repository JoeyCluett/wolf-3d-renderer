#pragma once

#include <iostream>
#include <SDL/SDL.h>
#include "texture.h"

struct block_t {

    int type;

    static const int b_none    = 0;
    static const int b_texture = 1;
    static const int b_color   = 2;
    
    union {
        texture_t* tex;
        unsigned int color;
    };

    // useless default ctor
    block_t(void) {
        this->type = block_t::b_none;
    };

    block_t(const block_t& b) {
        this->type = b.type;
        this->tex = b.tex; // tex is the larger of the two union types
    }

    block_t(texture_t* tex) {
        this->type = block_t::b_texture;
        this->tex = tex;
    }

    block_t(unsigned int color) {
        this->type = block_t::b_color;
        this->color = color;
    }

};

