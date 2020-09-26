#pragma once

//
// a texture class specifically for use with this raycasting engine
//
// the image files these textures accept are ASCII encoded strings derived from the Python Image Library
//

#include <stdio.h>
#include <stdlib.h>

#include <iostream>
<<<<<<< HEAD
#include <vector>
=======
>>>>>>> master
#include <fstream>
#include <string>

#include <SDL/SDL.h>

struct texture_t {

<<<<<<< HEAD
    // data fields are simple enough
=======
>>>>>>> master
    unsigned int* pixel_data;
    int h;
    int w;

    // ctor, no dimensions. that information is in the file
    texture_t(std::string filename, SDL_Surface* surface);

<<<<<<< HEAD
    // ctor for initializing texture with existing data
    // no checks are done to ensure data integrity
    texture_t(std::vector<unsigned int>& data, int h, int w);

    // what do you call a copy ctor that doesnt actually copy data?
    // this ctor doesnt actually move data either. it does, however, 
    // implement perfect forwarding...sorta
    texture_t(const texture_t& tex);

    // deletes the texture memory associated with this texture. 
    // this is because i do not want to add a destructor to this 
    // class as most texture_t structs will be sharing data while 
    // the program runs and implementing dtor and copy semantics 
    // is more effort than its worth
    void free_texture_memory(void);

    // open image file associated with this image and load pixel data
    // both file formats are custom
    void init_with_ascii(std::string filename,  SDL_Surface* surface);
    void init_with_binary(std::string filename, SDL_Surface* surface);

    // pixel data is accessed as [y][x]
    unsigned int* row(int idx);
    unsigned int* operator[](int idx); // just a wrapper over .row()
};

texture_t::texture_t(const texture_t& tex) {
    // share the data, as it is unlikely to get reallocated for the life of the program
    this->pixel_data = tex.pixel_data;

    // 'new' image needs to have same dimensions
    this->h = tex.h;
    this->w = tex.w;
}

void texture_t::free_texture_memory(void) {
    delete[] this->pixel_data;
}

texture_t::texture_t(std::vector<unsigned int>& data, int h, int w) {
    this->h = h;
    this->w = w;

    // allocate space for data and copy it over
    this->pixel_data = new unsigned int[ h * w ];
    memcpy(this->pixel_data, data.data(), h * w * 4);
}

=======
    // open image file associated with this image and load pixel data
    void init_with_ascii(std::string filename, SDL_Surface* surface);
    void init_with_binary(std::string filename, SDL_Surface* surface);

    // pixel data is accessed as [y][x]
    unsigned int* operator[](int idx);
    unsigned int* row(int idx);
};

>>>>>>> master
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


