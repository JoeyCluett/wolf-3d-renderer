#include <iostream>
#include <map>
#include <utility>
#include <math.h>
#include <sys/time.h>
#include <SDL/SDL.h>

// libs to make my life easier
#include "libs/block.h"
#include "libs/texture.h"
#include "libs/world.h"
#include "libs/linear_map.h"
#include "libs/atlas.h"
#include "libs/transparent_texture.h"

//#define DEBUG_SHOW_BOUNDS
//#define DEBUG_SHOW_CENTERLINE
//#define DEBUG_PRINT_FRAME_TIME
//#define FISHEYE_EFFECT

using namespace std;

struct point_t {
    float x;
    float y;
};

unsigned long int get_timestamp(void) {
    timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000L) + tv.tv_usec;
}

bool test_segment_block_collide(int map_x, int map_y, float near_x, float near_y, float far_x, float far_y, point_t& pt) {

    // shamelessly taken from the Wikipedia for geometric intersections
    float& x1 = near_x;
    float& y1 = near_y;
    float& x2 = far_x;
    float& y2 = far_y;

    // top line
    float x3 = float(map_x) - 0.5f;
    float y3 = float(map_y) - 0.5f;
    float x4 = float(map_x) + 0.5f;
    float y4 = float(map_y) + 0.5f;

    // i dont have a clue why the following works...

    float Px = (x1*y2 - y1*x2)*(x3 - x4) - (x1 - x2)*(x3*y4 - y3*x4);
    Px /= ( (x1 - x2)*(y3 - y4) - (y1 - y2)*(x3 - x4) );

    float Py = (x1*y2 - y1*x2)*(y3 - y4) - (y1 - y2)*(x3*y4 - y3*x4);
    Py /= ( (x1 - x2)*(y3 - y4) - (y1 - y2)*(x3 - x4) );

    pt.x = Px;
    pt.y = Py;

    // point of intersection must sit within a certain range
    bool collides = (Px > x3) && (Px < x4) && (Py > y3) && (Py < y4);

    // stop here if we have a collision
    if(collides) return true;

    x3 = float(map_x) - 0.5f;
    y3 = float(map_y) + 0.5f;
    x4 = float(map_x) + 0.5f;
    y4 = float(map_y) - 0.5f;

    // test other segment
    Px = (x1*y2 - y1*x2)*(x3 - x4) - (x1 - x2)*(x3*y4 - y3*x4);
    Px /= ( (x1 - x2)*(y3 - y4) - (y1 - y2)*(x3 - x4) );

    Py = (x1*y2 - y1*x2)*(y3 - y4) - (y1 - y2)*(x3*y4 - y3*x4);
    Py /= ( (x1 - x2)*(y3 - y4) - (y1 - y2)*(x3 - x4) );

    pt.x = Px;
    pt.y = Py;

    collides = (Px > x3) && (Px < x4) && (Py > y4) && (Py < y3);
    return collides;
}

// calculate_crutch_adj returns one of these structures which contains 
// adjusted projection data and an iterator to corrected color data
struct c_point {
    float first;
    float second;
    world::iterator third;
};

auto calculate_crutch_adjustment(
        int iters,
        int xtarget, int ytarget, 
        float xnear, float ynear, 
        float xfar, float yfar,
        world& env) -> c_point {

    // this should always exist, otherwise this function wouldnt be called in 
    // the first place. maybe this should just be passed as an argument
    auto iter = env.find({ xtarget, ytarget });

    for(int i = 0; i < iters; i++) {
        float xmid = (xfar + xnear) / 2.0f;
        float ymid = (yfar + ynear) / 2.0f;

        // round midpoints to integers and see if another block is closer. 
        // if so, re-call this function with new block coordinates
        int xmid_int = round(xmid);
        int ymid_int = round(ymid);

        if(xmid_int != xtarget || ymid_int != ytarget) {
            auto iter = env.find({ xmid_int, ymid_int });
            if(iter != env.end()) {
                // there exists a block that is closer
                // recursively call function with updated target information
                return calculate_crutch_adjustment(
                        iters - 1, // pass one less than the original because this is 
                                   // the result of doing one iteration...
                        xmid_int, ymid_int,
                        xnear, ynear,
                        xfar, yfar, 
                        env);
            }
        }
        
        if(roundf(xmid) == xtarget && roundf(ymid) == ytarget) {
            // midpoint collides with current block
            xfar = xmid;
            yfar = ymid;
        }
        else {
            // midpoint sits outside of 
            xnear = xmid;
            ynear = ymid;
        }
    }

    // return the final midpoint
    return {
        (xfar + xnear) / 2.0f,
        (yfar + ynear) / 2.0f,
        iter
    };

}


