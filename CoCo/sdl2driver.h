#ifndef __SDL2DRIVER_H__
#define __SDL2DRIVER_H__

#include <SDL2/SDL.h>
#include <agar/gui/drv.h>

typedef struct ag_sdl2_driver
{
    struct ag_driver_mw _inherit;
    Uint32 mwflags;
    SDL_Window *w;
    SDL_Renderer *r;
    SDL_PixelFormat *pf;
    Uint32 f;
    Uint32 wid;
} AG_DriverSDL2Ghost;

#endif
