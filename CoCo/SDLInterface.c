/*
Copyright 2015 by Joseph Forgione
This file is part of VCC (Virtual Color Computer).

    VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
*/

#include <agar/core.h>
#include <agar/gui.h>
#include <stdbool.h>
#include "defines.h"
#include "stdio.h"
#include "tcc1014graphicsSDL.h"
#include "SDLInterface.h"
#include "throttle.h"

static unsigned char InfoBand=1;
static unsigned char Resizeable=1;
static unsigned char ForceAspect=1;
static unsigned int Color=0;

//Function Prototypes for this module
void DisplayFlip2(SystemState2 *);

bool CreateAGARWindow(SystemState2 *CWState)
{
	char message[256]="";

	// Initialize AGAR

    if (AG_InitCore(NULL, 0) == -1 || AG_InitGraphics("glx") == -1)
    {
        fprintf(stderr, "Init failed: %s\n", AG_GetError());
        return FALSE;
    }

    CWState->agwin = AG_WindowNew(AG_WINDOW_MAIN);
    AG_WindowSetCaption(CWState->agwin, "OVCC 1.3.0");
    AG_WindowSetGeometryAligned(CWState->agwin, AG_WINDOW_ALIGNMENT_NONE, 644, 538);
    AG_WindowSetCloseAction(CWState->agwin, AG_WINDOW_DETACH);

	return TRUE;
}

/*--------------------------------------------------------------------------*/
// Checks if the memory associated with surfaces is lost and restores if necessary.
void CheckSurfacesSDL()
{
	// Not sure if this is relevant in SDL
}
/*--------------------------------------------------------------------------*/

/*
	DisplayFlipSDL is step 3 in the drawing process
	and is called after the texture pixels have been updated
	and are ready to bo rendered to screen.

	All it does is request AGAR redraw the fx widget.

	Because the fx widget has the widget-drawn event registered
	against it, this will cause step 4 the FXdrawn (vccgui.c) to be evoked.

	Because FXdrawn will be evoked by the AGAR
	event manager it will occur in the primary AGAR
	thread avoiding any multi thread complications.
*/

void DisplayFlipSDL(SystemState2 *DFState)	// Double buffering flip
{
	extern char redrawfx; // This is used by AG_RedrawOnChange() setup in vccgui.c
    //fprintf(stderr, "3(%2.3f)", timems());
	AG_PixmapUpdateSurface(DFState->fx, 0);
	AG_PixmapSetSurface(DFState->fx, 0);
	redrawfx = !redrawfx; // just needs to be changed
}

/*
	LockScreenSDL is step 1 in the drawing process
	and is called to lock the drawing texture prior
	to attempting to manipulate (draw) the texture
	pixels.

	It Posts a "lock-texture" event that will be processed
	by the AGAR event manager which will cause any function
	(step 2) registered to the event to be evoked.

	Currently step 2 the LockTexture function in vccgui.c is
	registered against this event.

	Because LockTexture will be evoked by the AGAR
	event manager it will occur in the primary AGAR
	thread avoiding any multi thread complications.
*/

unsigned char LockScreenSDL(SystemState2 *LSState)
{
	// fprintf(stderr, "1.");
    // fprintf(stderr, "1(%2.3f)", timems());
	// AG_PostEvent(LSState->fx, "lock-texture", "%p", LSState);

	return 0;
}

void UnlockScreenSDL(SystemState2 *USState)
{
	// if (USState->EmulationRunning == 0) 
	// {
	// 	//fprintf(stderr, "*");
	// 	return;
	// }
	DisplayFlipSDL(USState);
	return;
}

void ClsSDL(unsigned int ClsColor, SystemState2 *CLState)
{
	CLState->ResetPending=3; //Tell Main loop to hold Emu
	Color=ClsColor;
	return;
}

void DoClsSDL(SystemState2 *CLStatus)
{
	unsigned short x=0,y=0;

	LockScreenSDL(CLStatus);

	for (y=0;y<480;y++)
	{
		for (x=0;x<640;x++)
		{
			CLStatus->PTRsurface32[x + (y*CLStatus->SurfacePitch) ] = Color;
		}
	}

	UnlockScreenSDL(CLStatus);
	return;
}

unsigned char SetInfoBandSDL(unsigned char Tmp)
{
	if (Tmp!=QUERY)
		InfoBand=Tmp;
	return(InfoBand);
}

unsigned char SetResizeSDL(unsigned char Tmp)
{
	if (Tmp!=QUERY)
		Resizeable=Tmp;
	return(Resizeable);
}

unsigned char SetAspectSDL(unsigned char Tmp)
{
	if (Tmp!=QUERY)
		ForceAspect=Tmp;
	return(ForceAspect);
}


float StaticSDL(SystemState2 *STState)
{
	unsigned short x=0;
	static unsigned short y=0;

	unsigned char Temp=0;
	static unsigned short TextX=0,TextY=0;
	static unsigned char Counter,Counter1=32;
	static char Phase=1;
	static char Message[]=" Signal Missing! Press F9";
	static unsigned char GreyScales[4]={128,135,184,191};

	LockScreenSDL(STState);

	if (STState->Pixels == NULL) return(0);

	for (y=0;y<480;y++)
	{
		for (x=0;x<640;x++)
		{
			if(STState->Pixels == NULL) return (0.0);
			Temp=rand() & 255;
			STState->PTRsurface32[x + (y*STState->SurfacePitch)] = Temp | (Temp<<8) | (Temp<<16) | (255<<24);
		}
	}

	Counter++;
	Counter1+=Phase;

	if ((Counter1==60) | (Counter1==20))
		Phase=-Phase;

	Counter%=60; //about 1 seconds

	if (!Counter)
	{
		TextX=rand()%580;
		TextY=rand()%470;
	}

	UnlockScreenSDL(STState);
	return(CalculateFPS());
}