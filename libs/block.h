#pragma once

#include <iostream>
#include <SDL/SDL.h>
#include "texture.h"
<<<<<<< HEAD
#include "transparent_texture.h"
=======
>>>>>>> master

struct block_t {

    int type;

    static const int b_none    = 0;
    static const int b_texture = 1;
    static const int b_color   = 2;
<<<<<<< HEAD
    static const int b_opq_tex = 4; // texture with transparency
=======
>>>>>>> master
    
    union {
        texture_t* tex;
        unsigned int color;
<<<<<<< HEAD
        transparent_texture_t* transp_tex;
=======
>>>>>>> master
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

<<<<<<< HEAD
    block_t(transparent_texture_t* ttex) {
        this->type = block_t::b_opq_tex;
        this->transp_tex = ttex;
    }

=======
>>>>>>> master
    block_t(unsigned int color) {
        this->type = block_t::b_color;
        this->color = color;
    }

};

