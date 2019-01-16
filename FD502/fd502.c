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
/********************************************************************************************
*	fd502.cpp : Defines the entry point for the DLL application.							*
*	DLL for use with Vcc 1.0 or higher. DLL interface 1.0 Beta								* 
*	This Module will emulate a Tandy Floppy Disk Model FD-502 With 3 DSDD drives attached	* 
*	Copyright 2006 (c) by Joseph Forgione 													*
*********************************************************************************************/

#include <agar/core.h>
#include <agar/gui.h>
#include <stdio.h>
#include <stdlib.h>
#include "wd1793.h"
#include "distortc.h"
#include "fd502.h"
#include "defines.h"

#define EXTROMSIZE 16384

static char moduleName[13] = { "FD502 26-133" };

extern int WritePrivateProfileString(char *, char *, char *, char *);
extern int GetPrivateProfileString(char *, char *, char *, char *, int, char *);

extern DiskInfo Drive[5];
typedef unsigned char (*MEMREAD8)(unsigned short);
typedef void (*MEMWRITE8)(unsigned char,unsigned short);
typedef void (*ASSERTINTERUPT) (unsigned char,unsigned char);
typedef void (*DMAMEMPOINTERS) ( MEMREAD8,MEMWRITE8);
typedef void (*DYNAMICMENUCALLBACK)( char *,int, int);
static unsigned char ExternalRom[EXTROMSIZE];
static unsigned char DiskRom[EXTROMSIZE];
static unsigned char RGBDiskRom[EXTROMSIZE];
static char RomFileName[MAX_PATH]="";
static char TempRomFileName[MAX_PATH]="";
static void (*AssertInt)(unsigned char,unsigned char)=NULL;
static void (*DynamicMenuCallback)( char *,int, int)=NULL;
static unsigned char (*MemRead8)(unsigned short);
static void (*MemWrite8)(unsigned char,unsigned short);
static unsigned char *Memory=NULL;
unsigned char PhysicalDriveA=0,PhysicalDriveB=0,OldPhysicalDriveA=0,OldPhysicalDriveB=0;
static unsigned char *RomPointer[3]={ExternalRom,DiskRom,RGBDiskRom};
static unsigned char SelectRom=0;
unsigned char SetChip(unsigned char);
static unsigned char NewDiskNumber=0,DialogOpen=0,CreateFlag=0;
static unsigned char FileNotSelected=0;
static unsigned char TurboMode=0;
static unsigned char PersistDisks=0;
static char IniFile[MAX_PATH]="";
static unsigned char TempSelectRom=0;
static unsigned char ClockEnabled=1,ClockReadOnly=1;
static unsigned char NewDiskType=0;
static unsigned char NewDiskTracks=0;
static unsigned char DblSided=1;
//LRESULT CALLBACK Config(HWND, UINT, WPARAM, LPARAM);
//LRESULT CALLBACK NewDisk(HWND,UINT, WPARAM, LPARAM);
void LoadConfig(void);
void SaveConfig(void);
long CreateDiskHeader(char *,unsigned char,unsigned char,unsigned char);
void Load_Disk(unsigned char);

static unsigned long RealDisks=0;
long CreateDisk (unsigned char);
static char TempFileName[MAX_PATH]="";
unsigned char LoadExtRom( unsigned char,char *);

#define MAX_DRIVES 4
AG_MenuItem *menuAnchor = NULL;
AG_MenuItem *itemMenu[MAX_DRIVES] = { NULL,NULL,NULL,NULL };
AG_MenuItem *itemEjectFD[MAX_DRIVES] = { NULL,NULL,NULL,NULL };
AG_MenuItem *itemLoadFD[MAX_DRIVES] = { NULL,NULL,NULL,NULL };
AG_MenuItem *itemConfig = NULL;
AG_MenuItem *itemSeperator = NULL;


