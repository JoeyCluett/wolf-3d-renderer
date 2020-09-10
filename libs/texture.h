#pragma once

//
// a texture class specifically for use with this raycasting engine
//
// the image files these textures accept are ASCII encoded strings derived from the Python Image Library
//

#include <iostream>
#include <fstream>
#include <string>

#include <SDL/SDL.h>

struct texture_t {

    unsigned int* pixel_data;
    int h;
    int w;

    // ctor, no dimensions. that information is in the file
    texture_t(std::string filename, SDL_Surface* surface);

    // open image file associated with this image and load pixel data
    void init_with(std::string filename, SDL_Surface* surface);

    // pixel data is accessed as [y][x]
    unsigned int* operator[](int idx);
    unsigned int* row(int idx);
};

unsigned int* texture_t::row(int idx) {
    return this->pixel_data + (idx * this->w);
}

unsigned int* texture_t::operator[](int idx) {
    return this->row(idx);
}

texture_t::texture_t(std::string filename, SDL_Surface* surface) {
    this->init_with(filename, surface);
}

// open image file associated with this image and load pixel data
void texture_t::init_with(std::string filename, SDL_Surface* surface) {
    std::ifstream is(filename);

    std::string h_txt, w_txt;
    int h, w;
    is >> w_txt >> w >> h_txt >> h;
    long int sz = h * w;
    this->pixel_data = new unsigned int[sz];

    int r, g, b;

    for(long int i = 0; i < sz; i++) {
        is >> r >> g >> b;
        this->pixel_data[i] = SDL_MapRGB(surface->format, 3*r/4, 3*g/4, 3*b/4);
    }

    this->w = w;
    this->h = h;

}


