/*	Public domain	*/

#ifndef _AGAR_WIDGET_FIXEDTEXTURE_H_
#define _AGAR_WIDGET_FIXEDTEXTURE_H_

#include <SDL2/SDL.h>
#include <agar/gui.h>

typedef struct ag_fixedtexture {
	struct ag_widget _inherit;
	Uint flags;
#define AG_FIXEDTEXTURE_HFILL		0x01	/* Expand to fill available width */
#define AG_FIXEDTEXTURE_VFILL		0x02	/* Expand to fill available height */
#define AG_FIXEDTEXTURE_NO_UPDATE	0x04	/* Don't call WINDOW_UPDATE() */
#define AG_FIXEDTEXTURE_EXPAND		(AG_FIXED_HFILL|AG_FIXED_VFILL)
	int wPre, hPre;			/* User geometry */
	int wText, hText;       /* Texture geometry */
	SDL_Texture	*Texture;
	void *Pixels;
	SDL_Renderer *Renderer;
} AG_FixedTexture;

extern AG_WidgetClass agFixedTextureClass;

AG_FixedTexture *AG_FixedTextureNew(void *, int, int, Uint);
void	  AG_FixedTextureSizeHint(AG_FixedTexture *, int, int);
#define	  AG_FixedTexturePrescale AG_FixedTextureSizeHint

SDL_Texture	  *AG_FixedTextureGet(AG_FixedTexture *);
void	  AG_FixedTextureLock(AG_FixedTexture *);
void	  AG_FixedTextureUnlock(AG_FixedTexture *);

#endif /* _AGAR_WIDGET_FIXEDTEXTURE_H_ */