void __attribute__ ((constructor)) initLibrary(void) {
 //
 // Function that is called when the library is loaded
 //
 //   printf("FD502 is initialized\n"); 
}

void __attribute__ ((destructor)) cleanUpLibrary(void) {
 //
 // Function that is called when the library is »closed«.
 //
 //   printf("FD502 is exited\n"); 
}


void ADDCALL ModuleName(char *ModName, AG_MenuItem *Temp)
{

	menuAnchor = Temp;
	strcpy(ModName, moduleName);

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
	{
		int drv;

		AG_MenuDel(itemConfig);

		for (drv = 0 ; drv < MAX_DRIVES ; drv++)
		{
			AG_MenuDel(itemEjectFD[drv]);
			AG_MenuDel(itemLoadFD[drv]);
			AG_MenuDel(itemMenu[drv]);
		}

		AG_MenuDel(itemSeperator);
	}
	break;

	default:
		break;
	}

	return;
}

void ADDCALL SetIniPath (char *IniFilePath)
{
	strcpy(IniFile,IniFilePath);
	LoadConfig();
	return;
}

// This captures the Fuction transfer point for the CPU assert interupt 
void ADDCALL AssertInterupt(ASSERTINTERUPT Dummy)
{
	AssertInt=Dummy;
	return;
}

void ADDCALL PackPortWrite(unsigned char Port, unsigned char Data)
{
	if ( ((Port == 0x50) | (Port==0x51)) & ClockEnabled)
		write_time(Data,Port);
	else
		disk_io_write(Data,Port);
	return;
}

unsigned char ADDCALL PackPortRead(unsigned char Port)
{
	if ( ((Port == 0x50) | (Port==0x51)) & ClockEnabled)
		return(read_time(Port));
	return(disk_io_read(Port));
}

void ADDCALL HeartBeat(void)
{
	PingFdc();
	return;
}

//This captures the pointers to the MemRead8 and MemWrite8 functions. This allows the DLL to do DMA xfers with CPU ram.
/*
extern "C"
{
	__declspec(dllexport) void MemPointers(MEMREAD8 Temp1,MEMWRITE8 Temp2)
	{
		MemRead8=Temp1;
		MemWrite8=Temp2;
		return;
	}
}
*/

unsigned char PakMemRead8(unsigned short Address)
{
	return(RomPointer[SelectRom][Address & (EXTROMSIZE-1)]);
}

/*
extern "C"
{
	__declspec(dllexport) void PakMemWrite8(unsigned char Data,unsigned short Address)
	{

		return;
	}
}
*/

void ADDCALL ModuleStatus(char *MyStatus)
{
	DiskStatus(MyStatus);
	return ;
}

void CPUAssertInterupt(unsigned char Interupt,unsigned char Latencey)
{
	AssertInt(Interupt,Latencey);
	return;
}

void OKCallback(AG_Event *event)
{
	SetTurboDisk(TurboMode);

	if (!RealDisks)
	{
		PhysicalDriveA = 0;
		PhysicalDriveB = 0;
	}
	else
	{
		if (PhysicalDriveA != OldPhysicalDriveA)	//Drive changed
		{					
			if (OldPhysicalDriveA != 0)
				unmount_disk_image(OldPhysicalDriveA - 1);
			if (PhysicalDriveA != 0)
				mount_disk_image("*Floppy A:",PhysicalDriveA - 1);
		}
		if (PhysicalDriveB != OldPhysicalDriveB)	//Drive changed
		{					
			if (OldPhysicalDriveB != 0)
				unmount_disk_image(OldPhysicalDriveB - 1);
			if (PhysicalDriveB != 0)
				mount_disk_image("*Floppy B:",PhysicalDriveB - 1);
		}	
	}

	SelectRom = TempSelectRom;
	strcpy(RomFileName, TempRomFileName);
	CheckPath(RomFileName);
	LoadExtRom(External, RomFileName);
    SaveConfig();
    AG_CloseFocusedWindow();
}

void PhysDriveASelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_PTR(1);

    PhysicalDriveA = ti->label;
}

void PhysDriveBSelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_PTR(1);

    PhysicalDriveB = ti->label;	
}

int UpdateROM(AG_Event *event)
{
	char *file = AG_STRING(1);
	AG_FileType *ft = AG_PTR(2);

	AG_Strlcpy(TempRomFileName, file, sizeof(TempRomFileName));

    return 0;
}

void BrowseROM(AG_Event *event)
{
    AG_Window *fdw = AG_WindowNew(AG_WINDOW_DIALOG);
    AG_WindowSetCaption(fdw, "Disk Rom Image");
    AG_WindowSetGeometryAligned(fdw, AG_WINDOW_ALIGNMENT_NONE, 500, 500);
    AG_WindowSetCloseAction(fdw, AG_WINDOW_DETACH);

    AG_FileDlg *fd = AG_FileDlgNew(fdw, AG_FILEDLG_EXPAND | AG_FILEDLG_CLOSEWIN | AG_FILEDLG_MASK_EXT);
    AG_FileDlgSetDirectory(fd, ".");

	AG_FileDlgAddType(fd, "Disk Rom Images", "*.rom,*.bin",	UpdateROM, NULL);
    AG_WindowShow(fdw);
}

void PopulateDriveDevices(AG_Tlist *list)
{
    AG_TlistAddS(list, NULL, "None");
    AG_TlistAddS(list, NULL, "Drive 0");
    AG_TlistAddS(list, NULL, "Drive 1");
    AG_TlistAddS(list, NULL, "Drive 2");
    AG_TlistAddS(list, NULL, "Drive 3");
}

void ConfigFD502(AG_Event *event)
{
	AG_Window *win;

    if ((win = AG_WindowNewNamedS(0, "FD-502 Config")) == NULL)
    {
        return;
    }

    AG_WindowSetGeometryAligned(win, AG_WINDOW_ALIGNMENT_NONE, 440, 294);
    AG_WindowSetCaptionS(win, "FD-502 Config");
    AG_WindowSetCloseAction(win, AG_WINDOW_DETACH);

    AG_Box *hbox, *vbox;

	// Dos Image Radio items

	hbox = AG_BoxNewHoriz(win, AG_BOX_EXPAND | AG_BOX_FRAME);
	vbox = AG_BoxNewVert(hbox, AG_BOX_EXPAND);

	AG_LabelNew(vbox, 0, "Dos Image");

	const char *radioItems[] = 
	{
		"External Rom Image",
		"Disk Basic",
		"RGB Dos",
		NULL
	};

	AG_Radio *radio = AG_RadioNewFn(vbox, 0, radioItems, NULL, NULL);
	AG_BindUint8(radio, "value", &TempSelectRom);

	// Check Box items

	AG_Checkbox *xb = AG_CheckboxNewFn(vbox, 0, "OverClock Disk Drive", NULL, NULL);
	AG_BindUint8(xb, "state", &TurboMode);

	xb = AG_CheckboxNewFn(vbox, 0, "Persistent Disk Images", NULL, NULL);
	AG_BindUint8(xb, "state", &PersistDisks);

	xb = AG_CheckboxNewFn(vbox, 0, "Clock at 0xFF50-51", NULL, NULL);
	AG_BindUint8(xb, "state", &ClockEnabled);

	// OK - Cancel Buttons

	vbox = AG_BoxNewVert(hbox, 0);

	AG_ButtonNewFn(vbox, 0, "OK", OKCallback, NULL);
	AG_ButtonNewFn(vbox, 0, "Cancel", AGWINDETACH(win));

	// Physical Disks

	AG_Combo *com;
	AG_TlistItem *item;

	hbox = AG_BoxNewHoriz(win, AG_BOX_HFILL | AG_BOX_FRAME);
	vbox = AG_BoxNewVert(hbox, 0);

	AG_LabelNew(vbox, 0, "Physical Disks");

	com = AG_ComboNewS(vbox, AG_COMBO_HFILL, "A:");
	AG_ComboSizeHint(com, "Drive 0", 5);
	PopulateDriveDevices(com->list);
	item = AG_TlistFindByIndex(com->list, PhysicalDriveA);
	if (item != NULL) AG_ComboSelect(com, item);
	AG_SetEvent(com, "combo-selected", PhysDriveASelected, NULL);
	if (RealDisks) AG_WidgetEnable(com); else AG_WidgetDisable(com);

	com = AG_ComboNewS(vbox, AG_COMBO_HFILL, "B:");
	AG_ComboSizeHint(com, "Drive 0", 5);
	PopulateDriveDevices(com->list);
	item = AG_TlistFindByIndex(com->list, PhysicalDriveB);
	if (item != NULL) AG_ComboSelect(com, item);
	AG_SetEvent(com, "combo-selected", PhysDriveBSelected, NULL);
	if (RealDisks) AG_WidgetEnable(com); else AG_WidgetDisable(com);

	vbox = AG_BoxNewVert(hbox, 0);

	AG_LabelNew(vbox, 0, "Windows 2000 or higher and FDRAWREAD");
	AG_LabelNew(vbox, 0, "driver are required for Physical Disk access");

	// External Disk ROM Image

	hbox = AG_BoxNewHoriz(win, AG_HBOX_HFILL | AG_BOX_FRAME);
	vbox = AG_BoxNewVert(hbox, AG_VBOX_HFILL);

	AG_LabelNew(vbox, NULL, "External Disk ROM Image");

    AG_Label *lab = AG_LabelNewPolled(vbox, AG_LABEL_HFILL | AG_LABEL_FRAME, "%s", TempRomFileName);
	AG_LabelSizeHint(lab, 1, "/home/user/folder/path/directory/longfile.ext");

	vbox = AG_BoxNewVert(hbox, 0);

    AG_ButtonNewFn(vbox, 0, "Browse", BrowseROM, NULL);

	AG_WindowShow(win);
}

