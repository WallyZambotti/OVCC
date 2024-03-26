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
#include <stdio.h>
#include "harddisk.h"
#include <sys/types.h>
#include <unistd.h>
#include "cc3vhd.h"
#include "defines.h"
#include "cloud9.h"
#include "fileops.h"

static char moduleName[24] = { "Hard Drive + Cloud9 RTC" };

size_t EXTROMSIZE = 8192;
static char HDDfilename[MAX_PATH] = { 0 };
static char IniFile[MAX_PATH] = { 0 };

// typedef unsigned char (*MEMREAD8)(unsigned short);
// typedef void (*MEMWRITE8)(unsigned char,unsigned short);
typedef void (*ASSERTINTERUPT) (unsigned char,unsigned char);
typedef void (*DMAMEMPOINTERS) ( MEMREAD8,MEMWRITE8);
static void (*AssertInt)(unsigned char,unsigned char)=NULL;
static unsigned char (*MemRead8)(unsigned short)=NULL;
static void (*MemWrite8)(unsigned char,unsigned short)=NULL;
// static unsigned char *Memory=NULL;
static unsigned char DiskRom[8192];
static unsigned char ClockEnabled=1,ClockReadOnly=1;
static void LoadHardDisk(AG_Event *);
static void LoadConfig(void);
static void SaveConfig(char*);
static void BuildMenu(void);
static void UpdateMenu(int);
static unsigned char LoadExtRom( char *);
static char *PakRomAddr = NULL;

AG_MenuItem *menuAnchor = NULL;
AG_MenuItem *itemMenu[2] = { NULL, NULL };
AG_MenuItem *itemEjectHDD[2] = { NULL, NULL };
AG_MenuItem *itemLoadHDD[2] = { NULL, NULL };
AG_MenuItem *itemSeperator = NULL;

INIman *iniman = NULL;

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
}

void ADDCALL ModuleConfig(unsigned char func)
{
	switch (func)
	{
	case 0: // Destroy Menus
		if (itemEjectHDD[0])
			AG_MenuDel(itemEjectHDD[0]);
		itemEjectHDD[0] = NULL;
		if (itemLoadHDD[0])
			AG_MenuDel(itemLoadHDD[0]);
		itemLoadHDD[0] = NULL;
		if (itemMenu[0])
			AG_MenuDel(itemMenu[0]);
		itemMenu[0] = NULL;
		if (itemEjectHDD[1])
			AG_MenuDel(itemEjectHDD[1]);
		itemEjectHDD[1] = NULL;
		if (itemLoadHDD[1])
			AG_MenuDel(itemLoadHDD[1]);
		itemLoadHDD[1] = NULL;
		if (itemMenu[1])
			AG_MenuDel(itemMenu[1]);
		itemMenu[1] = NULL;
		if (itemSeperator)
			AG_MenuDel(itemSeperator);
		itemSeperator = NULL;
		UnmountHD(0);
		UnmountHD(1);
		break;

	case 1: // Update ini file
		strcpy(IniFile, iniman->files[iniman->lastfile].name);
		break;

	break;

	default:
		break;
	}
}

unsigned char ADDCALL ModuleReset(void)
{
	if (PakRomAddr != NULL) 
	{
		memcpy(PakRomAddr, DiskRom, EXTROMSIZE);
	}
}

void ADDCALL PakRomShare(char *pakromaddr)
{
	PakRomAddr = pakromaddr;
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
}

//void ADDCALL SetIniPath (char *IniFilePath)
void ADDCALL SetIniPath(INIman *InimanP)
{
	//strcpy(IniFile,IniFilePath);
	strcpy(IniFile, InimanP->files[InimanP->lastfile].name);
	InitPrivateProfile(InimanP);
	iniman = InimanP;
	LoadConfig();
}

/*
void CPUAssertInterupt(unsigned char Interupt,unsigned char Latencey)
{
	AssertInt(Interupt,Latencey);
	return;
}
*/

