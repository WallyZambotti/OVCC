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
#include <string.h>
#include "memboard.h"

#ifdef __MINGW32__
#define ADDCALL __cdecl
#else
#define ADDCALL
#endif

static char moduleName[8] = { "ramdisk" };

typedef unsigned char (*MEMREAD8)(unsigned short);
typedef void (*MEMWRITE8)(unsigned char,unsigned short);
typedef void (*ASSERTINTERUPT) (unsigned char,unsigned char);
typedef void (*DMAMEMPOINTERS) ( MEMREAD8,MEMWRITE8);
typedef void (*DYNAMICMENUCALLBACK)( char *,int, int);
static void (*AssertInt)(unsigned char,unsigned char)=NULL;
static unsigned char (*MemRead8)(unsigned short);
static void (*MemWrite8)(unsigned char,unsigned short);
static unsigned char *Memory=NULL;

void __attribute__ ((constructor)) initLibrary(void) {
 //
 // Function that is called when the library is loaded
 //
 //   printf("ramdisk is initialized\n"); 
}

void __attribute__ ((destructor)) cleanUpLibrary(void) {
 //
 // Function that is called when the library is »closed«.
 //
 //   printf("ramdisk is exited\n"); 
}

void ADDCALL ModuleName(char *ModName, void *Temp)
{
	ModName = strcpy(ModName, moduleName);
	InitMemBoard();	
	return ;
}

void ADDCALL PackPortWrite(unsigned char Port,unsigned char Data)
{
	switch (Port)
	{
		case 0x40:
		case 0x41:
		case 0x42:
			WritePort(Port,Data);
			return;
		break;

		case 0x43:
			WriteArray(Data);
			return;
		break;

		default:
			return;
		break;
	}	//End port switch		
	return;
}

unsigned char ADDCALL PackPortRead(unsigned char Port)
{
	switch (Port)
	{
		case 0x43:
			return(ReadArray());
		break;

		default:
			return(0);
		break;
	}	//End port switch
}
