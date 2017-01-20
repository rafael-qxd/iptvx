#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_mutex.h>

/* window args type */
struct create_window_args{
    int width;
    int height;
} typedef create_window_args;

/* SDL context type */
struct sdl_context{
    SDL_Surface *surf;
    SDL_mutex *mutex;
} typedef sdl_context;

/* window variables */
SDL_Thread *window_thread;
bool window_terminate;
sdl_context ctx;

/* creates the main window for this application */
int iptvx_create_window(int width, int height, 
                    void (*keyDownCallback)(int),
                    void (*startPlayCallback)(void*) ){
    SDL_Surface *screen, *overlay;
    SDL_Event event;

    /* set window terminate to false */
    window_terminate = false;

    /* initialise the SDL lib */
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTTHREAD) == -1){
        printf("Unable to initialize SDL\n");
    }

    /* create the SDL surface */
    ctx.surf = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 16, 0x001f, 0x07e0, 0xf800, 0);
    ctx.mutex = SDL_CreateMutex();

    int options = SDL_ANYFORMAT | SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_FULLSCREEN;
    screen = SDL_SetVideoMode(width, height, 32, options);
    if(!screen){
        printf("Unable to set video mode for SDL\n");
    }

    /* exec callback to call for player */
    (*startPlayCallback)(&ctx);

    while(!window_terminate){
        int keyPressed;

        /* poll SDL events and act accordingly */
        while(SDL_PollEvent(&event)){ 
            /* handle the SDL events */
            switch(event.type){
                /* kill the application */
                case SDL_QUIT:
                    window_terminate = true;
                    break;
                case SDL_KEYDOWN:
                    keyPressed = event.key.keysym.sym;
                    (*keyDownCallback)(keyPressed);
                    break;
            }

            /* handle pressed keys */
            switch(keyPressed){
                case SDLK_ESCAPE:
                    window_terminate = true;
                    break;
            }
        }

        /* Blitting the surface does not prevent it from being locked and
        * written to by another thread, so we use this additional mutex. */
        SDL_LockMutex(ctx.mutex);
        SDL_BlitSurface(ctx.surf, NULL, screen, NULL);
        //SDL_BlitSurface(overlay, NULL, screen, NULL);
        SDL_UnlockMutex(ctx.mutex);

        /* flush to screen */
        SDL_Flip(screen);
        SDL_Delay(10);
    }

    /* Close window and clean up SDL */
    SDL_DestroyMutex(ctx.mutex);
    SDL_FreeSurface(ctx.surf);
    SDL_FreeSurface(overlay);
    SDL_Quit();
}

extern void *iptvx_window_lock(void *data, void **p_pixels){
    sdl_context *ctx = data;

    SDL_LockMutex(ctx->mutex);
    SDL_LockSurface(ctx->surf);
    *p_pixels = ctx->surf->pixels;
    return NULL; /* picture identifier, not needed here */
}

extern void iptvx_window_unlock(void *data, void *id, void *const *p_pixels){
    sdl_context *ctx = data;

    SDL_UnlockSurface(ctx->surf);
    SDL_UnlockMutex(ctx->mutex);

    assert(id == NULL); /* picture identifier, not needed here */
}

extern void iptvx_window_display(void *data, void *id){
    /* LibVLC wants to display the video */
    (void) data;
    assert(id == NULL);
}