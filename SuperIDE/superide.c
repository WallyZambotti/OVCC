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

#include <agar/core.h>
#include <agar/gui.h>
#include "stdio.h"
#include "defines.h"
#include "superide.h"
#include "idebus.h"
#include "cloud9.h"
#include "logger.h"
#include "../CoCo/fileops.h"
#include "../CoCo/profport.h"

#define MAX_PATH 260

static char moduleName[27] = { "Glenside-IDE /w Clock 000" };

static char FileName[MAX_PATH] = { "" };
static char IniFile[MAX_PATH] = { "" };

typedef unsigned char (*MEMREAD8)(unsigned short);
typedef void (*MEMWRITE8)(unsigned char,unsigned short);
typedef void (*ASSERTINTERUPT) (unsigned char,unsigned char);
typedef void (*DMAMEMPOINTERS) ( MEMREAD8,MEMWRITE8);
typedef void (*DYNAMICMENUCALLBACK)( char *,int, int);
static void (*AssertInt)(unsigned char,unsigned char)=NULL;
static unsigned char (*MemRead8)(unsigned short);
static void (*MemWrite8)(unsigned char,unsigned short);
static unsigned char *Memory=NULL;
static void (*DynamicMenuCallback)( char *,int, int)=NULL;
static unsigned char BaseAddress=0x50;
void Select_Disk(unsigned char);
void SaveConfig();
void LoadConfig();
unsigned char BaseTable[4]={0x40,0x50,0x60,0x70};
void BuildMenu(void);

static unsigned char BaseAddr=1,ClockEnabled=1,ClockReadOnly=1;
static unsigned char DataLatch=0;
static FILE *g_hinstDLL;

AG_MenuItem *menuAnchor = NULL;
AG_MenuItem *itemMaster = NULL;
AG_MenuItem *itemSlave = NULL;
AG_MenuItem *itemMasterEject = NULL;
AG_MenuItem *itemSlaveEject = NULL;
AG_MenuItem *itemConfig = NULL;
AG_MenuItem *itemSeperator = NULL;

AG_Checkbox *xbClockFF70 = NULL;

void __attribute__ ((constructor)) initLibrary(void) {
 //
 // Function that is called when the library is loaded
 //
 //   printf("%s is initialized\n", moduleName); 
}

void __attribute__ ((destructor)) cleanUpLibrary(void) {
 //
 // Function that is called when the library is »closed«.
 //
 //   printf("%s is exited\n", moduleName); 
}

void ADDCALL ModuleName(char *ModName, AG_MenuItem *Temp)
{

	menuAnchor = Temp;
	strcpy(ModName, moduleName);
	IdeInit();

	if (menuAnchor != NULL) 
	{
		BuildMenu();
	}

	return ;
}

void ADDCALL PackPortWrite(unsigned char Port,unsigned char Data)
{
	if ( (Port >=BaseAddress) & (Port <= (BaseAddress+8)))
		switch (Port-BaseAddress)
		{
			case 0x0:	
				IdeRegWrite(Port-BaseAddress,(DataLatch<<8)+ Data );
				break;

			case 0x8:
				DataLatch=Data & 0xFF;		//Latch
				break;

			default:
				IdeRegWrite(Port-BaseAddress,Data);
				break;
		}	//End port switch	
	return;
}

unsigned char ADDCALL PackPortRead(unsigned char Port)
{
	unsigned char RetVal=0;
	unsigned short Temp=0;

	if ( ((Port==0x78) | (Port==0x79) | (Port==0x7C)) & ClockEnabled)
		RetVal=ReadTime(Port);

	if ( (Port >=BaseAddress) & (Port <= (BaseAddress+8)))
		switch (Port-BaseAddress)
		{
			case 0x0:	
				Temp=IdeRegRead(Port-BaseAddress);

				RetVal=Temp & 0xFF;
				DataLatch= Temp>>8;
				break;

			case 0x8:
				RetVal=DataLatch;		//Latch
				break;
			default:
				RetVal=IdeRegRead(Port-BaseAddress)& 0xFF;
				break;

		}	//End port switch	
		
	return(RetVal);
}

