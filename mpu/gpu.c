#include <stdio.h>
#include "mpu.h"

static unsigned short ScreenAddress;
static unsigned short ScreenWidth;
static unsigned short ScreenHeight;
static unsigned short ScreenPitch;
static unsigned short BitsPerPixel;
static unsigned short PixelsPerByte;
static unsigned short Color;


void SetScreen(unsigned short address, unsigned short width, unsigned short height, unsigned short bitsperpixel)
{
    ScreenAddress = address;
    ScreenWidth  = width;
    ScreenHeight = height;
    BitsPerPixel = bitsperpixel;
    PixelsPerByte = (8 / BitsPerPixel);
    ScreenPitch = width / PixelsPerByte;
}

void SetColor(unsigned short color)
{
    Color = color;
}

void SetPixel(unsigned short x, unsigned short y)
{
    unsigned short pixaddr = ScreenAddress + (y * ScreenPitch) + x;
}