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
void SetPixel(unsigned short, unsigned short, unsigned short);
void DrawLine(unsigned short, unsigned short, unsigned short, unsigned short, unsigned short);
void NewTexture(unsigned short, unsigned short, unsigned short, unsigned short);
