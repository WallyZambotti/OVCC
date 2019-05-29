#include <stdio.h>
#include "mpu.h"

static unsigned short ScreenAddress;
static unsigned short ScreenWidth;
static unsigned short ScreenPitch;
static unsigned short ScreenHeight;
static unsigned short ScreenEnd;
static unsigned short BitsPerPixel;
static unsigned short PixelsPerByte;
static short PPBshift;
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
};

void SetScreen(unsigned short address, unsigned short width, unsigned short height, unsigned short bitsperpixel)
{
    // fprintf(stderr, "SetScreen %x %d %d %d\n", address, width, height, bitsperpixel);

    if (bitsperpixel == 0) return ;

    ScreenAddress = address;
    ScreenWidth  = width;
    ScreenHeight = height;
    BitsPerPixel = bitsperpixel;
    PixelsPerByte = (8 / BitsPerPixel);
    ScreenPitch = width / PixelsPerByte;
    ScreenEnd = ScreenAddress + (ScreenPitch * ScreenHeight);

    PPBshift = -1;
    for(unsigned short int PPB = PixelsPerByte ; PPB ; PPB=PPB>>1) { PPBshift++; }

    // fprintf(stderr, "SetScreen %d %d %d %d\n", PixelsPerByte, ScreenPitch, ScreenEnd, PPBshift);
}

void SetColor(unsigned short color)
{
    // fprintf(stderr, "SetColor %d\n", color);
    Color = color;
}

void SetPixel(unsigned short x, unsigned short y)
{
    // fprintf(stderr, "SetPixel %d %d\n", x, y);
    unsigned short pixaddr = ScreenAddress + (y * ScreenPitch) + (x>>PPBshift);
    if (pixaddr < ScreenAddress || pixaddr > ScreenEnd) return;
    unsigned short xmodPPB = x%PixelsPerByte;
    unsigned char pixmask = pixelmasks[PPBshift][xmodPPB];
    unsigned char pixelbyte = MemRead(pixaddr) & (pixmask^0xff);
    pixelbyte |= Color<<(BitsPerPixel*(PixelsPerByte-xmodPPB-1));
    MemWrite(pixelbyte, pixaddr);
}

void DrawLine(unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2)
{
    // fprintf(stderr, "DrawLine %x %d %d %d\n", x1, y1, x2, y2);
	int dx, dy;
	int inc1, inc2;
	int x, y, d;
	int xEnd, yEnd;
	int xDir, yDir;
	
	dx = abs(x2 - x1);
	dy = abs(y2 - y1);

	if (dy <= dx) 
    {
		d = dy*2 - dx;
		inc1 = dy*2;
		inc2 = (dy-dx)*2;
		if (x1 > x2) { x = x2; y = y2; yDir = -1; xEnd = x1; } else { x = x1; y = y1; yDir = 1; xEnd = x2; }
		SetPixel(x, y);

		if (((y2-y1)*yDir) > 0) {
			while (x < xEnd) 
            {
				x++;
				if (d < 0) { d += inc1; } else { y++; d += inc2; }
				SetPixel(x, y);
			}
		} 
        else 
        {
			while (x < xEnd) 
            {
				x++;
				if (d < 0) { d += inc1; } else { y--; d += inc2; }
				SetPixel(x, y);
			}
		}		
	} 
    else 
    {
		d = dx*2 - dy;
		inc1 = dx*2;
		inc2 = (dx-dy)*2;
		if (y1 > y2) { y = y2; x = x2; yEnd = y1; xDir = -1; } 
        else { y = y1; x = x1; yEnd = y2; xDir = 1; }
		SetPixel(x, y);

		if (((x2-x1)*xDir) > 0) 
        {
			while (y < yEnd) 
            {
				y++;
				if (d < 0) { d += inc1; } else { x++; d += inc2; }
				SetPixel(x, y);
			}
		} 
        else 
        {
			while (y < yEnd) 
            {
				y++;
				if (d < 0) { d += inc1; } else { x--; d += inc2; }
				SetPixel(x, y);
			}
		}
	}
}