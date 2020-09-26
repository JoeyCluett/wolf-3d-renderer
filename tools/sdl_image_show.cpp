#include <iostream>
#include <SDL/SDL.h>
#include "../libs/texture.h"

using namespace std;

int main(int argc, char* argv[]) {


    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_Surface* surface = SDL_SetVideoMode(800, 600, 32, SDL_HWSURFACE | SDL_DOUBLEBUF);

    texture_t* tex = new texture_t("../assets/brick-texture.pil", surface);

    int max_x = tex->w;
    int max_y = tex->h;

    bool looping = true;
    while(looping) {

        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT)
                looping = false;


            for(int y = 0; y < max_y; y++) {
                for(int x = 0; x < max_x; x++) {

                    SDL_Rect r;
                    r.x = x;
                    r.y = y;
                    r.h = 1;
                    r.w = 1;

                    SDL_FillRect(surface, &r, tex->row(y)[x]);                    

                }
            }

            SDL_Flip(surface);
            SDL_Delay(20);

        }

    }

    return 0;
}
