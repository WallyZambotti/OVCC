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
//#include <sys/timerfd.h>
#include <sys/time.h>
#include <time.h>
#include <sched.h>
#include "throttle.h"
#include "audio.h"
#include "defines.h"
#include "vcc.h"

static unsigned long /*long*/ StartTime,EndTime,OneFrame,CurrentTime,SleepRes,TargetTime,OneMs;
static unsigned long /*long*/ LagTime = 0, LagCnt = 0;
static unsigned long /*long*/ MasterClock,Now;
static unsigned char FrameSkip=0;
static float fMasterClock=0;
static int timerfd;
static float cfps=0;

void CalibrateThrottle(void)
{
    struct sched_param schedparm;

	MasterClock = SDL_GetPerformanceFrequency();
	OneFrame = MasterClock / (TARGETFRAMERATE);
	OneMs = MasterClock / 1000;
	fMasterClock=(float)MasterClock;
	LagTime = 0;
	LagCnt = 0;


	// timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
	// memset(&schedparm, 0, sizeof(schedparm));
	// schedparm.sched_priority = 1; // lowest rt priority
	// pthread_setschedparam(0, SCHED_FIFO, &schedparm);

	StartTime = TargetTime = SDL_GetPerformanceCounter();
	//printf("CalibrateThrottle : MC %lld fMC %f 1ms %lld 1Frame %lld ST %lld TT %lld\n", MasterClock, fMasterClock, OneMs, OneFrame, StartTime, TargetTime);
}


void StartRender(void)
{
	StartTime = TargetTime; //SDL_GetPerformanceCounter();
	//fprintf(stderr, "<");
	return;
}

void EndRender(unsigned char Skip)
{
	FrameSkip = Skip;
	//if (abs(LagTime) > OneFrame) LagTime = 0;

	//fprintf(stderr, "<%lld>", LagTime);
	TargetTime = ( StartTime + (OneFrame * FrameSkip));
	//fprintf(stderr, ">");
	return;
}

void FrameWait(void)
{
	//unsigned long long TwoMs = OneMs * 1;
	//unsigned long long Tt_minus_2ms = TargetTime - TwoMs;
	CurrentTime = SDL_GetPerformanceCounter();
	//int msDelays = (Tt_minus_2ms - CurrentTime) / OneMs;
	struct timespec duration, dummy;

	//delayed = timems();

	if (CurrentTime > TargetTime)
	{
		extern void CPUConfigSpeedDec(void);  // ran out of time so reduce the CPU frequency
		if (cfps < (TARGETFRAMERATE-0.25)) CPUConfigSpeedDec();
		LagTime += CurrentTime - TargetTime;
		LagCnt++;
		//fprintf(stderr, "<%lld>", LagTime);
		return;
	}

	if (CurrentTime == TargetTime)
	{
		//LagTime = 0;
		//fprintf(stderr, "-");
		return;
	}

	extern void CPUConfigSpeedInc(void); // had time left over so increase the CPU frequency
	CPUConfigSpeedInc();
	// fprintf(stderr, "^");

	// Use nanosleep to delay

	unsigned long /*long*/ tmpt = TargetTime - CurrentTime;
	unsigned long /*long*/ lagavg = LagCnt ? LagTime / LagCnt : 0;
	if (lagavg < tmpt)
	{
		tmpt -= lagavg;
		duration.tv_sec = 0;
		duration.tv_nsec = tmpt;
		nanosleep(&duration, &dummy);
	}


	// Use AG_Delay or SDL_Delay ro delay

	// if (msDelays > 1)
	// {
	// 	AG_Delay(msDelays);
	// }

	CurrentTime = SDL_GetPerformanceCounter();

	// Use Loop with AG_Delay or SDL_Delay to delay

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

	// Use busy CPU with high resolution counter to delay

	while (CurrentTime < TargetTime)	//Poll Until frame end.
	{
		CurrentTime = SDL_GetPerformanceCounter();
	}

	LagTime += CurrentTime - TargetTime;
	LagCnt++;
	//fprintf(stderr, "(%ld,%ld)", LagTime);

	return;
}

// float timems()
// {
// 	static struct timeval tval_before, tval_after, tval_result;
// 	static int firsttime = 1;
// 	float secs, fsecs;

// 	if (firsttime)
// 	{
// 		gettimeofday(&tval_before, NULL);
// 		firsttime = 0;
// 		return 0.0;
// 	}

// 	gettimeofday(&tval_after, NULL);
// 	timersub(&tval_after, &tval_before, &tval_result);
// 	memcpy(&tval_before, &tval_after, sizeof(tval_after));

// 	secs = tval_result.tv_sec;
// 	fsecs = (float)tval_result.tv_usec / 1000000.0;

// 	return secs + fsecs;
// }

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
	fps*=FrameSkip;
	//printf("%d %2.2f %f %f %f\n", FrameCount, fps, fNow, fLast, fNow-fLast) ;
	fLast=fNow;
	FrameCount=0;
	cfps = fps;
	return(fps);
}

float GetCurrentFPS()
{
	return cfps;
}

