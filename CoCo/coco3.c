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

#include "stdio.h"
#include <math.h>
#include "defines.h"
#include "tcc1014graphicsAGAR.h"
#include "tcc1014registers.h"
#include "mc6821.h"
#include "hd6309.h"
#include "mc6809.h"
#include "pakinterface.h"
#include "audio.h"
#include "coco3.h"
#include "throttle.h"
#include "vcc.h"
#include "cassette.h"
#include "logger.h"
#include "AGARInterface.h"

//****************************************
	static double SoundInterupt=0;
	static double PicosToSoundSample=0;//SoundInterupt;
	static double CyclesPerSecord=(COLORBURST/4)*(TARGETFRAMERATE/FRAMESPERSECORD);
	static double LinesPerSecond= TARGETFRAMERATE * LINESPERSCREEN;
	static double PicosPerLine = PICOSECOND / (TARGETFRAMERATE * LINESPERSCREEN);
	static double CyclesPerLine = ((COLORBURST/4)*(TARGETFRAMERATE/FRAMESPERSECORD)) / (TARGETFRAMERATE * LINESPERSCREEN);
	static double CycleDrift=0;
	static double CyclesThisLine=0;
	static unsigned int StateSwitch=0;
	unsigned short SoundRate=0;
//*****************************************************

#define AUDIO_BUF_LEN 16384
#define CAS_BUF_LEN 8192

static unsigned char HorzInteruptEnabled=0,VertInteruptEnabled=0;
static unsigned char TopBoarder=0,BottomBoarder=0;
static unsigned char LinesperScreen;
static unsigned char TimerInteruptEnabled=0;
static int MasterTimer=0; 
static unsigned short TimerClockRate=0;
static int TimerCycleCount=0;
static double MasterTickCounter=0,UnxlatedTickCounter=0,OldMaster=0;
static double PicosThisLine=0;
static unsigned char BlinkPhase=1;
static unsigned int AudioBuffer[AUDIO_BUF_LEN];
static unsigned char CassBuffer[CAS_BUF_LEN];
static unsigned short AudioIndex=0;
double PicosToInterupt=0;
static int IntEnable=0;
static int SndEnable=1;
static int OverClock=1;
static unsigned char SoundOutputMode=0;	//Default to Speaker 1= Cassette
void AudioOut(void);
void CassOut(void);
void CassIn(void);
void (*AudioEvent)(void)=AudioOut;
void SetMasterTickCounter(void);
int CPUCycle(void);

static unsigned long DC;
static long LC;

#ifndef ISOCPU
float RenderFrame (SystemState2 *RFState2, unsigned long DCnt)
{
	static unsigned short FrameCounter = 0;
	//WriteLog("RF ", TOCONS);
	//fprintf(stderr, ".");
	DC = DCnt;
	LC = 0;

//********************************Start of frame Render*****************************************************
	SetBlinkStateAGAR(BlinkPhase);
	irq_fs(0);				//FS low to High transition start of display Boink needs this
	for (RFState2->LineCounter=0;RFState2->LineCounter<13;RFState2->LineCounter++)		//Vertical Blanking 13 H lines 
		CPUCycle();

	for (RFState2->LineCounter=0;RFState2->LineCounter<4;RFState2->LineCounter++)		//4 non-Rendered top Boarder lines
		CPUCycle();

	for (RFState2->LineCounter=0;RFState2->LineCounter<(TopBoarder-4);RFState2->LineCounter++) 		
	{
		if (!(FrameCounter % RFState2->FrameSkip)) 
		{
			DrawTopBoarderAGAR(RFState2);
		}
		CPUCycle();
	}

	for (RFState2->LineCounter=0;RFState2->LineCounter<LinesperScreen;RFState2->LineCounter++)		//Active Display area		
	{
		CPUCycle();
		if (!(FrameCounter % RFState2->FrameSkip))
		{
			UpdateScreen(RFState2);
		}
	} 
	irq_fs(1);  //End of active display FS goes High to Low
	if (VertInteruptEnabled)
		GimeAssertVertInterupt();	
	for (RFState2->LineCounter=0;RFState2->LineCounter < (BottomBoarder) ;RFState2->LineCounter++)	// Bottom boarder 
	{
		CPUCycle();
		if (!(FrameCounter % RFState2->FrameSkip))
		{
			DrawBottomBoarderAGAR(RFState2);
		}
	}

	if (!(FrameCounter % RFState2->FrameSkip))
	{
		UpdateAGAR(RFState2);
		SetBoarderChangeAGAR(0);
	}

	for (RFState2->LineCounter=0;RFState2->LineCounter<6;RFState2->LineCounter++)		//Vertical Retrace 6 H lines
		CPUCycle();

	switch (SoundOutputMode)
	{
	case 0:
		FlushAudioBufferSDL(AudioBuffer,AudioIndex<<2);
		break;
	case 1:
		FlushCassetteBuffer(CassBuffer,AudioIndex);
		break;
	case 2:
		LoadCassetteBuffer(CassBuffer);

		break;
	}
	AudioIndex=0;

	return(CalculateFPS());
}
#else
static volatile int waitsync=1;
static long   lPicosPerLine = PICOSECOND / (TARGETFRAMERATE * LINESPERSCREEN);

