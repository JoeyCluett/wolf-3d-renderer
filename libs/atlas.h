#pragma once

#include <vector>
#include <SDL/SDL.h>
#include "linear_map.h"
#include "texture.h"

struct texture_atlas_t {

    std::vector<texture_t> textures;

    // number of images, not pixels
    int w;
    int h;

    // texture_atlas_t expects data to already be in transposed form
    // user needs to be aware of this
    texture_atlas_t(std::string filename, int w, int h, SDL_Surface* surface);

    // 0,0 is upper left corner
    // no error checking is performed as i assume the user 
    // will always know what they are doing
    texture_t* get(int x, int y);

};

texture_t* texture_atlas_t::get(int x, int y) {
    int idx = (this->w * y) + x;
    return this->textures.data() + idx;
}

texture_atlas_t::texture_atlas_t(std::string filename, int w, int h, SDL_Surface* surface) {

    this->w = w;
    this->h = h;

    auto* tex = new texture_t(filename, surface);

    float image_width  = float(tex->w) / w;
    float image_height = float(tex->h) / h;

    for(int y = 0; y < h; y++) {
        for(int x = 0; x < w; x++) {

            std::vector<unsigned int> image_data;

            int start_x = map_float(x, 0, w, 0.0f, tex->w);
            int start_y = map_float(y, 0, h, 0.0f, tex->h);

            int end_x = float(start_x) + image_width;
            int end_y = float(start_y) + image_height;

            // clamp to ensure
            start_x = clamp_int(start_x, 0, tex->w-1);
            start_y = clamp_int(start_y, 0, tex->h-1);
            end_x   = clamp_int(end_x,   0, tex->w-1);
            end_y   = clamp_int(end_y,   0, tex->h-1);

            // the 4 ints above describe the corners of one complete image
            // iterate across this sub-section
            for(int img_y = start_y; img_y < end_y; img_y++) {

                // based on y-coordinate, find proper row
                unsigned int* row_data = tex->row(img_y);

                // take data from chunk within row
                for(int img_x = start_x; img_x < end_x; img_x++)
                    image_data.push_back(row_data[img_x]);                    
            }

            // sub-image data is now stored in image_data vector
            this->textures.emplace_back(
                    image_data, 
                    (end_y - start_y), 
                    (end_x - start_x));
        }
    }    

    tex->free_texture_memory();
    delete tex;
}


