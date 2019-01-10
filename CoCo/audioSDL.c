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

#include <stdio.h>
#include "defines.h"
#include "vcc.h"
#include "config.h"
#include "coco3.h"
#include "audio.h"
#include "logger.h"

#define MAXCARDS	12
//PlayBack
static SDL_AudioDeviceID audioDev = NULL;
static SDL_AudioSpec audioSpec, actualAudioSpec;
//Record

static unsigned short BitRate=0;
static unsigned char InitPassed=0;
static int CardCount=0;
static unsigned short CurrentRate=0;
static unsigned char AudioPause=0;
static SndCardList *Cards=NULL;

void dumpbuffer(unsigned char *Abuffer, int length)
{
	size_t byteswritten;
	AG_DataSource *af = AG_OpenFile("audio.raw", "a+");

	AG_WriteP(af, Abuffer, (size_t)length, &byteswritten);

	AG_CloseFile(af);
}

int SoundInitSDL (int devID, unsigned short Rate)
{
	if (!SDL_WasInit(SDL_INIT_AUDIO))
	{
		if (SDL_Init(SDL_INIT_AUDIO) != 0)
		{
			fprintf(stderr, "Couldn't initialise SDL_Audio\n");
			return 0;
		}
	}

	Rate=(Rate & 3);
	if (Rate != 0)	//Force 44100 or Mute
		Rate = 3;

	CurrentRate=Rate;

	if (InitPassed)
	{
		InitPassed=0;
		SDL_PauseAudioDevice(audioDev, 1);

		if (audioDev!=NULL)
		{
			SDL_CloseAudioDevice(audioDev);
			audioDev=NULL;
		}
	}

	BitRate=iRateList[Rate];

	if (Rate)
	{
		audioSpec.callback = NULL;
		audioSpec.channels = 2;
		audioSpec.freq = BitRate;
		audioSpec.format = AUDIO_S16LSB;
		audioSpec.samples = 4096;

		audioDev = SDL_OpenAudioDevice(Cards[devID].CardName, 0, &audioSpec, &actualAudioSpec, 0);
		//audioDev = SDL_OpenAudioDevice(Cards[devID].CardName, 0, NULL, NULL, 0);

		if (audioDev == 0)
			return(1);

		//SDL_PauseAudioDevice(audioDev, 0);

		InitPassed=1;
		AudioPause=0;
	}

	SetAudioRate (iRateList[Rate]);
	return(0);
}

void FlushAudioBufferSDL(unsigned int *Abuffer,unsigned short Length)
{
	unsigned short LeftAverage=0,RightAverage=0,Index=0;
	unsigned char Flag=0;
	LeftAverage=Abuffer[0]>>16;
	RightAverage=Abuffer[0]& 0xFFFF;
	UpdateSoundBar(LeftAverage,RightAverage);

	if ((!InitPassed) | (AudioPause))
		return;

	SDL_QueueAudio(audioDev, (void*)Abuffer, (Uint32)Length);
	SDL_PauseAudioDevice(audioDev, 0);
	Uint32 as = SDL_GetQueuedAudioSize(audioDev);

	//dumpbuffer(Abuffer, Length);
}

int GetSoundCardListSDL (SndCardList *List)
{
	if (!SDL_WasInit(SDL_INIT_AUDIO))
	{
		if (SDL_Init(SDL_INIT_AUDIO) != 0)
		{
			fprintf(stderr, "Couldn't initialise SDL_Audio\n");
			return 0;
		}
	}

	int totAudioDevs = SDL_GetNumAudioDevices(0);
	CardCount=0;
	Cards=List;

	for(CardCount = 0 ; CardCount < totAudioDevs && CardCount < MAXCARDS ; CardCount++)
	{
		strcpy(Cards[CardCount].CardName, SDL_GetAudioDeviceName(CardCount, 0));
		Cards[CardCount].sdlID = CardCount;
	}
	return(CardCount);
}

int SoundDeInitSDL(void)
{
	if (InitPassed)
	{
		InitPassed=0;
		SDL_PauseAudioDevice(audioDev, 1);
		SDL_CloseAudioDevice(audioDev);
		audioDev = NULL;
	}
	return(0);
}

unsigned short GetSoundStatusSDL(void)
{
	return(CurrentRate);
}

void ResetAudioSDL (void)
{
	SetAudioRate (iRateList[CurrentRate]);
	return;
}

unsigned char PauseAudioSDL(unsigned char Pause)
{
	AudioPause=Pause;
	if (InitPassed)
	{
		SDL_PauseAudioDevice(audioDev, (int)AudioPause);
	}
	return(AudioPause);
}