float RenderFrame (SystemState2 *RFState2, unsigned long DCnt)
{
	long long frametime = 16666667;
	static unsigned short FrameCounter = 0;
	unsigned long long StartTime, EndTime, TargetTime;
    static volatile long finetune=0;
	//WriteLog("RF ", TOCONS);
	//fprintf(stderr, ".");
	DC = DCnt;
	LC = 0;

//********************************Start of frame Render*****************************************************
	SetBlinkStateAGAR(BlinkPhase);
	irq_fs(0);				//FS low to High transition start of display Boink needs this
	waitsync = 0; // release the cpu

	FrameCounter = (FrameCounter+1) % RFState2->FrameSkip;

    StartTime=SDL_GetPerformanceCounter(); // used to calc frame time

	for (RFState2->LineCounter=0;RFState2->LineCounter<13;RFState2->LineCounter++)		//Vertical Blanking 13 H lines 
	{
		TargetTime = SDL_GetPerformanceCounter() + lPicosPerLine - finetune;
		EndTime = SDL_GetPerformanceCounter();
		while(EndTime < TargetTime) { EndTime = SDL_GetPerformanceCounter(); }
	}

	for (RFState2->LineCounter=0;RFState2->LineCounter<4;RFState2->LineCounter++)		//4 non-Rendered top Boarder lines
	{
		TargetTime = SDL_GetPerformanceCounter() + lPicosPerLine - finetune;
		EndTime = SDL_GetPerformanceCounter();
		while(EndTime < TargetTime) { EndTime = SDL_GetPerformanceCounter(); }
	}

	for (RFState2->LineCounter=0;RFState2->LineCounter<(TopBoarder-4);RFState2->LineCounter++) 		// Top boarder
	{
		TargetTime = SDL_GetPerformanceCounter() + lPicosPerLine - finetune;
		if (!FrameCounter) { DrawTopBoarderAGAR(RFState2); }
		EndTime = SDL_GetPerformanceCounter();
		while(EndTime < TargetTime) { EndTime = SDL_GetPerformanceCounter(); }
	}

	for (RFState2->LineCounter=0;RFState2->LineCounter<LinesperScreen;RFState2->LineCounter++)		//Active Display area		
	{
		TargetTime = SDL_GetPerformanceCounter() + lPicosPerLine - finetune;
		if (!FrameCounter) { UpdateScreen(RFState2); }
		EndTime = SDL_GetPerformanceCounter();
		while(EndTime < TargetTime) { EndTime = SDL_GetPerformanceCounter(); }
	} 

	irq_fs(1);  //End of active display FS goes High to Low
	if (VertInteruptEnabled)
	{
		GimeAssertVertInterupt();	
	}

	for (RFState2->LineCounter=0;RFState2->LineCounter < (BottomBoarder) ;RFState2->LineCounter++)	// Bottom boarder 
	{
		TargetTime = SDL_GetPerformanceCounter() + lPicosPerLine - finetune;
		if (!FrameCounter) { DrawBottomBoarderAGAR(RFState2); }
		EndTime = SDL_GetPerformanceCounter();
		while(EndTime < TargetTime) { EndTime = SDL_GetPerformanceCounter(); }
	}

	if (!FrameCounter)
	{
		UpdateAGAR(RFState2);
		SetBoarderChangeAGAR(0);
	}

	for (RFState2->LineCounter=0;RFState2->LineCounter<6;RFState2->LineCounter++)		//Vertical Retrace 6 H lines
	{
		TargetTime = SDL_GetPerformanceCounter() + lPicosPerLine - finetune;
		EndTime = SDL_GetPerformanceCounter();
		while(EndTime < TargetTime) { EndTime = SDL_GetPerformanceCounter(); }
	}

	// frametime = EndTime - StartTime;
	// if (frametime < 16666667) finetune--; else finetune++;
	//fprintf(stderr, "<%lld:%d>", frametime, finetune);

	switch (SoundOutputMode)
	{
	case 0:
		FlushAudioBufferSDL(AudioBuffer,AudioIndex<<2);
		break;
	case 1:
		FlushCassetteBuffer(CassBuffer,AudioIndex);
		break;
	case 2:
		LoadCassetteBuffer(CassBuffer);

		break;
	}
	AudioIndex=0;

	float fps = CalculateFPS();

	if (fps > 0.1)
	{
		finetune+=(60.0 - fps) * 10.0;
	}

	//fprintf(stderr, "fps %2.1f - ft %ld - i %d\n", fps, finetune, fineinc);

	return(fps);
}

