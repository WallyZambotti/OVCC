#include <stdio.h>
#include "mpu.h"

static unsigned short ScreenAddress;
static unsigned short ScreenWidth;
static unsigned short ScreenHeight;
static unsigned short ScreenPitch;
static unsigned short BitsPerPixel;
static unsigned short PixelsPerByte;
static unsigned short PPBshift;
static unsigned short Color;

unsigned char pixelmasks[4][8] = 
{
    {
        0xff
    },
    {
        0xf0, 0x0f
    },
    {
        0xC0, 0x30, 0x0c, 0x03
    },
    {
        0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01
    }
}

void SetScreen(unsigned short address, unsigned short width, unsigned short height, unsigned short bitsperpixel)
{
    ScreenAddress = address;
    ScreenWidth  = width;
    ScreenHeight = height;
    BitsPerPixel = bitsperpixel;
    PixelsPerByte = (8 / BitsPerPixel);
    ScreenPitch = width / PixelsPerByte;

    PPBshift = 0;
    for(unsigned short int PPB = PixelsPerByte ; PPB ; PPB>>1)
    {
        PPBshift++;
    }
    PPBshift--;
}

void SetColor(unsigned short color)
{
    Color = color;
}

void SetPixel(unsigned short x, unsigned short y)
{
    unsigned short pixaddr = ScreenAddress + (y * ScreenPitch) + (x>>PPBshift);
    unsigned short xmodPPB = x%PixelsPerByte;
    unsigned char pixmask = pixelmasks[PPBshift][xmodPPB];
    unsigned char pixelbyte = MemRead8(pixaddr) & (pixmask^0xff);
    pixelbyte |= Color<<(BitsPerPixel*(PixelsPerByte-xmodPPB-1));
    MemWrite8(pixelbyte, pixaddr);
}