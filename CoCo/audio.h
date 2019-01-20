#ifndef __AUDIO_H__
#define __AUDIO_H__
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

#define WIN_SDL 2
#define CARDNAME_LEN 256

int SoundInitSDL (int ,unsigned short);
int SoundDeInitSDL(void);
void FlushAudioBufferSDL ( unsigned int *,unsigned short);
void ResetAudioSDL (void);
unsigned char PauseAudioSDL(unsigned char Pause);
int GetFreeBlockCount(void);
int GetAuxBlockCount(void);
void PurgeAuxBuffer(void);
unsigned short GetSoundStatus(void);
unsigned short GetSoundStatusSDL(void);
typedef struct CardList {
	char CardName[CARDNAME_LEN];
    int  sdlID;
} SndCardList;

int GetSoundCardListSDL(SndCardList *);

#endif