void *CPUloop (void *p)
{
	SystemState2 *RFState2 = p;
	long long cputime = 16666667;
	unsigned long long StartTime, EndTime, TargetTime;
	long timeavg, finetune=0;
	short lc;

	while (CPUExec == NULL) { AG_Delay(1); } // The Render loop resets the system and the CPU must wait for reset 1st time;

	while(1)
	{
		while(waitsync) {} waitsync=1; // sync with RenderFrame()
		timeavg = 0;
		StartTime = SDL_GetPerformanceCounter();
		// total iterations = 13 blanking lines + 4 non boarder lines + TopBoarder-4 lines + Active display lines + Bottom boarder lines + 6 vertical retrace lines
		// or TopBoarder + LinesPerScreen + Bottom Boarder + 19
		for(lc=0 ; lc < TopBoarder+LinesperScreen+BottomBoarder+19 ; lc++)
		{
			TargetTime = SDL_GetPerformanceCounter() + lPicosPerLine - finetune;
			CPUCycle();
			EndTime = SDL_GetPerformanceCounter();
			timeavg += TargetTime - EndTime;
			while(EndTime < TargetTime) { EndTime = SDL_GetPerformanceCounter(); }
		}
		timeavg /= lc;
		if (timeavg < 0) CPUConfigSpeedDec(); else if (timeavg > 0) CPUConfigSpeedInc();
		cputime = EndTime - StartTime;
		if (cputime < 16666667) finetune--; else finetune++;
		//fprintf(stderr, "<%lld:%d>", cputime, finetune);
	}
	return(RFState2);
}
#endif

void SetClockSpeed(unsigned short Cycles)
{
	OverClock=Cycles;
	return;
}

void SetHorzInteruptState(unsigned char State)
{
	HorzInteruptEnabled= !!State;
	return;
}

void SetVertInteruptState(unsigned char State)
{
	VertInteruptEnabled= !!State;
	return;
}

void SetLinesperScreen (unsigned char Lines)
{
	Lines = (Lines & 3);
	LinesperScreen=Lpf[Lines];
	TopBoarder=VcenterTable[Lines];
	BottomBoarder= 243 - (TopBoarder + LinesperScreen); //4 lines of top boarder are unrendered 244-4=240 rendered scanlines
	return;
}

void SelectCPUExec(SystemState2* EmuState)
{
	if(EmuState->CpuType == 1) // 6309
	{
		switch(EmuState->MouseType)
		{
			case 0: // for normal mouse
				CPUExec = HD6309Exec;
				//fprintf(stdout, "CPU Exec\n");
				break;
			
			case 1: // for Hires mouse
				CPUExec = HD6309ExecHiRes;
				//fprintf(stdout, "CPU Exec Hi-Res\n");
				break;

			default:
				break;
		}
	}
}