void MountDiskFile(char *diskname, int disk)
{
	if (mount_disk_image(TempFileName, disk) == 0)
	{	
		AG_TextMsg(AG_MSG_INFO, "Can't open disk file!");
	}
	else
	{
		FileNotSelected = 0 ;
		SaveConfig();
		UpdateMenu(disk);
	}
}

int CreateCallback(AG_Event *event)
{
	int disk = AG_INT(1);
	char *file = AG_STRING(2);

	AG_Strlcpy(TempFileName, file, sizeof(TempFileName));

	PathRemoveExtension(TempFileName);
	strcat(TempFileName,".dsk");

	if(CreateDiskHeader(TempFileName, NewDiskType, NewDiskTracks, DblSided))
	{
		strcpy(TempFileName,"");
		AG_TextMsg(AG_MSG_INFO, "Can't create disk image file!");
	}
	else
	{
		MountDiskFile(TempFileName, disk);
	}

    AG_CloseFocusedWindow();
}

void CreateNewDisk(char *diskname, int disk)
{
	AG_Window *win;

    if ((win = AG_WindowNewNamedS(0, "Insert Disk Image")) == NULL)
    {
        return;
    }

    AG_WindowSetGeometryAligned(win, AG_WINDOW_ALIGNMENT_NONE, 440, 270);
    AG_WindowSetCaptionS(win, "Insert Disk Image");
    AG_WindowSetCloseAction(win, AG_WINDOW_DETACH);

    AG_Box *box, *hbox, *vbox;

	box = AG_BoxNewVert(win, AG_BOX_EXPAND);

	// Image Type

	hbox = AG_BoxNewHoriz(box, AG_BOX_FRAME);
	vbox = AG_BoxNewVert(hbox, AG_BOX_FRAME);

	AG_LabelNew(vbox, 0, "Image Type");

	const char *radioItems[] = 
	{
		"DMK",
		"JVC",
		"VDK",
		NULL
	};

	AG_Radio *radio = AG_RadioNewFn(vbox, 0, radioItems, NULL, NULL);
	AG_BindUint8(radio, "value", &NewDiskType);

	// Tracks

	vbox = AG_BoxNewVert(hbox, AG_BOX_FRAME);

	AG_LabelNew(vbox, 0, "Tracks");

	const char *radioItems2[] = 
	{
		"35",
		"40",
		"80",
		NULL
	};

	radio = AG_RadioNewFn(vbox, 0, radioItems2, NULL, NULL);
	AG_BindUint8(radio, "value", &NewDiskTracks);

	// Info

	vbox = AG_BoxNewVert(hbox, AG_BOX_FRAME);

	AG_LabelNew(vbox, 0, diskname);
	AG_SeparatorNewHoriz(vbox);
	AG_LabelNew(vbox, 0, "This file does not exist");
	AG_LabelNew(vbox, 0, "");
	AG_LabelNew(vbox, 0, "Create this file?");

	// Double Sided / Ok / Cancel	

	hbox = AG_BoxNewHoriz(box, 0);

	AG_Checkbox *xb = AG_CheckboxNewFn(hbox, 0, "Double Sided", NULL, NULL);
	AG_BindUint8(xb, "state", &DblSided);

	AG_ButtonNewFn(hbox, 0, "OK", CreateCallback, "%i,%s", disk, diskname);
	AG_ButtonNewFn(hbox, 0, "Cancel", AGWINDETACH(win));

	AG_WindowShow(win);
}


