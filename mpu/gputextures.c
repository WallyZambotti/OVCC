#include <stdio.h>
#include <stdlib.h>
#include "mpu.h"
#include "gpu.h"
#include "gpuprimitives.h"
#include "gputextures.h"
#include "linkedlists.h"
#include "dma.h"

unsigned char GetScreenMMUmemPagefromAddress(Screen *screen, unsigned short int addr);

LinkedList TextureList = { NULL, NULL, 0 };

static ushort currentID;

void NewTexture(ushort idref, ushort w, ushort h, ushort bpp)
{
    // fprintf(stderr, "NewTexture %x %d %d %d\n", idref, w, h, bpp);

    if (bpp == 0) { WriteCoCoInt(idref, 0xFFFF); return ; };

    Texture *NewTexture = malloc(sizeof(Texture));

    NewTexture->id = ++currentID;
    NewTexture->w = w;
    NewTexture->h = h;
    NewTexture->bpp = bpp;
    NewTexture->ppb = 8 / bpp;
    NewTexture->pitch = w / NewTexture->ppb;
    NewTexture->bitmapsize = NewTexture->pitch * h;
    NewTexture->tranparencyColor = 0;
    NewTexture->transparencyActive = 0;
    NewTexture->bitmap = malloc(NewTexture->bitmapsize);
    NewTexture->nextTexture = NULL;
    NewTexture->savemap = NULL;

    NewTexture->ppbshift = -1;
    for(unsigned short int PPB = NewTexture->ppb ; PPB ; PPB=PPB>>1) { NewTexture->ppbshift++; }

    AppendListItem(&TextureList, (LinkedListItem*)NewTexture);

    // return the id to the caller

    WriteCoCoInt(idref, NewTexture->id);
}

void DestroyTexture(ushort id)
{
    // fprintf(stderr, "DestroyTexture %d\n", id);

    Texture *texture = (Texture*)RemovelistItem(&TextureList, (unsigned int)id);

    if (texture == NULL) return;

    free(texture->bitmap);
    free(texture);
}

void LoadTexture(ushort screenid, ushort textureid, ushort memaddr)
{
    // fprintf(stderr, "LoadTexture %d %d %x\n", screenid, textureid, memaddr);

    Screen *screen = GetScreen(screenid);

    if (screen == NULL) return;

    Texture *texture = (Texture*)FindListItem(&TextureList, (ushort)textureid);

    if (texture == NULL) return;

    for (ushort i = 0 ; i < texture->bitmapsize ; i++)
    {
        ushort bankpage = GetScreenMMUmemPagefromAddress(screen, memaddr);
        texture->bitmap[i] = MmuRead(bankpage, memaddr++);
    }
}

void SetTextureTransparency(ushort id, ushort transparencyonoff, ushort color)
{
    // fprintf(stderr, "SetTextureTransparency %d %d %d\n", id, transparencyonoff, color);

    Texture *texture = (Texture*)FindListItem(&TextureList,  (unsigned int)id);

    if (texture == NULL) return;

    texture->transparencyActive = transparencyonoff;
    texture->tranparencyColor = color;
}

void RenderTexture(ushort screenid, ushort textureid, ushort sx, ushort sy, ushort rectref)
{
    Screen *screen = GetScreen(screenid);

    if (screen == NULL) return;

    Texture *texture = (Texture*)FindListItem(&TextureList, (ushort)textureid);

    if (texture == NULL) return;

    Rect *rect = NULL;

    if (rectref != NULL)
    {
        rect = malloc(sizeof(Rect));
        Data bytes = ReadCoCo8bytes(rectref);
        *rect = *((Rect*)&bytes);
    }

    // fprintf(stderr, "RenderTexture %d %d %d %d %d %d %d %d\n", screenid, textureid, sx, sy, rect->x, rect->y, rect->w, rect->h);

    QueueGPUrequest(CMD_RenderTexture, screen, texture, sx, sy, rect);
}

void QRenderTexture(Screen *screen, Texture *texture, ushort sx, ushort sy, Rect *rect)
{
    ushort tmpsx = sx;
    ushort rx, ry, rw, rh;

    if (rect == NULL)
    {
        rx = ry = 0; rw = texture->w; rh = texture->h;
    }
    else 
    {
        rx = rect->x; ry = rect->y; rw = rect->w; rh = rect->h;
    }
    
    // fprintf(stderr, "QRenderTexture %d %d %d %d %d %d %d %d %d %d\n", screen->id, texture->id, sx, sy, screen->ScreenWidth, screen->ScreenHeight, rx, ry, rw, rh);
    ushort tw = rx + rw;
    ushort th = ry + rh;

    if(rect) { free(rect); }

    if (tw > texture->w || th > texture->h)
    {
        // fprintf(stderr, "QRenderTexture tw or th to large %d %d %d %d\n", tw, th, texture->w, texture->h);
        return; 
    }

    uchar *ta = texture->bitmap + (ry * texture->pitch) + (rx>>texture->ppbshift);
    // uchar *sa = screen->ScreenAddress + (sy * screen->ScreenPitch) + (sx>>screen->PPBshift);
    uchar *tmpta = ta;

    // fprintf(stderr, "ppbshift %d, ppb %d, bpp %d\n", texture->ppbshift, texture->ppb, texture->bpp);

    for (ushort y = ry ; y < th ; y++)
    {
        if (sy >= screen->ScreenHeight) break;

        for  (ushort x = rx ; x < tw ; x++)
        {
            if (sx >= screen->ScreenWidth) break;

            ushort xmodPPB = x%texture->ppb;
            uchar  pixmask = pixelmasks[texture->ppbshift][xmodPPB];
            uchar  pixelbyte = *ta & pixmask;
            uchar  pixel = pixelbyte>>texture->bpp*(texture->ppb-xmodPPB-1);

            if (texture->transparencyActive == 0 || pixel != texture->tranparencyColor)
            {
                SetScreenColor(screen, pixel);
                SetScreenPixel(screen, sx, sy);
           }

            if (((x+1)%texture->ppb) == 0) { ta++; }
            sx++;
        }

        tmpta += texture->pitch;
        ta = tmpta;
        sy++;
        sx = tmpsx;
    }
}

