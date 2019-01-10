// /*
// Copyright 2015 by Joseph Forgione
// This file is part of VCC (Virtual Color Computer).

//     VCC (Virtual Color Computer) is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.

//     VCC (Virtual Color Computer) is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.

//     You should have received a copy of the GNU General Public License
//     along with VCC (Virtual Color Computer).  If not, see <http://www.gnu.org/licenses/>.
// */
// // hardisk.cpp : Defines the entry point for the DLL application.

typedef int BOOL;

#include <agar/core.h>
#include <agar/gui.h>
#include "harddisk.h"
#include <stdio.h>
#include <unistd.h>
#include "cc3vhd.h"
#include "defines.h"
#include "cloud9.h"
#include "profport.h"
#include "fileops.h"

static char moduleName[24] = { "Hard Drive + Cloud9 RTC" };

size_t EXTROMSIZE = 8192;
static char HDDfilename[MAX_PATH] = { 0 };
static char IniFile[MAX_PATH] = { 0 };

//typedef unsigned char (*MEMREAD8)(unsigned short);
//typedef void (*MEMWRITE8)(unsigned char,unsigned short);
// typedef void (*ASSERTINTERUPT) (unsigned char,unsigned char);
// typedef void (*DMAMEMPOINTERS) ( MEMREAD8,MEMWRITE8);
// static void (*AssertInt)(unsigned char,unsigned char)=NULL;
static unsigned char (*MemRead8)(unsigned short);
static void (*MemWrite8)(unsigned char,unsigned short);
// static unsigned char *Memory=NULL;
static unsigned char DiskRom[8192];
static unsigned char ClockEnabled=1,ClockReadOnly=1;
static void LoadHardDisk(AG_Event *);
static void LoadConfig(void);
static void SaveConfig(void);
static void BuildMenu(void);
static void UpdateMenu(void);
static unsigned char LoadExtRom( char *);

AG_MenuItem *menuAnchor = NULL;
AG_MenuItem *itemMenu = NULL;
AG_MenuItem *itemEjectHDD = NULL;
AG_MenuItem *itemLoadHDD = NULL;
AG_MenuItem *itemSeperator = NULL;

void __attribute__ ((constructor)) initLibrary(void) {
 //
 // Function that is called when the library is loaded
 //
 //   printf("Library is initialized\n"); 
}

void __attribute__ ((destructor)) cleanUpLibrary(void) {
 //
 // Function that is called when the library is »closed«.
 //
 //   printf("Library is exited\n"); 
}

void MemWrite(unsigned char Data, unsigned short Address)
{
	MemWrite8(Data,Address);
	return;
}

unsigned char MemRead(unsigned short Address)
{
	return(MemRead8(Address));
}

void ADDCALL ModuleName(char *ModName, AG_MenuItem *Temp)
{

	menuAnchor = Temp;
	strcpy(ModName, moduleName);
	SetClockWrite(!ClockReadOnly);

	if (menuAnchor != NULL) 
	{
		BuildMenu();
	}

	return ;
}

void ADDCALL ModuleConfig(unsigned char func)
{
	switch (func)
	{
	case 0: // Destroy Menus
		AG_MenuDel(itemEjectHDD);
		AG_MenuDel(itemLoadHDD);
		AG_MenuDel(itemMenu);
		AG_MenuDel(itemSeperator);
		break;

	default:
		break;
	}

	return;
}

/*
// This captures the Fuction transfer point for the CPU assert interupt 
void AssertInterupt(ASSERTINTERUPT Dummy)
{
	AssertInt=Dummy;
	return;
}
*/

void ADDCALL PackPortWrite(unsigned char Port, unsigned char Data)
{
	IdeWrite(Data,Port);
	return;
}

unsigned char ADDCALL PackPortRead(unsigned char Port)
{
	if ( ((Port==0x78) | (Port==0x79) | (Port==0x7C)) & ClockEnabled)
	{
		return(ReadTime(Port));
	}
	
	return(IdeRead(Port));
}

/*
void HeartBeat(void)
{
	PingFdc();
	return;
}
*/

// //This captures the pointers to the MemRead8 and MemWrite8 functions. This allows the DLL to do DMA xfers with CPU ram.

void ADDCALL MemPointers(MEMREAD8 Temp1, MEMWRITE8 Temp2)
{
	MemRead8=Temp1;
	MemWrite8=Temp2;
	return;
}

unsigned char ADDCALL PakMemRead8(unsigned short Address)
{
	return(DiskRom[Address & 8191]);
}

/*
void PakMemWrite8(unsigned char Data,unsigned short Address)
{
	return;
}
*/

void ADDCALL ModuleStatus(char *MyStatus)
{
	DiskStatus(MyStatus);
	return ;
}