void UpdateMenu(int disk)
{
	char hddname[MAX_PATH];

	AG_Strlcpy(hddname, Drive[disk].ImageName, sizeof(hddname));
	PathStripPath(hddname);

	AG_MenuSetLabel(itemEjectFD[disk], "Eject : %s", hddname);
}
 

void UnloadFD502(AG_Event *event)
{
	int disk = AG_INT(1);

	unmount_disk_image(disk);
	UpdateMenu(disk);
}

int LoadFD502(AG_Event *event)
{
	int disk = AG_INT(1);
	char *file = AG_STRING(2);
	AG_FileType *ft = AG_PTR(3);

	AG_Strlcpy(TempFileName, file, sizeof(TempFileName));

	if (!AG_FileExists(TempFileName)) 
	{
		NewDiskNumber = disk;
		CreateNewDisk(TempFileName, disk);
	}
	else
	{
		MountDiskFile(TempFileName, disk);
	}
}

void BrowseFD502(AG_Event *event)
{
	int disk = AG_INT(1);
	
    AG_Window *fdw = AG_WindowNew(AG_WINDOW_DIALOG);
    AG_WindowSetCaption(fdw, "Insert FD Image");
    AG_WindowSetGeometryAligned(fdw, AG_WINDOW_ALIGNMENT_NONE, 500, 500);
    AG_WindowSetCloseAction(fdw, AG_WINDOW_DETACH);

    AG_FileDlg *fd = AG_FileDlgNew(fdw, AG_FILEDLG_EXPAND | AG_FILEDLG_CLOSEWIN | AG_FILEDLG_MASK_EXT);
    AG_FileDlgSetDirectory(fd, ".");

	AG_FileDlgAddType(fd, "Disk Images", "*.dsk,*.os9",	LoadFD502, "%i", disk);
    AG_WindowShow(fdw);
}

unsigned char SetChip(unsigned char Tmp)
{
	if (Tmp!=QUERY)
		SelectRom=Tmp;
	return(SelectRom);
}