int CPUCycle(void)	
{
	char Message[256];

	//WriteLog(".", TOCONS);

	if (HorzInteruptEnabled)
		GimeAssertHorzInterupt();
	irq_hs(ANY);
	PakTimer();
	PicosThisLine+=PicosPerLine;	
	while (PicosThisLine>1)		
	{
		LC++;
		StateSwitch=0;
		if ((PicosToInterupt<=PicosThisLine) & IntEnable )	//Does this iteration need to Timer Interupt
			StateSwitch=1;
		if ((PicosToSoundSample<=PicosThisLine) & SndEnable)//Does it need to collect an Audio sample
			StateSwitch+=2;
		switch (StateSwitch)
		{
			case 0:		//No interupts this line
				//WriteLog("0", TOCONS);
				CyclesThisLine= CycleDrift + (PicosThisLine * CyclesPerLine * OverClock/PicosPerLine);
				if (CyclesThisLine>=1)	//Avoid un-needed CPU engine calls
					CycleDrift = CPUExec((int)floor(CyclesThisLine))+(CyclesThisLine- floor(CyclesThisLine));
				else 
					CycleDrift=CyclesThisLine;
				PicosToInterupt-=PicosThisLine;
				PicosToSoundSample-=PicosThisLine;
				PicosThisLine=0;
			break;

			case 1:		//Only Interupting
				//WriteLog("1", TOCONS);
				PicosThisLine-=PicosToInterupt;
				CyclesThisLine= CycleDrift + (PicosToInterupt * CyclesPerLine * OverClock/PicosPerLine);
				if (CyclesThisLine>=1)
					CycleDrift = CPUExec((int)floor(CyclesThisLine))+(CyclesThisLine- floor(CyclesThisLine));
				else 
					CycleDrift=CyclesThisLine;
				GimeAssertTimerInterupt();
				PicosToSoundSample-=PicosToInterupt;
				PicosToInterupt=MasterTickCounter;
			break;

			case 2:		//Only Sampling
				// WriteLog("2", TOCONS);
				PicosThisLine-=PicosToSoundSample;
				CyclesThisLine=CycleDrift +(PicosToSoundSample * CyclesPerLine * OverClock/PicosPerLine );
				if (CyclesThisLine >= 1)
				{
					//sprintf(Message, "%d ", LC);
					//WriteLog(Message, TOCONS);
					if (DC == 23 && LC == 382)
					{
						//WriteLog("\nWe are here\n", TOCONS);
					}
					CycleDrift = CPUExec((int)floor(CyclesThisLine)) + (CyclesThisLine - floor(CyclesThisLine));
				}
				else 
					CycleDrift=CyclesThisLine;
				//WriteLog("B", TOCONS);
				AudioEvent();
				//WriteLog("C", TOCONS);
				PicosToInterupt-=PicosToSoundSample;
				PicosToSoundSample=SoundInterupt;
			break;

			case 3:		//Interupting and Sampling
				//WriteLog("3", TOCONS);
				if (PicosToSoundSample<PicosToInterupt)
				{
					PicosThisLine-=PicosToSoundSample;	
					CyclesThisLine=CycleDrift +(PicosToSoundSample * CyclesPerLine * OverClock/PicosPerLine);
					if (CyclesThisLine>=1)
						CycleDrift = CPUExec((int)floor(CyclesThisLine))+(CyclesThisLine- floor(CyclesThisLine));
					else 
						CycleDrift=CyclesThisLine;
					AudioEvent();
					PicosToInterupt-=PicosToSoundSample;	
					PicosToSoundSample=SoundInterupt;
					PicosThisLine-=PicosToInterupt;

					CyclesThisLine= CycleDrift +(PicosToInterupt * CyclesPerLine * OverClock/PicosPerLine);
					if (CyclesThisLine>=1)
						CycleDrift = CPUExec((int)floor(CyclesThisLine))+(CyclesThisLine- floor(CyclesThisLine));
					else 
						CycleDrift=CyclesThisLine;
					GimeAssertTimerInterupt();
					PicosToSoundSample-=PicosToInterupt;
					PicosToInterupt=MasterTickCounter;
					break;
				}

				if (PicosToSoundSample>PicosToInterupt)
				{
					PicosThisLine-=PicosToInterupt;
					CyclesThisLine= CycleDrift +(PicosToInterupt * CyclesPerLine * OverClock/PicosPerLine);
					if (CyclesThisLine>=1)
						CycleDrift = CPUExec((int)floor(CyclesThisLine))+(CyclesThisLine- floor(CyclesThisLine));
					else 
						CycleDrift=CyclesThisLine;
					GimeAssertTimerInterupt();		
					PicosToSoundSample-=PicosToInterupt;
					PicosToInterupt=MasterTickCounter;
					PicosThisLine-=PicosToSoundSample;
					CyclesThisLine=  CycleDrift +(PicosToSoundSample * CyclesPerLine * OverClock/PicosPerLine);	
					if (CyclesThisLine>=1)
						CycleDrift = CPUExec((int)floor(CyclesThisLine))+(CyclesThisLine- floor(CyclesThisLine));
					else 
						CycleDrift=CyclesThisLine;
					AudioEvent();
					PicosToInterupt-=PicosToSoundSample;
					PicosToSoundSample=SoundInterupt;
					break;
				}
					//They are the same (rare)
			PicosThisLine-=PicosToInterupt;
			CyclesThisLine=CycleDrift +(PicosToSoundSample * CyclesPerLine * OverClock/PicosPerLine );
			if (CyclesThisLine>1)
				CycleDrift = CPUExec((int)floor(CyclesThisLine))+(CyclesThisLine- floor(CyclesThisLine));
			else 
				CycleDrift=CyclesThisLine;
			GimeAssertTimerInterupt();
			AudioEvent();
			PicosToInterupt=MasterTickCounter;
			PicosToSoundSample=SoundInterupt;
		}
	}

	return(0);
}

