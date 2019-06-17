#include "mpu.h"
#include "gpuprimitives.h"
#include "linkedlists.h"

struct _Texture
{
    unsigned int  id;
    struct _Texture *nextTexture;
    unsigned short  w, h, pitch, bitmapsize;
    unsigned short  ppb, bpp, transparencyActive;
    unsigned char   tranparencyColor, *bitmap;
};

typedef struct _Texture Texture;

LinkedList TextureList = { NULL, NULL, 0 };

static unsigned short currentID;

void NewTexture(unsigned short idref, unsigned short w, unsigned short h, unsigned short bpp)
{
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

    AppendListItem(&TextureList, (LinkedListItem*)NewTexture);

    // return the id to the caller

    WriteCoCoInt(idref, NewTexture->id);
}

void SetTextureTransparency(unsigned short id, unsigned short transparency, unsigned short color)
{
    Texture *texture = (Texture*)FindListItem(&TextureList,  (unsigned int)id);

    if (texture == NULL) return;

    texture->transparencyActive = transparency;
    texture->tranparencyColor = color;
}

void DestroyTexture(unsigned short int id)
{
    Texture *texture = (Texture*)RemovelistItem(&TextureList, (unsigned int)id);

    if (texture == NULL) return;

    free(texture->bitmap);
    free(texture);
}

void LoadTexture(unsigned short screenid, unsigned short id, unsigned short memaddr)
{
    Screen *screen = GetScreen(screenid);

    if (screen == NULL) return;

    Texture *texture = (Texture*)FindListItem(&TextureList, (unsigned short)id);

    if (texture == NULL) return;

    for (unsigned short int i = 0 ; i < texture->bitmapsize ; i++)
    {
        unsigned short bankpage = GetScreenMMUmemPagefromAddress(screen, memaddr);
        texture->bitmap[i] = MmuRead(bankpage, memaddr++);
    }
}