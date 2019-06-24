#include <SDL2/SDL.h>
#include "dma.h"

static long long MasterClock = 0;
static long long OneFrame;
static long long OneMs;

void GetHighResTicks(ushort clockref)
{
    if (MasterClock == 0)
    {
        MasterClock = SDL_GetPerformanceFrequency();
        OneFrame = MasterClock / 60;
        OneMs = MasterClock / 1000;
    }

    long long CurrentTime = SDL_GetPerformanceCounter();

    Data ticks;

    ticks.lval = CurrentTime / OneMs;

    WriteCoCo4bytes(clockref, ticks);
}