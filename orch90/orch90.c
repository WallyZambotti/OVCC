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
typedef int BOOL;

#include <stdio.h>
#include <string.h>
#include "defines.h"
#include "orch90.h"

static char moduleName[13] = { "Orchestra-90" };

typedef void (*SETCART)(unsigned char);
typedef void (*SETCARTPOINTER)(SETCART);
typedef void (*DYNAMICMENUCALLBACK)( char *,int, int);
static unsigned char LeftChannel=0,RightChannel=0;
static void (*PakSetCart)(unsigned char)=NULL;
static unsigned char LoadExtRom(char *);
static unsigned char Rom[8192];

void __attribute__ ((constructor)) initLibrary(void) {
 //
 // Function that is called when the library is loaded
 //
 //   printf("ORCH90 is initialized\n"); 
}

void __attribute__ ((destructor)) cleanUpLibrary(void) {
 //
 // Function that is called when the library is »closed«.
 //
 //   printf("ORCH90 is exited\n"); 
}

void ADDCALL ModuleName(char *ModName, void *Temp)
{
	ModName = strcpy(ModName, moduleName);
	//printf("Orch90 : ModuleName\n");
	return ;
}

void ADDCALL PackPortWrite(unsigned char Port,unsigned char Data)
{
	switch (Port)
	{
	case 0x7A:
		RightChannel=Data;			
		break;

	case 0x7B:
		LeftChannel=Data;
		break;
	}
	return;
}

unsigned char ADDCALL PackPortRead(unsigned char Port)
{
	return(0);
}

unsigned char ADDCALL ModuleReset(void)
{
	char RomPath[MAX_PATH];

	//printf("Orch90 : ModuleReset\n");

	memset(Rom, 0xff, 8192);
	strcpy(RomPath, "orch90.rom");
	
	if (PakSetCart != NULL) 
	{
		if (LoadExtRom(RomPath))
		{	//If we can load the rom them assert cart 
			//printf("Orch90 : Loaded orch90.rom\n");
			PakSetCart(1);
		}
		else
		{
			printf("Orch90 : couldn't load orch90.rom\n");
		}
	}
	else
	{
		printf("Orch90 : No PakSetcart\n");
	}

	return(0);
}

unsigned char ADDCALL SetCart(SETCART Pointer)
{
	PakSetCart=Pointer;
	//printf("Orch90 : SetCart\n");
	return(0);
}

unsigned char ADDCALL PakMemRead8(unsigned short Address)
{
	//printf("%2x", (int)Rom[Address & 8191]);
	return(Rom[Address & 8191]);
}

// This gets called at the end of every scan line 262 Lines * 60 Frames = 15780 Hz 15720
unsigned short ADDCALL ModuleAudioSample(void)
{
	return((short)((LeftChannel<<8) | RightChannel)) ;
}

void ADDCALL ModuleConfig(unsigned char func)
{
}

static unsigned char LoadExtRom(char *FilePath)	//Returns 1 on if loaded
{
	FILE *rom_handle = NULL;
	unsigned short index = 0;
	unsigned char RetVal = 0;

	rom_handle = fopen(FilePath, "rb");
	if (rom_handle == NULL)
		memset(Rom, 0xFF, 8192);
	else
	{
		while ((feof(rom_handle) == 0) & (index<8192))
			Rom[index++] = fgetc(rom_handle);
		RetVal = 1;
		fclose(rom_handle);
	}
	return(RetVal);
}