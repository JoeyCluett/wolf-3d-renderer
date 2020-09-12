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

using namespace std;

float map_float(float input, float input_start, float input_end, float output_start, float output_end) {
    return output_start + ((output_end - output_start) / (input_end - input_start)) * (input - input_start);
}

double map_double(double input, double input_start, double input_end, double output_start, double output_end) {
    return output_start + ((output_end - output_start) / (input_end - input_start)) * (input - input_start);
}

unsigned long int get_timestamp(void) {
    timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000L) + tv.tv_usec;
}

// calculate_crutch_adj returns one of these structure which contains 
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
                        iters, // may pass one less than the original because this is 
                               // the result of doing one iteration...
                        xmid_int, ymid_int,
                        xnear, ynear,
                        xfar, yfar, 
                        env);
            }
        }
        // otherwise, continue with the algorithm as usual
        // ...

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

    const int scaniters     = 32;
    const float scandist    = 30.0f;
    const float fov         = M_PI_2;
    const float fov_2       = fov / 2.0f;
    const float screen_dist = 1.0f;
    const float crutch_dist = 20.0f; // any intersection below this goes through extra processing
    const float lateral_speed = 3.0f;

    world env;
    float* scanline = new float[800];
    float* scanline_adjust = new float[800];
    pair<float, float>* projected_intercepts = new pair<float, float>[800]; // the exact location within the map that this line intersected
    pair<int, int>* block_intercept = new pair<int, int>[800];

    // colors work a bit differently now
    //unsigned int* color_table = new unsigned int[800];
    block_t** color_table = new block_t*[800];

    for(int i = 0; i < 800; i++) {
        double opp = map_double(i, 0, 800, -1.0f, 1.0f);
        double theta = atan(opp / 1.0f);
        scanline_adjust[i] = cos(theta);
    }

    float player_x = 4.5;
    float player_y = 4.5;
    float direction = 0.0f;

    //float player_x  = 12.0775f;
    //float player_y  = 6.8788f;
    //float direction = 46.32f;

    SDL_Surface* surface = SDL_SetVideoMode(800, 600, 32, SDL_DOUBLEBUF | SDL_HWSURFACE );

    //unsigned int black = SDL_MapRGB(surface->format, 0, 0, 0);
    block_t* black = new block_t(SDL_MapRGB(surface->format, 0, 0, 0));

    texture_t* bricks    = new texture_t("assets/brick-texture.pil", surface);
    texture_t* shrubs    = new texture_t("assets/shrubbery.pil", surface);
    texture_t* painted   = new texture_t("assets/hand-painted.pil", surface);
    texture_t* paper     = new texture_t("assets/paper.pil", surface);
    texture_t* spaceship = new texture_t("assets/spaceship.pil", surface);

    // generate blocks in the environment
    for(int i = 0; i < 20; i++) {
        env[{ i,  0 }] = block_t( paper );
        env[{ i, 19 }] = block_t( bricks );
        env[{ 0,  i }] = block_t( painted );
        env[{ 19, i }] = block_t( shrubs );
    }

    env[{ 9,   9 }] = block_t( spaceship );
    env[{ 9,  10 }] = block_t( spaceship );
    env[{ 10,  9 }] = block_t( spaceship );
    env[{ 10, 10 }] = block_t( spaceship );

    struct {
        bool up    = false;
        bool down  = false;
        bool left  = false;
        bool right = false;
    } input_flags;

    SDL_WarpMouse(400, 300);
    SDL_ShowCursor(SDL_DISABLE);

    double start_loop_time = double(get_timestamp() / 1000);

    int attention_mouse_motion = 1;

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
                if(attention_mouse_motion) {
                    direction += (float(e.motion.xrel) / 200.0f);

                    // ensure theta always stays in [ 0.0, 2*pi ]
                    if(direction > (2.0 * M_PI)) direction -= (2.0 * M_PI);
                    else if(direction < 0.0) direction += (2.0 * M_PI);
                }
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

        // find projections for every vertical scan line
        for(int i = 0; i < 800; i++) {

            // prep the results area
            color_table[i] = NULL;

            // angle calculation works a lil differently
            float delta;

            {
                double opp = map_double(i, 0, 800, -1.0f, 1.0f);
                double theta = atan(opp / 1.0f);
                delta = theta + direction;
            }

            float prev_x = player_x;
            float prev_y = player_y;

            for(int k = 0; k < scaniters; k++) {

                // projected distance
                const float proj = map_float(k, 0.0f, scaniters, 0.0f, scandist);

                float tmp_x = player_x + proj*cosf(delta);
                float tmp_y = player_y + proj*sinf(delta);

                int int_x = roundf(tmp_x);
                int int_y = roundf(tmp_y);

                auto iter = env.find({ int_x, int_y });
                if(iter != env.end()) {
                                    
                    if(proj < crutch_dist) {
                        auto crutch_point = calculate_crutch_adjustment(
                                10, // do three iterations
                                int_x, int_y,
                                prev_x, prev_y,
                                tmp_x, tmp_y,
                                env);

                        float delta_x = player_x - crutch_point.first;
                        float delta_y = player_y - crutch_point.second;

                        // calculate adjusted projection distance
                        scanline[i] = sqrtf(delta_x*delta_x + delta_y*delta_y);
                        color_table[i] = &crutch_point.third->second;
                        projected_intercepts[i] = 
                            { 
                                crutch_point.first, 
                                crutch_point.second 
                            };
                        block_intercept[i] = crutch_point.third->first;
                           
                    }
                    else {
                        scanline[i] = proj; //  - scanline_adjust[i];
                        color_table[i] = &iter->second;
                        projected_intercepts[i] = { tmp_x, tmp_y };
                        block_intercept[i] = { int_x, int_y };
                    }

                    break;
                }

                prev_x = tmp_x;
                prev_y = tmp_y;

                // failed to find wall intersection
                color_table[i] = black;
                scanline[i] = -1;
            }
        }

        // apply ceiling and floor colors
        {
            SDL_Rect r;
            r.x = 0;
            r.y = 0;
            r.w = 800;

            r.h = 600;
            SDL_FillRect(surface, &r, SDL_MapRGB(surface->format, 200, 200, 200));

            r.h = 300;
            SDL_FillRect(surface, &r, SDL_MapRGB(surface->format, 100, 100, 100));
        }

        //std::cout << "x: " << player_x << ", y: " << player_y << ", theta: " << direction << std::endl;

        for(int i = 0; i < 800; i++) {
            if(scanline[i] > scanline_adjust[i] && color_table[i]) {

                SDL_Rect r;
                r.x = i;
                //r.h = 600.0f / (scanline[i] * scanline_adjust[i]);
                r.h = 1000.0f / (scanline[i] * scanline_adjust[i]);
                r.y = 300 - (r.h / 2);
                r.w = 2;

                // our rendering technique changes depending on whether we have a texture here or a solid color
                switch(color_table[i]->type) {
                    case block_t::b_texture:
                        {
                            texture_t* texptr = color_table[i]->tex;

                            // find which side we are on
                            float ind_x = projected_intercepts[i].first  - block_intercept[i].first;
                            float ind_y = projected_intercepts[i].second - block_intercept[i].second;

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

                            unsigned int* row = color_table[i]->tex->row(row_index);

                            for(int j = 0; j < r.h; j++) {
                                int col_index = map_float(j, 0, r.h, 0, texptr->w);

                                SDL_Rect r1;
                                r1.w = 2;
                                r1.h = 1;
                                r1.x = i;
                                r1.y = r.y + j;
                                SDL_FillRect(surface, &r1, row[col_index]);
                            }

                        }
                        break;
                    case block_t::b_color:
                        {
                            SDL_FillRect(surface, &r, color_table[i]->color);
                        }
                        break;
                    default:
                        break; // nothing stops the 'march' of the raycaster!
                }
   
            }
        }

        SDL_Flip(surface);

        auto end_timestamp = get_timestamp();
        auto delta_time = end_timestamp - start_timestamp;
        cout << "frame time: " << (delta_time/1000L) << " ms\n";

        start_loop_time = start_time;

    }

    SDL_Quit();

    return 0;
}