int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_EVERYTHING);

    const int scaniters     = 31;
    const float scandist    = 30.0f;
    const float fov         = M_PI_2;
    const float lateral_speed = 3.0f;

    world env;

    float* scanline = new float[800];
    float* const_scanline_adjust = new float[800];
    //pair<float, float>* projected_intercepts = new pair<float, float>[800]; // the exact location within the map that this line intersected
    //pair<int, int>* block_intercept = new pair<int, int>[800];

    for(int i = 0; i < 800; i++) {
        double opp = map_double(i, 0, 800, -0.1f, 0.1f);
        double theta = atan(opp / 0.1f);
        const_scanline_adjust[i] = cos(theta);
    }

    float player_x  = 4.5;
    float player_y  = 4.5;
    float direction = 0.0f;

    SDL_Surface* surface = SDL_SetVideoMode(800, 600, 32, SDL_DOUBLEBUF | SDL_HWSURFACE | SDL_FULLSCREEN);

    unsigned int pink = SDL_MapRGB(surface->format, 255, 105, 180);
    unsigned int red  = SDL_MapRGB(surface->format, 255, 0, 0);
    block_t* black = new block_t(SDL_MapRGB(surface->format, 0, 0, 0));

    // 11 different textures in this one file
    texture_atlas_t tat("assets/textures.pil", 1, 11, surface);

    /*
    texture_t* bricks    = new texture_t("assets/brick-texture.bin", surface);
    texture_t* oiledup   = new texture_t("assets/oil-up.bin",        surface);
    texture_t* shaggy    = new texture_t("assets/shaggy.bin",        surface);
    texture_t* pixeled   = new texture_t("assets/pixelated.bin",     surface);
    texture_t* shrubs    = new texture_t("assets/shrubbery.bin",     surface);
    texture_t* painted   = new texture_t("assets/hand-painted.bin",  surface);
    texture_t* paper     = new texture_t("assets/paper.bin",         surface);
    texture_t* spaceship = new texture_t("assets/spaceship.bin",     surface);
    texture_t* backrooms = new texture_t("assets/backrooms.bin",     surface);
    texture_t* ytho      = new texture_t("assets/ytho.bin",          surface);
    texture_t* wat       = new texture_t("assets/wat.bin",           surface);
    */

    texture_t* bricks    = tat.get(0, 0);
    texture_t* shaggy    = tat.get(0, 1);
    texture_t* pixeled   = tat.get(0, 2);
    texture_t* shrubs    = tat.get(0, 3);
    texture_t* painted   = tat.get(0, 4);
    texture_t* paper     = tat.get(0, 5);
    texture_t* spaceship = tat.get(0, 6);
    texture_t* ytho      = tat.get(0, 7);
    texture_t* wat       = tat.get(0, 8);

    transparent_texture_t barrel{ tat.get(0, 8) };
    transparent_texture_t pillar{ tat.get(0, 9) };
    transparent_texture_t lamp{   tat.get(0, 10) };

    // generate blocks in the environment
