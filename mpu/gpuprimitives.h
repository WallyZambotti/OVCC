struct _screen
{
    unsigned int id;
    struct _screen *nextScreen;
    unsigned short ScreenAddress;
    unsigned short ScreenWidth;
    unsigned short ScreenPitch;
    unsigned short ScreenHeight;
    unsigned short ScreenEnd;
    unsigned short BitsPerPixel;
    unsigned short PixelsPerByte;
    short PPBshift;
    unsigned short Color;
    unsigned char taskmmubank[8];
};

typedef struct _screen Screen;

Screen *GetScreen(unsigned short int);

void NewScreen(unsigned short, unsigned short, unsigned short, unsigned short, unsigned short);
void DestroyScreen(unsigned short);
void SetColor(unsigned short, unsigned short);
void SetScreenColor(Screen*, unsigned short);
void SetPixel(unsigned short, unsigned short, unsigned short);
void SetScreenPixel(Screen*, unsigned short, unsigned short);
void DrawLine(unsigned short, unsigned short, unsigned short, unsigned short, unsigned short);

static unsigned char pixelmasks[4][8] = 
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