static void LoadHDD(AG_Event *event)
{
	int disk = AG_INT(1);
	char *hddfile = AG_STRING(2), entry[16];
	//AG_FileType *ft = AG_PTR(2);

    if (AG_FileExists(hddfile))
    {
		if (MountHD(hddfile, disk) == 0)
		{
			AG_TextMsg(AG_MSG_ERROR, "Can't mount hard disk file %s", hddfile);
		}
    }

	AG_Strlcpy(HDDfilename, hddfile, sizeof(HDDfilename));
	sprintf(entry, "VHDImage%d", disk);
	SaveConfig(entry);
	UpdateMenu(disk);
}

static void LoadHardDisk(AG_Event *event)
{
	int disk = AG_INT(1);

	AG_Window *fdw = AG_WindowNew(0);
	AG_WindowSetCaption(fdw, "Select HDD file");
	AG_WindowSetGeometryAligned(fdw, AG_WINDOW_ALIGNMENT_NONE, 500, 500);
	AG_WindowSetCloseAction(fdw, AG_WINDOW_DETACH);

	AG_FileDlg *fd = AG_FileDlgNew(fdw, AG_FILEDLG_EXPAND | AG_FILEDLG_CLOSEWIN | AG_FILEDLG_MASK_EXT);
	AG_FileDlgSetDirectory(fd, ".");

	AG_FileDlgAddType(fd, "CoCo Hard Disk File", "*.vhd",	LoadHDD, "%i", disk);
  AG_WindowShow(fdw);
}

static void LoadConfig(void)
{
	char DiskRomPath[MAX_PATH]; 
	char entry[16];
	AG_DataSource *hr = NULL;
	int disk = 0;

	for(disk = 0 ; disk < 2 ; disk++)
	{
		sprintf(entry, "VHDImage%d", disk);
		GetPrivateProfileString(moduleName, entry, "", HDDfilename, MAX_PATH, IniFile);
		//fprintf(stderr, "Hard Disk : loadConfig : VHDImage %s %s\n", IniFile, HDDfilename);

		hr = AG_OpenFile(HDDfilename, "r+");

		if (hr == NULL)
		{
			strcpy(HDDfilename,"");
			WritePrivateProfileString(moduleName, entry, HDDfilename, IniFile);
		}
		else
		{
			AG_CloseFile(hr);
			//fprintf(stderr, "Hard Disk : loadConfig : Mounting %s %s\n", IniFile, HDDfilename);
			MountHD(HDDfilename, disk);
		}

		UpdateMenu(disk);
	}

	getcwd(DiskRomPath, sizeof(DiskRomPath));
	strcat(DiskRomPath, "/rgbdos.rom");
	LoadExtRom(DiskRomPath);
}

static void SaveConfig(char *entry)
{
	ValidatePath(HDDfilename);
	WritePrivateProfileString(moduleName, entry, HDDfilename, IniFile);
}

void UnloadHardDisk(AG_Event *event)
{
	char entry[16];
	int disk = AG_INT(1);
	UnmountHD(disk);
	AG_Strlcpy(HDDfilename, "", sizeof(HDDfilename));
	sprintf(entry, "VHDImage%d", disk);
	SaveConfig(entry);
	UpdateMenu(disk);
}

static void BuildMenu(void)
{
	int drv;
	char disklabel[16];

	if (itemSeperator != NULL) return;

	itemSeperator = AG_MenuSeparator(menuAnchor);

	for (drv = 0 ; drv < 2 ; drv++)
	{
		sprintf(disklabel, "Hard Disk %d", drv);

		itemMenu[drv] = AG_MenuNode(menuAnchor, disklabel, NULL);
		{
				itemLoadHDD[drv] = AG_MenuAction(itemMenu[drv], "Insert Disk", NULL, LoadHardDisk, "%i", drv);
				itemEjectHDD[drv] = AG_MenuAction(itemMenu[drv], 
						"Eject :                            ", 
						NULL, UnloadHardDisk, "%i", drv);
		}
	}
}

static void UpdateMenu(int Drive)
{
	char hddname[MAX_PATH];

	AG_Strlcpy(hddname, HDDfilename, sizeof(hddname));
	PathStripPath(hddname);

	AG_MenuSetLabel(itemEjectHDD[Drive], "Eject : %s", hddname);
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
