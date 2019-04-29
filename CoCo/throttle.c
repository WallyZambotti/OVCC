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
#include <sys/time.h>
#include <time.h>
#include "throttle.h"
#include "audio.h"
#include "defines.h"
#include "vcc.h"

static unsigned long long StartTime,EndTime,OneFrame,CurrentTime,SleepRes,TargetTime,OneMs;
static long long LagTime;
static unsigned long long MasterClock,Now;
static unsigned char FrameSkip=0;
static float fMasterClock=0;

void CalibrateThrottle(void)
{
	MasterClock = SDL_GetPerformanceFrequency();
	OneFrame = MasterClock / (TARGETFRAMERATE);
	OneMs = MasterClock / 1000;
	fMasterClock=(float)MasterClock;
	LagTime = 0;
	//printf("CalibrateThrottle : MC %ld fMC %f 1ms %ld 1Frame %ld\n", MasterClock, fMasterClock, OneMs, OneFrame);
}


void StartRender(void)
{
	StartTime = SDL_GetPerformanceCounter();
	return;
}

void EndRender(unsigned char Skip)
{
	FrameSkip = Skip;
	TargetTime = ( StartTime + (OneFrame * FrameSkip)) + LagTime;
	return;
}

void TestDelay(void)
{
	float time1, time2;
	int i;

	for (i = 0 ; i < 10 ; i++)
	{
		time1 = timems();
		AG_Delay(1);
		time2 = timems();
		fprintf(stderr, "(%2.3f)\n", time2);
	}
}

void FrameWait(void)
{
	unsigned long long TwoMs = OneMs * 1;
	unsigned long long Tt_minus_2ms = TargetTime - TwoMs;
	CurrentTime = SDL_GetPerformanceCounter();
	int msDelays = (Tt_minus_2ms - CurrentTime) / OneMs;
	long cnt;
	float delayed;
	struct timespec duration, dummy;

	//fprintf(stderr, "%d ", msDelays);
	//fprintf(stderr, "(%ld) ", (long long)TargetTime-CurrentTime);

	//delayed = timems();

	if (CurrentTime > TargetTime)
	{
		extern void CPUConfigSpeedDec(void);
		CPUConfigSpeedDec();
		return;
	}

	if (CurrentTime == TargetTime)
	{
		return;
	}
	
	duration.tv_sec = 0;
	duration.tv_nsec = TargetTime - CurrentTime;
	nanosleep(&duration, &dummy);

	{
		extern void CPUConfigSpeedInc(void);
		CPUConfigSpeedInc();
	}

	// if (msDelays > 1)
	// {
	// 	AG_Delay(msDelays);
	// }

	//delayed = timems();
	//fprintf(stderr, "%2.3f ", delayed);

	CurrentTime = SDL_GetPerformanceCounter();

	// while (CurrentTime < Tt_minus_2ms)	//If we have more that 2Ms till the end of the frame
	// {
	// 	AG_Delay(1);	//Give about 1Ms back to the system
	// 	CurrentTime = SDL_GetPerformanceCounter();	//And check again
	// }

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
	
	//fprintf(stderr, "%ld ", (long long)TargetTime-CurrentTime);
	//cnt=0;

	while (CurrentTime < TargetTime)	//Poll Until frame end.
	{
		CurrentTime = SDL_GetPerformanceCounter();
		//cnt++;
	}

	LagTime = (long long)TargetTime-CurrentTime;
	//fprintf(stderr, "%ld %d\n", LagTime, cnt);

	return;
}

float timems()
{
	static struct timeval tval_before, tval_after, tval_result;
	static int firsttime = 1;
	float secs, fsecs;

	if (firsttime)
	{
		gettimeofday(&tval_before, NULL);
		firsttime = 0;
		return 0.0;
	}

	gettimeofday(&tval_after, NULL);
	timersub(&tval_after, &tval_before, &tval_result);
	memcpy(&tval_before, &tval_after, sizeof(tval_after));

	secs = tval_result.tv_sec;
	fsecs = (float)tval_result.tv_usec / 1000000.0;

	return secs + fsecs;
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
	fps= FRAMEINTERVAL/fps;
	//printf("%d %2.2f %f %f %f\n", FrameCount, fps, fNow, fLast, timems());
	fLast=fNow;
	FrameCount=0;
	return(fps);
}

