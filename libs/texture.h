#pragma once

//
// a texture class specifically for use with this raycasting engine
//
// the image files these textures accept are ASCII encoded strings derived from the Python Image Library
//

#include <stdio.h>
#include <stdlib.h>

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
    void init_with_ascii(std::string filename, SDL_Surface* surface);
    void init_with_binary(std::string filename, SDL_Surface* surface);

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
    int idx = filename.find_last_of('.');
    std::string file_ending = filename.substr(idx);

    if(file_ending == ".pil")
        this->init_with_ascii(filename, surface);
    else if(file_ending == ".bin")
        this->init_with_binary(filename, surface);
    else
        throw std::runtime_error("invalid texture file '" + filename + "'");
}

void texture_t::init_with_binary(std::string filename, SDL_Surface* surface) {

    int wh_buf[2];
    int w, h;

    FILE* fptr = fopen(filename.c_str(), "rb");
    int _ = fread(wh_buf, 8, 1, fptr);

    w = wh_buf[0];
    h = wh_buf[1];

    int r, g, b;

    size_t npixels = w * h;

    this->pixel_data = new unsigned int[npixels];

    for(size_t i = 0; i < npixels; i++) {

        unsigned char buf[3];
        _ = fread(buf, 3, 1, fptr);

        r = buf[0];
        g = buf[1];
        b = buf[2];

        this->pixel_data[i] = SDL_MapRGB(surface->format, r, g, b);
    }

    this->w = w;
    this->h = h;

    fclose(fptr);
}

// open image file associated with this image and load pixel data
void texture_t::init_with_ascii(std::string filename, SDL_Surface* surface) {
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

    is.close();
}