void ADDCALL ModuleStatus(char *MyStatus)
{
	DiskStatus(MyStatus);
	return ;
}

void ADDCALL ModuleConfig(unsigned char func)
{
	switch (func)
	{
	case 0: // Destroy Menus
		AG_MenuDel(itemMaster);
		AG_MenuDel(itemSlave);
		AG_MenuDel(itemConfig);
		AG_MenuDel(itemSeperator);
		break;

	default:
		break;
	}

	return;
}

void ADDCALL SetIniPath (char *IniFilePath)
{
	strcpy(IniFile, IniFilePath);
	LoadConfig();
	return;
}

void UpdateMenu(int disk)
{
	char TempDisk[MAX_PATH] = "";

	QueryDisk(disk, TempDisk);
	PathStripPath(TempDisk);

	if (disk == MASTER)
	{
		AG_MenuSetLabel(itemMasterEject, "Eject : %s", TempDisk);
	}
	else
	{
		AG_MenuSetLabel(itemSlaveEject, "Eject : %s", TempDisk);
	}
}

int LoadHardDisk(AG_Event *event)
{
	int disk = AG_INT(1);
	char *file = AG_STRING(2);

	if (AG_FileExists(file)) 
	{
		if (!(MountDisk(file ,disk)))
		{
			AG_TextMsg(AG_MSG_INFO, "Can't open the Image specified.");
		}
	}

	SaveConfig();
	UpdateMenu(disk);
}

void BrowseHardDisk(AG_Event *event)
{
	int disk = AG_INT(1);
	
    AG_Window *fdw = AG_WindowNew(AG_WINDOW_DIALOG);
    AG_WindowSetCaption(fdw, "Insert FD Image");
    AG_WindowSetGeometryAligned(fdw, AG_WINDOW_ALIGNMENT_NONE, 500, 500);
    AG_WindowSetCloseAction(fdw, AG_WINDOW_DETACH);

    AG_FileDlg *fd = AG_FileDlgNew(fdw, AG_FILEDLG_EXPAND | AG_FILEDLG_CLOSEWIN | AG_FILEDLG_MASK_EXT);
    AG_FileDlgSetDirectory(fd, ".");

	AG_FileDlgAddType(fd, "Disk Images", "*.img,*.dsk,*.os9,*.vhd",	LoadHardDisk, "%i", disk);
    AG_WindowShow(fdw);
}

void UnloadHardDisk(AG_Event *event)
{
	int disk = AG_INT(1);
	
	DropDisk(disk);

	SaveConfig();
	UpdateMenu(disk);
}

void OKCallback(AG_Event *event)
{
	BaseAddress = BaseTable[BaseAddr & 3];
	SetClockWrite(!ClockReadOnly);
	SaveConfig();
	AG_CloseFocusedWindow();
}

void BaseComboSelected(AG_Event *event)
{
    AG_TlistItem *ti = AG_PTR(1);

    BaseAddr = ti->label;

	if (BaseAddr == 3)
	{
		ClockEnabled = 0;
		AG_WidgetDisable(xbClockFF70);
	}
	else
	{
		AG_WidgetEnable(xbClockFF70);
	}
}

void PopulateBaseAddresses(AG_Tlist *list)
{
    AG_TlistAddS(list, NULL, "40");
    AG_TlistAddS(list, NULL, "50");
    AG_TlistAddS(list, NULL, "60");
    AG_TlistAddS(list, NULL, "70");
}

