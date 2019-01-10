#include <SDL2/SDL.h>
#include <agar/core.h>
#include <agar/gui.h>
#include <agar/gui/begin.h>
#include "fixedtexture.h"
#include "sdl2driver.h"

AG_FixedTexture *
AG_FixedTextureNew(void *parent, int w, int h, Uint flags)
{
	AG_FixedTexture *fx;

	fx = malloc(sizeof(AG_FixedTexture));
	AG_ObjectInit(fx, &agFixedTextureClass);
	fx->flags |= flags;
	fx->wText = w;
	fx->hText = h;
	fx->Pixels = NULL;

	if (flags & AG_FIXEDTEXTURE_HFILL) { AG_ExpandHoriz(fx); }
	if (flags & AG_FIXEDTEXTURE_VFILL) { AG_ExpandVert(fx); }

	AG_ObjectAttach(parent, fx);
	return (fx);
}

static void
Init(void *obj)
{
	AG_FixedTexture *fx = obj;
	AG_DriverSDL2Ghost *sdl = AGWIDGET(fx)->drv;

	fx->flags = 0;
	fx->wPre = 0;
	fx->hPre = 0;

	fx->Renderer = sdl->r;

	if (fx->Renderer != NULL)
	{
		fx->Texture = SDL_CreateTexture(fx->Renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, fx->wText, fx->hText);
	}

	if (fx->Texture == NULL)
	{
		fprintf(stderr, "Cannot create texture. Redender = %lx\n", (unsigned long)fx->Renderer);
	}
}

void
AG_FixedTextureSizeHint(AG_FixedTexture *fx, int w, int h)
{
	AG_ObjectLock(fx);
	fx->wPre = w;
	fx->hPre = h;
	AG_ObjectUnlock(fx);
}

static void
SizeRequest(void *obj, AG_SizeReq *r)
{
	AG_FixedTexture *fx = obj;

	r->w = fx->wPre;
	r->h = fx->hPre;
}

static int
SizeAllocate(void *obj, const AG_SizeAlloc *a)
{
	AG_FixedTexture *fx = obj;
	return (0);
}

static void
Draw(void *obj)
{
	AG_FixedTexture *fx = obj;
	AG_Widget *chld;

	if (fx->Renderer != NULL && fx->Texture != NULL) SDL_RenderCopy(fx->Renderer, fx->Texture, NULL, (SDL_Rect*)&AGWIDGET(fx)->x);
}

static __inline__ void
UpdateWindow(AG_FixedTexture *fx)
{
	if (!(fx->flags & AG_FIXEDTEXTURE_NO_UPDATE))
		AG_WidgetUpdate(fx);
}

SDL_Texture *
AG_FixedTextureGet(AG_FixedTexture *fx)
{
	return fx->Texture;
}

void
AG_FixedTextureLock(AG_FixedTexture *fx)
{
	AG_SizeReq r;
	AG_SizeAlloc a;
	int pitch;

	if (fx->Texture != NULL && fx->Pixels == NULL) SDL_LockTexture(fx->Texture, NULL, &fx->Pixels, &pitch);
}

void
AG_FixedTextureUnLock(AG_FixedTexture *fx)
{
	AG_SizeReq r;
	AG_SizeAlloc a;

	if (fx->Pixels != NULL) SDL_UnlockTexture(fx->Texture);
	fx->Pixels = NULL;
}

AG_WidgetClass agFixedTextureClass = {
	{
		"AG_Widget:AG_FixedTexture", 
		sizeof(AG_FixedTexture),
		{ 0,0 },
		Init,
		NULL,		/* free */
		NULL,		/* destroy */
		NULL,		/* load */
		NULL,		/* save */
		NULL		/* edit */
	},
	Draw,
	SizeRequest,
	SizeAllocate
};