void RenderTextureSafe(ushort screenid, ushort textureid, ushort sx, ushort sy, ushort rectref)
{
    Screen *screen = GetScreen(screenid);

    if (screen == NULL) return;

    Texture *texture = (Texture*)FindListItem(&TextureList, (ushort)textureid);

    if (texture == NULL) return;

    Rect *rect = NULL;

    if (rectref != NULL)
    {
        rect = malloc(sizeof(Rect));
        Data bytes = ReadCoCo8bytes(rectref);
        *rect = *((Rect*)&bytes);
    }

    // fprintf(stderr, "RenderTextureSafe %d %d %d %d %d %d %d %d\n", screenid, textureid, sx, sy, rect->x, rect->y, rect->w, rect->h);

    QueueGPUrequest(CMD_RenderTexture, screen, texture, sx, sy, rect);
}

// void QRenderTextureSafe(Screen *screen, Texture *texture, ushort sx, ushort sy, Rect *rect)
// {
//     ushort tmpsx = sx;
//     ushort rx, ry, rw, rh, rsx, rsy, tsx = 0, tsy = 0, spc = 0;
//     ushort restore = texture->savemap != NULL;

//     if (rect == NULL)
//     {
//         rx = ry = 0; rw = texture->w; rh = texture->h;
//     }
//     else 
//     {
//         rx = rect->x; ry = rect->y; rw = rect->w; rh = rect->h;
//     }
    
//     // fprintf(stderr, "QRenderTextureSafe %d %d %d %d %d %d %d %d %d %d\n", screen->id, texture->id, sx, sy, screen->ScreenWidth, screen->ScreenHeight, rx, ry, rw, rh);
//     ushort tw = rx + rw;
//     ushort th = ry + rh;

//     if(rect) { free(rect); }

//     if (tw > texture->w || th > texture->h)
//     {
//         // fprintf(stderr, "QRenderTextureSafe tw or th to large %d %d %d %d\n", tw, th, texture->w, texture->h);
//         return; 
//     }

//     if (!restore)
//     {
//         texture->smw = rw; texture->smh = rh;
//         texture->savemap = malloc((texture->smx * texture->smh) >> texture->ppbshift);
//     }

//     rsx = texture->smx; rsy = texture->smy;
//     texture->smx = sx; texture->smy = sy;

//     uchar *ta = texture->bitmap + (ry * texture->pitch) + (rx>>texture->ppbshift);
//     // uchar *sa = screen->ScreenAddress + (sy * screen->ScreenPitch) + (sx>>screen->PPBshift);
//     uchar *tmpta = ta;
//     uchar *sta = texture->savemap;

//     // fprintf(stderr, "ppbshift %d, ppb %d, bpp %d\n", texture->ppbshift, texture->ppb, texture->bpp);

//     for (ushort y = ry ; y < th ; y++)
//     {
//         if (sy >= screen->ScreenHeight) break;

//         for  (ushort x = rx ; x < tw ; x++)
//         {
//             if (sx >= screen->ScreenWidth) break;

//             ushort xmodPPB = x%texture->ppb;
//             uchar  pixmask = pixelmasks[texture->ppbshift][xmodPPB];
//             uchar  pixelbyte = *ta & pixmask;
//             uchar  pixel = pixelbyte>>texture->bpp*(texture->ppb-xmodPPB-1);

//             if (texture->transparencyActive == 0 || pixel != texture->tranparencyColor)
//             {
//                 if (restore)
//                 {
//                     if (rsx < texture->smx || rsy < texture->smy || rsx > texture->smx + texture->smw || rsy > texture->smy + texture->smh)
//                     {
//                         xmodPPB = tsx%texture->ppb;
//                         pixmask = pixelmasks[texture->ppbshift][xmodPPB];
//                         pixelbyte = *sta & pixmask;
//                         pixel = pixelbyte>>texture->bpp*(texture->ppb-xmodPPB-1);
//                     }
//                 }

//                 SetScreenColor(screen, pixel);
//                 uchar screenpixel = SetScreenPixel(screen, sx, sy);

//                 // Save screen pixel

//                 // TO DO : do the same boundary check if out of boundary save from screen otherwise save from previous offshifted save

//                 pixelbyte = *sta & (pixmask^0xff);
//                 pixel = pixelbyte>>texture->bpp*(texture->ppb-xmodPPB-1);
//                 *sta = pixelbyte | screenpixel<<(texture->bpp*(texture->ppb-xmodPPB-1));
//                 if (((++spc)%texture->ppb) == 0) { sta++; }
//             }

//             if (((x+1)%texture->ppb) == 0) { ta++; }
//             sx++;
//         }

//         tmpta += texture->pitch;
//         ta = tmpta;
//         sy++;
//         sx = tmpsx;
//     }
// }