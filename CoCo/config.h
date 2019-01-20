#ifndef __CONFIG_H__
#define __CONFIG_H__
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

#include "audio.h"

void LoadConfig(SystemState2 *);
unsigned char WriteIniFile(void);
unsigned char ReadIniFile(void);
char * BasicRomName(void);
void GetIniFilePath( char *);
void UpdateConfig (void);
void UpdateSoundBar(unsigned short,unsigned short);
void UpdateTapeCounter(unsigned int,unsigned char);
void UpdateOnBoot(char *);
unsigned short TranslateDisp2Scan(int);

typedef struct  {
	unsigned short	CPUMultiplyer;
	unsigned short	MaxOverclock;
	unsigned char	FrameSkip;
	unsigned char	SpeedThrottle;
	unsigned char	CpuType;
	unsigned char	MmuType;
//	unsigned char	AudioMute;
	unsigned char	MonitorType;
	unsigned char	ScanLines;
	unsigned char	Resize;
	unsigned char	Aspect;
	unsigned char	RamSize;
	unsigned char	AutoStart;
	unsigned char	CartAutoStart;
	unsigned char	ThreadSafeLocking;
	unsigned char	RebootNow;
	unsigned char	SndOutDev;
	unsigned char	KeyMap;
	char			SoundCardName[CARDNAME_LEN];
	unsigned short	AudioRate;
	char			ExternalBasicImage[MAX_PATH];
	char			ModulePath[MAX_PATH];
	char			PathtoExe[MAX_PATH];
} STRConfig;
	
#endif

