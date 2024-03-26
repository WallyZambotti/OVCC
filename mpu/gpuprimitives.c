#include <stdio.h>
#include <stdlib.h>
#include "mpu.h"
#include "gpu.h"
#include "dma.h"
#include "gpuprimitives.h"
#include "linkedlists.h"

LinkedList ScreenList = { NULL, NULL, 0 };

static unsigned short currentID;

void NewScreen(unsigned short idref, unsigned short address, unsigned short width, unsigned short height, unsigned short bitsperpixel)
{
    // fprintf(stderr, "NewScreen %x %d %d %d\n", address, width, height, bitsperpixel);

    if (bitsperpixel == 0) { WriteCoCoInt(idref, 0xFFFF); return ; };

    Screen *NewScreen = malloc(sizeof(Screen));

    NewScreen->id = ++currentID;
    NewScreen->nextScreen = NULL;
    NewScreen->ScreenAddress = address;
    NewScreen->ScreenWidth  = width;
    NewScreen->ScreenHeight = height;
    NewScreen->BitsPerPixel = bitsperpixel;
    NewScreen->PixelsPerByte = (8 / NewScreen->BitsPerPixel);
    NewScreen->ScreenPitch = width / NewScreen->PixelsPerByte;
    NewScreen->ScreenEnd = NewScreen->ScreenAddress + (NewScreen->ScreenPitch * NewScreen->ScreenHeight);

    NewScreen->PPBshift = -1;
    for(unsigned short int PPB = NewScreen->PixelsPerByte ; PPB ; PPB=PPB>>1) { NewScreen->PPBshift++; }

    AppendListItem(&ScreenList, (LinkedListItem*)NewScreen);

    // Interegate the mmu and record the current process taskmmubank map

    unsigned char tr = (MemRead(0xFF91) & 0x01)<<3; // Task register
    unsigned short addr = 0xFFA0 + tr;

    for(short i = 0 ; i <  8 ; i++)
    {
        NewScreen->taskmmubank[i] = MemRead(addr++);
        //fprintf(stderr, "%02x ", NewScreen->taskmmubank[i]);
    }
    // write(2, "\n",1);
    // fprintf(stderr, "NewScreen %d %d %d %d\n", NewScreen->PixelsPerByte, NewScreen->ScreenPitch, NewScreen->ScreenEnd, NewScreen->PPBshift);
    // ReportQueue();

    WriteCoCoInt(idref, (unsigned short)NewScreen->id);
}

void DestroyScreen(unsigned short int id)
{
    // fprintf(stderr, "DestroyScreen %d\n", id);

    Screen *screen = (Screen*)RemovelistItem(&ScreenList, (unsigned int)id);

    if (screen == NULL) return;

    free(screen);
}

Screen *GetScreen(unsigned short int id)
{
    return (Screen*)FindListItem(&ScreenList, (unsigned int)id);
}

unsigned char GetScreenMMUmemPagefromAddress(Screen *screen, unsigned short int addr)
{
    if (screen == NULL) return 0;
    unsigned short bankidx = addr>>13;
    return screen->taskmmubank[bankidx];
}

void SetColor(unsigned short screenid, unsigned short color)
{
    Screen *screen = (Screen*)FindListItem(&ScreenList,  (unsigned int)screenid);

    if (screen == NULL) return;

    // fprintf(stderr, "SetColor %d\n", color);
    screen->Color = color;
}

void SetScreenColor(Screen *screen, unsigned short color)
{
    if (screen == NULL) { return; }

    // fprintf(stderr, "SetColor %d\n", color);
    screen->Color = color;
}

void SetPixel(unsigned short screenid, unsigned short x, unsigned short y)
{
    Screen *screen = (Screen*)FindListItem(&ScreenList,  (unsigned int)screenid);

    if (screen == NULL) return;

    SetScreenPixel(screen, x, y);
}


void SetScreenPixel(Screen *screen, unsigned short x, unsigned short y)
{
    if (screen == NULL) { return; }

    // fprintf(stderr, "SetPixel %d %d\n", x, y);
    unsigned short pixaddr = screen->ScreenAddress + (y * screen->ScreenPitch) + (x>>screen->PPBshift);
    if (pixaddr < screen->ScreenAddress || pixaddr > screen->ScreenEnd) 
    {
        // write(0, "?", 1);
        return;
    }
    unsigned short xmodPPB = x%screen->PixelsPerByte;
    unsigned char  pixmask = pixelmasks[screen->PPBshift][xmodPPB];
    unsigned char bankpage = GetScreenMMUmemPagefromAddress(screen, pixaddr);
    // unsigned char pixelbyte = MemRead(pixaddr) & (pixmask^0xff);
    unsigned char  pixelbyte = MmuRead(bankpage, pixaddr) & (pixmask^0xff);
    pixelbyte |= screen->Color<<(screen->BitsPerPixel*(screen->PixelsPerByte-xmodPPB-1));
    // MemWrite(pixelbyte, pixaddr);
    MmuWrite(pixelbyte, bankpage, pixaddr);
}

void DrawLine(unsigned short screenid, unsigned short x1, unsigned short y1, unsigned short x2, unsigned short y2)
{
    Screen *screen = (Screen*)FindListItem(&ScreenList,  (unsigned int)screenid);

    if (screen == NULL) return;

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
		if (x1 > x2) { x = x2; y = y2; yDir = -1; xEnd = x1; } 
        else { x = x1; y = y1; yDir = 1; xEnd = x2; }
		SetScreenPixel(screen, x, y);

		if (((y2-y1)*yDir) > 0) {
			while (x < xEnd) 
            {
				x++;
				if (d < 0) { d += inc1; } else { y++; d += inc2; }
				SetScreenPixel(screen, x, y);
			}
		} 
        else 
        {
			while (x < xEnd) 
            {
				x++;
				if (d < 0) { d += inc1; } else { y--; d += inc2; }
				SetScreenPixel(screen, x, y);
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
		SetScreenPixel(screen, x, y);

		if (((x2-x1)*xDir) > 0) 
        {
			while (y < yEnd) 
            {
				y++;
				if (d < 0) { d += inc1; } else { x++; d += inc2; }
				SetScreenPixel(screen, x, y);
			}
		} 
        else 
        {
			while (y < yEnd) 
            {
				y++;
				if (d < 0) { d += inc1; } else { x--; d += inc2; }
				SetScreenPixel(screen, x, y);
			}
		}
	}
}