void SetTimerInteruptState(unsigned char State)
{
	// printf("%d", (int)State); fflush(stdout);
	TimerInteruptEnabled=State;
	return;
}

void SetInteruptTimer(unsigned short Timer)
{
	UnxlatedTickCounter=(Timer & 0xFFF);
	SetMasterTickCounter();
	return;
}

void SetTimerClockRate (unsigned char Tmp)	//1= 279.265nS (1/ColorBurst) 
{											//0= 63.695uS  (1/60*262)  1 scanline time
	TimerClockRate=!!Tmp;
	SetMasterTickCounter();
	return;
}

void SetMasterTickCounter(void)
{
	double Rate[2]={PICOSECOND/(TARGETFRAMERATE*LINESPERSCREEN),PICOSECOND/COLORBURST};

	if (UnxlatedTickCounter==0)
		MasterTickCounter=0;
	else
		MasterTickCounter=(UnxlatedTickCounter+2)* Rate[TimerClockRate];

	if (MasterTickCounter != OldMaster)  
	{
		OldMaster=MasterTickCounter;
		PicosToInterupt=MasterTickCounter;
	}

	if (MasterTickCounter!=0)
		IntEnable=1;
	else
		IntEnable=0;

	return;
}

void MiscReset(void)
{
	HorzInteruptEnabled=0;
	VertInteruptEnabled=0;
	TimerInteruptEnabled=0;
	MasterTimer=0; 
	TimerClockRate=0;
	MasterTickCounter=0;
	UnxlatedTickCounter=0;
	OldMaster=0;
//*************************
	SoundInterupt=0;//PICOSECOND/44100;
	PicosToSoundSample=SoundInterupt;
	CycleDrift=0;
	CyclesThisLine=0;
	PicosThisLine=0;
	IntEnable=0;
	AudioIndex=0;
	ResetAudioSDL();
	return;
}

unsigned short SetAudioRate (unsigned short Rate)
{

	SndEnable=1;
	SoundInterupt=0;
	CycleDrift=0;
	AudioIndex=0;
	if (Rate != 0)	//Force Mute or 44100Hz
		Rate = 44100;

	if (Rate==0)
		SndEnable=0;
	else
	{
		SoundInterupt=PICOSECOND/Rate;
		PicosToSoundSample=SoundInterupt;
	}
	SoundRate=Rate;
	return(0);
}

void AudioOut(void)
{
	if (AudioIndex < AUDIO_BUF_LEN)
	{
		AudioBuffer[AudioIndex++]=GetDACSample();
	}
	return;
}

void CassOut(void)
{
	if (AudioIndex < CAS_BUF_LEN)
	{
		CassBuffer[AudioIndex++]=GetCasSample();
	}
	return;
}

void CassIn(void)
{
	if (AudioIndex < AUDIO_BUF_LEN)
	{
		AudioBuffer[AudioIndex]=GetDACSample();
		SetCassetteSample(CassBuffer[AudioIndex++]);
	}
	return;
}

unsigned char SetSndOutMode(unsigned char Mode)  //0 = Speaker 1= Cassette Out 2=Cassette In
{
	static unsigned char LastMode=0;
	static unsigned short PrimarySoundRate; 	PrimarySoundRate=SoundRate;

	switch (Mode)
	{
	case 0:
		if (LastMode==1)	//Send the last bits to be encoded
			FlushCassetteBuffer(CassBuffer,AudioIndex);

		AudioEvent=AudioOut;
		SetAudioRate (PrimarySoundRate);
		break;

	case 1:
		AudioEvent=CassOut;
		PrimarySoundRate=SoundRate;
		SetAudioRate (TAPEAUDIORATE);
		break;

	case 2:
		AudioEvent=CassIn;
		PrimarySoundRate=SoundRate;;
		SetAudioRate (TAPEAUDIORATE);
		break;

	default:	//QUERY
		return(SoundOutputMode);
		break;
	}

	if (Mode != LastMode)
	{
		AudioIndex=0;	//Reset Buffer on true mode switch
		LastMode=Mode;
	}

	SoundOutputMode=Mode;
	return(SoundOutputMode);
}
