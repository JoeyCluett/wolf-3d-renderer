#include <iostream>
#include <map>
#include <utility>
#include <math.h>
#include <sys/time.h>
#include <SDL/SDL.h>

#define PERFORM_CRUTCH_ADJUSTMENT

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

#ifdef PERFORM_CRUTCH_ADJUSTMENT

// calculate_crutch_adj returns one of these structure which contains 
// adjusted projection data and an iterator to corrected color data
struct c_point {
    float first;
    float second;
    std::map<std::pair<int, int>, unsigned int>::iterator third;
};

auto calculate_crutch_adjustment(
        int iters,
        int xtarget, int ytarget, 
        float xnear, float ynear, 
        float xfar, float yfar,
        std::map<std::pair<int, int>, unsigned int>& env) -> c_point {

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

#endif

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_EVERYTHING);

    const int scaniters     = 35;
    const float scandist    = 30.0f;
    const float fov         = M_PI_2;
    const float fov_2       = fov / 2.0f;
    const float screen_dist = 1.0f;
    const float crutch_dist = 20.0f; // any intersection below this goes through extra processing
    const float lateral_speed = 3.0f;

    std::map<std::pair<int, int>, unsigned int> env;
    float* scanline = new float[800];
    float* scanline_adjust = new float[800];
    unsigned int* color_lookup_table = new unsigned int[256];
    unsigned int* color_table = new unsigned int[800];

    for(int i = 0; i < 800; i++) {
        double opp = map_double(i, 0, 800, -1.0f, 1.0f);
        double theta = atan(opp / 1.0f);
        scanline_adjust[i] = cos(theta);
    }

    float direction = 0.0f;
    float player_x = 4.5;
    float player_y = 4.5;

    SDL_Surface* surface = SDL_SetVideoMode(800, 600, 32, SDL_DOUBLEBUF | SDL_HWSURFACE | SDL_FULLSCREEN);

    unsigned int black = SDL_MapRGB(surface->format, 0, 0, 0);

    // generate blocks in the environment
    // just a solid border for now
    for(int i = 0; i < 20; i++) {
        env[{ i,  0 }] = SDL_MapRGB(surface->format, 200, 0, 0);
        env[{ i, 19 }] = SDL_MapRGB(surface->format, 200, 0, 0);
        env[{ 0,  i }] = SDL_MapRGB(surface->format, 0, 0, 200);
        env[{ 19, i }] = SDL_MapRGB(surface->format, 0, 0, 200);
    }

    for(int c = 0; c < 256; c++)
        color_lookup_table[c] = SDL_MapRGB(surface->format, c, c, c);

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
                    SDL_WarpMouse(400, 300);
                }
                attention_mouse_motion = 1 - attention_mouse_motion;
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

        //float lhs = direction - (fov_2);
        //float rhs = direction + (fov_2);

        float max_distance = -1.0f;
        float min_distance = 1000000.0f;

        // find projections for every vertical scan line
        for(int i = 0; i < 800; i++) {
            // angle calculation works a lil differently
            //float delta = map_float(i, 0.0f, 800.0f, lhs, rhs);
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
                
                    #ifdef PERFORM_CRUTCH_ADJUSTMENT
                    
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
                        color_table[i] = crutch_point.third->second;
                    }
                    else {
                        scanline[i] = proj; //  - scanline_adjust[i];
                        color_table[i] = iter->second;
                    }

                    #else

                    scanline[i] = proj;
                    color_table[i] = iter->second;

                    #endif

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

        //SDL_FillRect(surface, NULL, 0);

        for(int i = 0; i < 800; i++) {
            if(scanline[i] > scanline_adjust[i]) {
                SDL_Rect r;
                r.x = i;
                r.h = 600.0f / (scanline[i] * scanline_adjust[i]);
                r.y = 300 - (r.h / 2);
                r.w = 1;

                //int color_index = map_float(scanline[i], min_distance, max_distance, 0.0f, 255.0f);
                //SDL_FillRect(surface, &r, color_lookup_table[255 - color_index]);
                SDL_FillRect(surface, &r, color_table[i]);
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
