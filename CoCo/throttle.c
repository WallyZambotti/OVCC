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
#include <SDL2/SDL.h>
#include "throttle.h"
#include "audio.h"
#include "defines.h"
#include "vcc.h"

static Uint64 StartTime,EndTime,OneFrame,CurrentTime,SleepRes,TargetTime,OneMs;
static Uint64 MasterClock,Now;
static unsigned char FrameSkip=0;
static float fMasterClock=0;

void CalibrateThrottle(void)
{
	MasterClock = SDL_GetPerformanceFrequency();
	OneFrame = MasterClock / (TARGETFRAMERATE);
	OneMs = MasterClock / 1000;
	fMasterClock=(float)MasterClock;
}


void StartRender(void)
{
	StartTime = SDL_GetPerformanceCounter();
	return;
}

void EndRender(unsigned char Skip)
{
	FrameSkip = Skip;
	TargetTime = ( StartTime + (OneFrame * FrameSkip));
	return;
}

void FrameWait(void)
{
	CurrentTime = SDL_GetPerformanceCounter();

	while ( ((long)TargetTime - (long)CurrentTime) > (long)(OneMs * 2))	//If we have more that 2Ms till the end of the frame
	{
		AG_Delay(1);	//Give about 1Ms back to the system
		CurrentTime = SDL_GetPerformanceCounter();	//And check again
	}

	if (GetSoundStatusSDL())	//Lean on the sound card a bit for timing
	{
		// PurgeAuxBufferSDL();
		// if (FrameSkip==1)
		// {
		// 	if (GetFreeBlockCountSDL()>AUDIOBUFFERS/2)		//Dont let the buffer get lest that half full
		// 		return;
		// 	while (GetFreeBlockCount() < 1);	// Dont let it fill up either
		// }

	}
	while ( CurrentTime < TargetTime )	//Poll Untill frame end.
		CurrentTime = SDL_GetPerformanceCounter();

	return;
}

float CalculateFPS(void) //Done at end of render;
{

	static unsigned short FrameCount=0;
	static float fps=0,fNow=0,fLast=0;

	if (++FrameCount!=FRAMEINTERVAL)
		return(fps);

	Now = SDL_GetPerformanceCounter();
	fNow=(float)Now;
	fps=(fNow-fLast)/fMasterClock;
	fLast=fNow;
	FrameCount=0;
	fps= FRAMEINTERVAL/fps;
	return(fps);
}
		