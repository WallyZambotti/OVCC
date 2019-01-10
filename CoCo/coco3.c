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
#include "tcc1014graphicsSDL.h"
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
static unsigned int AudioBuffer[16384];
static unsigned char CassBuffer[8192];
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

float RenderFrame (SystemState2 *RFState2, unsigned long DCnt)
{
	static unsigned short FrameCounter = 0;
	//WriteLog("RF ", TOCONS);
	//fprintf(stderr, ".");
	DC = DCnt;
	LC = 0;

//********************************Start of frame Render*****************************************************
	SetBlinkStateSDL(BlinkPhase);
	irq_fs(0);				//FS low to High transition start of display Boink needs this

	for (RFState2->LineCounter=0;RFState2->LineCounter<13;RFState2->LineCounter++)		//Vertical Blanking 13 H lines 
		CPUCycle();

	for (RFState2->LineCounter=0;RFState2->LineCounter<4;RFState2->LineCounter++)		//4 non-Rendered top Boarder lines
		CPUCycle();

	if (!(FrameCounter % RFState2->FrameSkip))
	{	
		if (LockScreenSDL(RFState2))
			return(0);
	}
	for (RFState2->LineCounter=0;RFState2->LineCounter<(TopBoarder-4);RFState2->LineCounter++) 		
	{
		if (!(FrameCounter % RFState2->FrameSkip)) 
		{
			DrawTopBoarderSDL(RFState2);
		}
		CPUCycle();
	}

	for (RFState2->LineCounter=0;RFState2->LineCounter<LinesperScreen;RFState2->LineCounter++)		//Active Display area		
	{
		CPUCycle();
		if (!(FrameCounter % RFState2->FrameSkip))
		{
			UpdateScreenSDL(RFState2);
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
			DrawBottomBoarderSDL(RFState2);
		}
	}

	if (!(FrameCounter % RFState2->FrameSkip))
	{
		UnlockScreenSDL(RFState2);
		SetBoarderChangeSDL(0);
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
	AudioBuffer[AudioIndex++]=GetDACSample();
	return;
}

void CassOut(void)
{
	CassBuffer[AudioIndex++]=GetCasSample();
	return;
}

void CassIn(void)
{
	AudioBuffer[AudioIndex]=GetDACSample();
	SetCassetteSample(CassBuffer[AudioIndex++]);
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