/*
    for(int i = 0; i < 20; i++) {
        env[{ i,  0 }] = block_t( paper );
        env[{ i, 19 }] = block_t( bricks );
        env[{ 0,  i }] = block_t( painted );
        env[{ 19, i }] = block_t( shrubs );
    }
*/

    for(int i = 0; i < 20; i++) {
        env[{ i,  0 }] = block_t( &barrel );
        env[{ i, 19 }] = block_t( &barrel );
        env[{ 0,  i }] = block_t( &pillar );
        env[{ 19, i }] = block_t( &pillar );
    }

    env[{ 9,   9 }] = block_t( spaceship );
    env[{ 9,  10 }] = block_t( spaceship );
    env[{ 10,  9 }] = block_t( spaceship );
    env[{ 10, 10 }] = block_t( spaceship );

    env[{ 15, 15 }] = block_t( pixeled );
    env[{ 14, 14 }] = block_t( pixeled );
    env[{ 13, 15 }] = block_t( pixeled );
    env[{ 14, 15 }] = block_t( shaggy );

    env[{ 4, 14 }] = block_t( ytho );
    env[{ 15, 4 }] = block_t( &barrel );

    struct {
        bool up    = false;
        bool down  = false;
        bool left  = false;
        bool right = false;
    } input_flags;

    SDL_WarpMouse(400, 300);
    SDL_ShowCursor(SDL_DISABLE);

    double start_loop_time = double(get_timestamp() / 1000);

    bool quit = false;
    while(!quit) {

        auto start_timestamp = get_timestamp();
        double start_time = double(get_timestamp() / 1000);
        float delta_loop_time = (start_time - start_loop_time) / 1000.0;

        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) {
                quit = true;
            }
            else if(e.type == SDL_KEYDOWN) {
                switch(e.key.keysym.sym) {
                    case SDLK_ESCAPE: quit = true; break;
                    case SDLK_w: input_flags.up    = true; break;
                    case SDLK_a: input_flags.left  = true; break;
                    case SDLK_s: input_flags.down  = true; break;
                    case SDLK_d: input_flags.right = true; break;
                    default: break;
                }
            }
            else if(e.type == SDL_KEYUP) {
                switch(e.key.keysym.sym) {
                    case SDLK_ESCAPE: quit = true; break;
                    case SDLK_w: input_flags.up    = false; break;
                    case SDLK_a: input_flags.left  = false; break;
                    case SDLK_s: input_flags.down  = false; break;
                    case SDLK_d: input_flags.right = false; break;
                    default: break;
                }
            }
            else if(e.type == SDL_MOUSEMOTION) {
                direction += (float(e.motion.xrel) / 200.0f);

                // ensure theta always stays in [ 0.0, 2*pi ]
                if(direction > (2.0 * M_PI)) direction -= (2.0 * M_PI);
                else if(direction < 0.0) direction += (2.0 * M_PI);
            }
        }

        // calculate new player position/orientation based on flag state
        if(input_flags.up) {
            player_x += delta_loop_time * lateral_speed * cosf(direction);
            player_y += delta_loop_time * lateral_speed * sinf(direction);
        }
        if(input_flags.down) {
            player_x -= delta_loop_time * lateral_speed * cosf(direction);
            player_y -= delta_loop_time * lateral_speed * sinf(direction);   
        }
        if(input_flags.left) {
            player_x += delta_loop_time * lateral_speed * cosf(direction - M_PI_2);
            player_y += delta_loop_time * lateral_speed * sinf(direction - M_PI_2);
        }
        if(input_flags.right) {
            player_x += delta_loop_time * lateral_speed * cosf(direction + M_PI_2);
            player_y += delta_loop_time * lateral_speed * sinf(direction + M_PI_2);
        }

        {

            SDL_Rect ceiling_tile;
            ceiling_tile.x = 0;
            ceiling_tile.y = 0;
            ceiling_tile.w = 800;
            ceiling_tile.h = 300;
            SDL_FillRect(surface, &ceiling_tile, SDL_MapRGB(surface->format, 222 / 2, 184 / 2, 135 / 2));
            ceiling_tile.y = 300;
            SDL_FillRect(surface, &ceiling_tile, SDL_MapRGB(surface->format, 255 / 2, 222 / 2, 173 / 2));

        }

        // find projections for every vertical scan line
        for(int i = 0; i < 800; i++) {

            // actual angle to project out at
            float delta;

            #ifndef FISHEYE_EFFECT

            {
                double opp = map_double(i, 0, 800, -1.0f, 1.0f);
                double theta = atan(opp / 1.0f);
                delta = theta + direction;
            }

            #else

            {
                delta = direction + map_double(i, 0, 800, -M_PI_2 / 2.0, M_PI_2 / 2.0);
            }

            #endif

            float prev_x = player_x;
            float prev_y = player_y;

            c_point cpt;

            bool found_intercept = false;

            for(int k = 1; k < scaniters && !found_intercept; k++) {

                const float near_projection = map_float(k-1, 0, scaniters, 0.0f, scandist);
                const float far_projection  = map_float(k,   0, scaniters, 0.0f, scandist);

                // trigonometry!!
                const float x_near = player_x + near_projection*cosf(delta);
                const float y_near = player_y + near_projection*sinf(delta);
                const float x_far  = player_x + far_projection*cosf(delta);
                const float y_far  = player_y + far_projection*sinf(delta);

                float x_intercept;
                float y_intercept;
                
                world::iterator world_iter;

                point_t pt;

                // gonna break a cardinal rule of programming and use ... *shudders*... a goto

                {
                    x_intercept = x_near;
                    y_intercept = y_near;
                    
                    world_iter = env.find({ roundf(x_intercept), roundf(y_intercept) });
                    if(world_iter != env.end()) {
                        if(test_segment_block_collide(
                                world_iter->first.first, 
                                world_iter->first.second, 
                                x_near, y_near, 
                                x_far, y_far, 
                                pt)) {
                            
                            goto found_world_block; // #1

                        }
                    }
                }

                {

                    x_intercept = x_far;
                    y_intercept = y_near;

                    world_iter = env.find({ roundf(x_intercept), roundf(y_intercept) });
                    if(world_iter != env.end()) {
                        if(test_segment_block_collide(
                                world_iter->first.first, 
                                world_iter->first.second, 
                                x_near, y_near, 
                                x_far, y_far, 
                                pt)) {
                            
                            goto found_world_block; // #2
                        }
                    }
                }

                {

                    x_intercept = x_near;
                    y_intercept = y_far;

                    world_iter = env.find({ roundf(x_intercept), roundf(y_intercept) });
                    if(world_iter != env.end()) {
                        if(test_segment_block_collide(
                                world_iter->first.first, 
                                world_iter->first.second, 
                                x_near, y_near, 
                                x_far, y_far, 
                                pt)) {
                            
                            goto found_world_block; // #3
                        }
                    }
                }

                // didnt find an intercept. skip to the next iteration
                // the goto's above skip this one line
                continue;

            // label for forbidden goto
            found_world_block:

                // if we make it to this point, it means we found an intercept
                // world_iter contains world intercept information

                found_intercept = true;

                // the exact point of intersection is in pt. pass it to calculate_crutch_adjustment to 
                // find the point of intersection closest to the player
                cpt = calculate_crutch_adjustment(
                        10, 
                        world_iter->first.first, 
                        world_iter->first.second, 
                        player_x, player_y, 
                        pt.x, pt.y, 
                        env);

            }

            // only render scanline if we found something to render
            if(found_intercept) {

                // calculate actual distance to new intercept
                float delta_x = player_x - cpt.first;
                float delta_y = player_y - cpt.second;

                float actual_distance = sqrtf(delta_x*delta_x + delta_y*delta_y);

                if(actual_distance < const_scanline_adjust[i] / 10.0f) {
                    continue; // skip the rest of the rendering algorithm
                }

                SDL_Rect r;
                r.x = i;
                r.h = 600.0f / (actual_distance * const_scanline_adjust[i]);
                r.y = 300 - (r.h / 2);
                r.w = 1;

                // our rendering technique changes depending on whether we have a texture here or a solid color
                switch(cpt.third->second.type) {
                    case block_t::b_texture:
                        {
                            texture_t* texptr = cpt.third->second.tex;

                            // find which side we are on
                            float ind_x = cpt.first  - cpt.third->first.first;
                            float ind_y = cpt.second - cpt.third->first.second;

                            float slope = ind_y / ind_x;
                            int row_index;

                            if(slope > -1.0f && slope < 1.0f) {
                                // side
                                row_index = map_float(ind_y, -0.5f, 0.5f, 0, texptr->h);
                            }
                            else {
                                // top/bottom
                                row_index = map_float(ind_x, -0.5f, 0.5f, 0, texptr->h);
                            }

                            //std::cout << " row[" << row_index << "/" << texptr->h << "]" << std::flush;
                            row_index = clamp_int(row_index, 0, texptr->h-1);

                            unsigned int* row = texptr->row(row_index);

                            for(int j = 0; j < r.h; j++) {
                                int col_index = map_float(j, 0, r.h, 0, texptr->w);

                                //std::cout << " col[" << col_index << "/" << texptr->w << "] " << std::flush;
                                col_index = clamp_int(col_index, 0, texptr->w - 1);

                                SDL_Rect r1;
                                r1.w = 1;
                                r1.h = 1;
                                r1.x = i;
                                r1.y = r.y + j;
                                SDL_FillRect(surface, &r1, row[col_index]);
                                //env.pixel(r.x, r.y) = row[col_index];
                            }

                        }
                        break;
                    case block_t::b_color:
                        {
                            SDL_FillRect(surface, &r, cpt.third->second.color);
                        }
                        break;
                    case block_t::b_opq_tex:
                        {
                            texture_t* texptr = cpt.third->second.transp_tex->tex;
                            unsigned int tcolor = cpt.third->second.transp_tex->opaque_color;

                            // find which side we are on
                            float ind_x = cpt.first  - cpt.third->first.first;
                            float ind_y = cpt.second - cpt.third->first.second;

                            float slope = ind_y / ind_x;
                            int row_index;

                            if(slope > -1.0f && slope < 1.0f) {
                                // side
                                row_index = map_float(ind_y, -0.5f, 0.5f, 0, texptr->h);
                            }
                            else {
                                // top/bottom
                                row_index = map_float(ind_x, -0.5f, 0.5f, 0, texptr->h);
                            }

                            row_index = clamp_int(row_index, 0, texptr->h-1);

                            unsigned int* row = texptr->row(row_index);

                            for(int j = 0; j < r.h; j++) {
                                int col_index = map_float(j, 0, r.h, 0, texptr->w);
                                col_index = clamp_int(col_index, 0, texptr->w - 1);

                                SDL_Rect r1;
                                r1.w = 1;
                                r1.h = 1;
                                r1.x = i;
                                r1.y = r.y + j;

                                unsigned int color_to_draw = row[col_index];
                                if(color_to_draw != tcolor)
                                    SDL_FillRect(surface, &r1, color_to_draw);
                            }
                        }
                        break;
                }

            #ifdef DEBUG_SHOW_BOUNDS
                SDL_Rect trim;
                trim.x = r.x;
                trim.y = r.y - 1;
                trim.h = 3;
                trim.w = 1;

                SDL_FillRect(surface, &trim, pink);

                trim.y = r.y - 1 + r.h;

                SDL_FillRect(surface, &trim, pink);
            #endif

            #ifdef DEBUG_SHOW_CENTERLINE

                trim.x = 0;
                trim.y = 300 - 1;
                trim.h = 2;
                trim.w = 800;

                SDL_FillRect(surface, &trim, red);
            #endif

            }

        }

        SDL_Flip(surface);

        auto end_timestamp = get_timestamp();
        auto delta_time = end_timestamp - start_timestamp;

        #ifdef DEBUG_PRINT_FRAME_TIME
        cout << "frame time: " << (delta_time/1000L) << " ms\n";
        #endif

        start_loop_time = start_time;

    }

    SDL_Quit();

    return 0;
}