void ConfigIDE(AG_Event *event)
{
	AG_Window *win;

    if ((win = AG_WindowNewNamedS(0, "Glenside IDE Config")) == NULL)
    {
        return;
    }

    AG_WindowSetGeometryAligned(win, AG_WINDOW_ALIGNMENT_NONE, 440, 294);
    AG_WindowSetCaptionS(win, "Glenside IDE Config");
    AG_WindowSetCloseAction(win, AG_WINDOW_DETACH);

    AG_Box *hbox, *vbox;

	// Base Address Combo

	hbox = AG_BoxNewHoriz(win, AG_BOX_EXPAND | AG_BOX_FRAME);
	vbox = AG_BoxNewVert(hbox, AG_BOX_EXPAND);

	AG_Combo *comBase = AG_ComboNew(vbox, AG_COMBO_HFILL, NULL);
	AG_ComboSizeHint(comBase, "40", 4);
	PopulateBaseAddresses(comBase->list);
	AG_TlistItem *item = AG_TlistFindByIndex(comBase->list, BaseAddr+1);
	if (item != NULL) AG_ComboSelect(comBase, item);
	AG_SetEvent(comBase, "combo-selected", BaseComboSelected, NULL);

	// Check Box items

	xbClockFF70 = AG_CheckboxNewFn(vbox, 0, "Clock at 0xFF70", NULL, NULL);
	AG_BindUint8(xbClockFF70, "state", &ClockEnabled);

	AG_Checkbox *xb = AG_CheckboxNewFn(vbox, 0, "Clock is Read Only", NULL, NULL);
	AG_BindUint8(xb, "state", &ClockReadOnly);

	if (BaseAddr == 3)
	{
		ClockEnabled = 0;
		AG_WidgetDisable(xbClockFF70);
	}

	// OK - Cancel Buttons

	vbox = AG_BoxNewVert(hbox, 0);

	AG_ButtonNewFn(vbox, 0, "OK", OKCallback, NULL);
	AG_ButtonNewFn(vbox, 0, "Cancel", AGWINDETACH(win));

	AG_WindowShow(win);
}

void BuildMenu(void)
{
	if (itemConfig == NULL)
	{
		itemSeperator = AG_MenuSeparator(menuAnchor);
	    itemMaster = AG_MenuNode(menuAnchor, "IDE Master", NULL);
        {
            AG_MenuAction(itemMaster, "Insert Disk", NULL, BrowseHardDisk, "%i", 0);
            itemMasterEject = AG_MenuAction(itemMaster, "Eject : ", NULL, UnloadHardDisk, "%i", 0);
		}
	    itemSlave = AG_MenuNode(menuAnchor, "IDE Slave", NULL);
        {
            AG_MenuAction(itemSlave, "Insert Disk", NULL, BrowseHardDisk, "%i", 1);
            itemSlaveEject = AG_MenuAction(itemSlave, "Eject : ", NULL, UnloadHardDisk, "%i", 1);
		}

		itemConfig = AG_MenuNode(menuAnchor, "IDE Config", NULL);
		AG_MenuAction(itemConfig, "Config", NULL, ConfigIDE, NULL);
	}
}

void SaveConfig(void)
{
	unsigned char Index=0;
	QueryDisk(MASTER, FileName);
	WritePrivateProfileString(moduleName, "Master", FileName, IniFile);
	QueryDisk(SLAVE, FileName);
	WritePrivateProfileString(moduleName, "Slave", FileName, IniFile);
	WritePrivateProfileInt(moduleName, "BaseAddr", BaseAddr, IniFile);
	WritePrivateProfileInt(moduleName, "ClkEnable", ClockEnabled, IniFile);
	WritePrivateProfileInt(moduleName, "ClkRdOnly", ClockReadOnly, IniFile);
	return;
}

void LoadConfig(void)
{
	char ModName[MAX_LOADSTRING] = "";
	unsigned char Index = 0;
	char Temp[16] = "";
	char DiskName[MAX_PATH] = "";
	unsigned int RetVal = 0;

	GetPrivateProfileString(moduleName, "Master", "", FileName, MAX_PATH, IniFile);
	MountDisk(FileName, MASTER);
	UpdateMenu(MASTER);
	GetPrivateProfileString(moduleName, "Slave", "", FileName, MAX_PATH, IniFile);
	BaseAddr = GetPrivateProfileInt(moduleName, "BaseAddr", 1, IniFile); 
	ClockEnabled = GetPrivateProfileInt(moduleName, "ClkEnable", 1, IniFile); 
	ClockReadOnly = GetPrivateProfileInt(moduleName, "ClkRdOnly", 1, IniFile); 
	BaseAddr &= 3;

	if (BaseAddr == 3)
	{
		ClockEnabled=0;
	}

	BaseAddress = BaseTable[BaseAddr];
	SetClockWrite(!ClockReadOnly);
	MountDisk(FileName ,SLAVE);
	UpdateMenu(SLAVE);
	return;
}