void ADDCALL SetIniPath (char *IniFilePath)
{
	strcpy(IniFile, IniFilePath);
	LoadConfig();
	return;
}

/*
void CPUAssertInterupt(unsigned char Interupt,unsigned char Latencey)
{
	AssertInt(Interupt,Latencey);
	return;
}
*/

static int LoadHDD(AG_Event *event)
{
	char *hddfile = AG_STRING(1);
	//AG_FileType *ft = AG_PTR(2);

    if (AG_FileExists(hddfile))
    {
		if (MountHD(hddfile) == 0)
		{
			AG_TextMsg(AG_MSG_ERROR, "Can't mount hard disk file %s", hddfile);
		}
    }

	AG_Strlcpy(HDDfilename, hddfile, sizeof(HDDfilename));
	SaveConfig();
	UpdateMenu();

    return 0;
}

static void LoadHardDisk(AG_Event *event)
{
    AG_Window *fdw = AG_WindowNew(AG_WINDOW_DIALOG);
    AG_WindowSetCaption(fdw, "Select HDD file");
    AG_WindowSetGeometryAligned(fdw, AG_WINDOW_ALIGNMENT_NONE, 500, 500);
    AG_WindowSetCloseAction(fdw, AG_WINDOW_DETACH);

    AG_FileDlg *fd = AG_FileDlgNew(fdw, AG_FILEDLG_EXPAND | AG_FILEDLG_CLOSEWIN | AG_FILEDLG_MASK_EXT);
    AG_FileDlgSetDirectory(fd, ".");

	AG_FileDlgAddType(fd, "CoCo Hard Disk File", "*.vhd",	LoadHDD, NULL);
    AG_WindowShow(fdw);
}

static void LoadConfig(void)
{
	char DiskRomPath[MAX_PATH]; 
	AG_DataSource *hr = NULL;

	GetPrivateProfileString(moduleName, "VHDImage", "", HDDfilename, MAX_PATH, IniFile);
	//fprintf(stderr, "Hard Disk : loadConfig : VHDImage %s %s\n", IniFile, HDDfilename);

	hr = AG_OpenFile(HDDfilename, "r+");

	if (hr == NULL)
	{
		strcpy(HDDfilename,"");
		WritePrivateProfileString(moduleName, "VHDImage", HDDfilename, IniFile);
	}
	else
	{
		AG_CloseFile(hr);
		//fprintf(stderr, "Hard Disk : loadConfig : Mounting %s %s\n", IniFile, HDDfilename);
		MountHD(HDDfilename);
	}

	UpdateMenu();

	getcwd(DiskRomPath, sizeof(DiskRomPath));
	strcat(DiskRomPath, "/rgbdos.rom");
	LoadExtRom(DiskRomPath);
	return;
}

static void SaveConfig(void)
{
	ValidatePath(HDDfilename);
	WritePrivateProfileString(moduleName, "VHDImage", HDDfilename, IniFile);
	return;
}

void UnloadHardDisk(AG_Event *event)
{
	UnmountHD();
	AG_Strlcpy(HDDfilename, "", sizeof(HDDfilename));
	SaveConfig();
	UpdateMenu();
}

static void BuildMenu(void)
{
	if (itemMenu == NULL)
	{
		itemSeperator = AG_MenuSeparator(menuAnchor);
	    itemMenu = AG_MenuNode(menuAnchor, "Hard Disk", NULL);
        {
            itemLoadHDD = AG_MenuAction(itemMenu, "Insert Disk", NULL, LoadHardDisk, NULL);
            itemEjectHDD = AG_MenuAction(itemMenu, 
                "Eject :                            ", 
                NULL, UnloadHardDisk, NULL);
		}
	}
}

static void UpdateMenu(void)
{
	char hddname[MAX_PATH];

	AG_Strlcpy(hddname, HDDfilename, sizeof(hddname));
	PathStripPath(hddname);

	AG_MenuSetLabel(itemEjectHDD, "Eject : %s", hddname);
}

static unsigned char LoadExtRom( char *FilePath)	//Returns 1 on if loaded
{

	FILE *rom_handle = NULL;
	unsigned short index=0;
	unsigned char RetVal=0;

	//fprintf(stderr, "Hard Disk : LoadExtRom : %s\n", FilePath);

	rom_handle = fopen(FilePath,"rb");

	if (rom_handle == NULL)
	{
		memset(DiskRom, 0xFF, EXTROMSIZE);
	}
	else
	{
		while ((feof(rom_handle)==0) & (index<EXTROMSIZE))
		{
			DiskRom[index++]=fgetc(rom_handle);
		}
		
		RetVal = 1;
		fclose(rom_handle);
	}

	return(RetVal);
}