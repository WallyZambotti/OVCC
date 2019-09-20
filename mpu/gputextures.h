struct _Texture
{
    unsigned int  id;
    struct _Texture *nextTexture;
    ushort  w, h, pitch, bitmapsize;
    ushort  ppb, bpp, ppbshift, transparencyActive;
    uchar   tranparencyColor;
    uchar   *bitmap;
    ushort  smx, smy, smw, smh;
    uchar   *savemap;
};

typedef struct _Texture Texture;

struct _rect
{
    unsigned short h, w, y, x;
};

typedef struct _rect Rect;

void NewTexture(unsigned short, unsigned short, unsigned short, unsigned short);
void DestroyTexture(unsigned short);
void SetTextureTransparency(unsigned short, unsigned short, unsigned short);
void LoadTexture(unsigned short, unsigned short, unsigned short);
void DestroyTexture(unsigned short);
void RenderTexture(unsigned short, unsigned short, unsigned short, unsigned short, unsigned short);
void QRenderTexture(Screen*, Texture*, unsigned short, unsigned short, Rect*);