void BuildMenu(void)
{
	int drv = 0;
	char drvname[32];

	if (itemConfig != NULL) return;

	itemSeperator = AG_MenuSeparator(menuAnchor);

	for (drv = 0 ; drv < MAX_DRIVES ; drv++)
	{
		sprintf(drvname, "FD-502 Drive %i", drv);
		itemMenu[drv] = AG_MenuNode(menuAnchor, drvname, NULL);
		{
			itemLoadFD[drv] = AG_MenuAction(itemMenu[drv], "Insert", NULL, BrowseFD502, "%i", drv);
			itemEjectFD[drv] = AG_MenuAction(itemMenu[drv], "Eject :                            ", NULL, UnloadFD502, "%i", drv);
		}
	}

	itemConfig = AG_MenuNode(menuAnchor, "FD-502 Config", NULL);
	AG_MenuAction(itemConfig, "Config", NULL, ConfigFD502, NULL);
}

long CreateDiskHeader(char *FileName, unsigned char Type, unsigned char Tracks, unsigned char DblSided)
{
	AG_DataSource *hr=NULL;
	unsigned char Dummy=0;
	unsigned char HeaderBuffer[16]="";
	unsigned char TrackTable[3]={35,40,80};
	unsigned short TrackSize=0x1900;
	unsigned char IgnoreDensity=0,SingleDensity=0,HeaderSize=0;
	unsigned long BytesWritten=0,FileSize=0;

	hr = AG_OpenFile(FileName, "w+");

	if (hr == NULL) 
		return(1); //Failed to create File

	switch (Type)
	{
		case DMK:
			HeaderBuffer[0]=0;
			HeaderBuffer[1]=TrackTable[Tracks];
			HeaderBuffer[2]=(TrackSize & 0xFF);
			HeaderBuffer[3]=(TrackSize >>8);
			HeaderBuffer[4]=(IgnoreDensity<<7) | (SingleDensity<<6) | ((!DblSided)<<4);
			HeaderBuffer[0xC]=0;
			HeaderBuffer[0xD]=0;
			HeaderBuffer[0xE]=0;
			HeaderBuffer[0xF]=0;
			HeaderSize=0x10;
			FileSize= HeaderSize + (HeaderBuffer[1] * TrackSize * (DblSided+1) );
		break;

		case JVC:	// has variable header size
			HeaderSize=0;
			HeaderBuffer[0]=18;			//18 Sectors
			HeaderBuffer[1]=DblSided+1;	// Double or single Sided;
			FileSize = (HeaderBuffer[0] * 0x100 * TrackTable[Tracks] * (DblSided+1));
			if (DblSided)
			{
				FileSize+=2;
				HeaderSize=2;
			}
		break;

		case VDK:	
			HeaderBuffer[9]=DblSided+1;
			HeaderSize=12;
			FileSize = ( 18 * 0x100 * TrackTable[Tracks] * (DblSided+1));
			FileSize+=HeaderSize;
		break;

		case 3:
			HeaderBuffer[0]=0;
			HeaderBuffer[1]=TrackTable[Tracks];
			HeaderBuffer[2]=(TrackSize & 0xFF);
			HeaderBuffer[3]=(TrackSize >>8);
			HeaderBuffer[4]=((!DblSided)<<4);
			HeaderBuffer[0xC]=0x12;
			HeaderBuffer[0xD]=0x34;
			HeaderBuffer[0xE]=0x56;
			HeaderBuffer[0xF]=0x78;
			HeaderSize=0x10;
			FileSize=1;
		break;

	}

	AG_Seek(hr, 0, AG_SEEK_SET);
	AG_WriteP(hr, HeaderBuffer, HeaderSize, &BytesWritten);
	AG_Seek(hr, FileSize-1, AG_SEEK_SET);
	AG_WriteP(hr, HeaderBuffer, HeaderSize, &BytesWritten);
	AG_WriteP(hr, &Dummy, 1, &BytesWritten);
	AG_CloseFile(hr);

	return(0);
}

void LoadConfig(void)
{
	char *ModName = moduleName;
	unsigned char Index=0;
	char Temp[16] = "";
	char DiskRomPath[MAX_PATH], RGBRomPath[MAX_PATH];
	char DiskName[MAX_PATH] = "";
	unsigned int RetVal=0;

	SelectRom = GetPrivateProfileInt(ModName, "DiskRom", 1, IniFile);  //0 External 1=TRSDOS 2=RGB Dos
	TempSelectRom = SelectRom;
	GetPrivateProfileString(ModName, "RomPath", "", RomFileName, MAX_PATH, IniFile);
	AG_Strlcpy(TempRomFileName, RomFileName, sizeof(TempFileName));
	CheckPath(RomFileName);
	LoadExtRom(External, RomFileName);
	getcwd(DiskRomPath, sizeof(DiskRomPath));
	strcpy(RGBRomPath, DiskRomPath);
	strcat(DiskRomPath, "/disk11.rom");
	strcat(RGBRomPath, "/rgbdos.rom");
	LoadExtRom(TandyDisk, DiskRomPath);
	LoadExtRom(RGBDisk, RGBRomPath);

	PersistDisks = GetPrivateProfileInt(ModName, "Persist", 1, IniFile);  

	if (PersistDisks)
	{
		for (Index=0;Index<4;Index++)
		{
			sprintf(Temp,"Disk#%i",Index);
			GetPrivateProfileString(ModName,Temp,"",DiskName,MAX_PATH,IniFile);
			if (strlen(DiskName))
			{
				RetVal=mount_disk_image(DiskName,Index);
				//MessageBox(0, "Disk load attempt", "OK", 0);
				if (RetVal)
				{
					if ( (!strcmp(DiskName,"*Floppy A:")) )	//RealDisks
						PhysicalDriveA=Index+1;
					if ( (!strcmp(DiskName,"*Floppy B:")) )
						PhysicalDriveB=Index+1;
				}

				UpdateMenu(Index);
			}
		}
	}

	ClockEnabled = GetPrivateProfileInt(ModName, "ClkEnable", 1, IniFile);
	TurboMode =  GetPrivateProfileInt(ModName, "TurboDisk", 1, IniFile);
	SetTurboDisk(TurboMode);
	return;
}

void SaveConfig(void)
{
	unsigned char Index=0;
	char *ModName = moduleName;
	char Temp[16]="";

	ValidatePath(RomFileName);
	WritePrivateProfileInt(ModName, "DiskRom", SelectRom, IniFile);
	WritePrivateProfileString(ModName,"RomPath", RomFileName, IniFile);
	WritePrivateProfileInt(ModName, "Persist", PersistDisks, IniFile);

	if (PersistDisks)
		for (Index=0 ; Index < 4 ; Index++)
		{	
			sprintf(Temp,"Disk#%i",Index);
			ValidatePath(Drive[Index].ImageName);
			WritePrivateProfileString(ModName, Temp, Drive[Index].ImageName, IniFile);
		}
	WritePrivateProfileInt(ModName, "ClkEnable", ClockEnabled, IniFile);
	WritePrivateProfileInt(ModName, "TurboDisk", SetTurboDisk(QUERY), IniFile);
	return;
}

unsigned char LoadExtRom( unsigned char RomType,char *FilePath)	//Returns 1 on if loaded
{

	FILE *rom_handle=NULL;
	unsigned short index=0;
	unsigned char RetVal=0;
	unsigned char *ThisRom[3]={ExternalRom,DiskRom,RGBDiskRom};
	
	rom_handle=fopen(FilePath,"rb");
	if (rom_handle==NULL)
		memset(ThisRom[RomType],0xFF,EXTROMSIZE);
	else
	{
		while ((feof(rom_handle)==0) & (index<EXTROMSIZE))
			ThisRom[RomType][index++]=fgetc(rom_handle);
		RetVal=1;
		fclose(rom_handle);
	}
	return(RetVal);
}