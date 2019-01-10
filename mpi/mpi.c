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
// This is an expansion module for the Vcc Emulator. It simulated the functions of the TRS-80 Multi-Pak Interface

#include <agar/core.h>
#include <agar/gui.h>
#include <SDL2/SDL.h>
#include <stdbool.h>
#include "stdio.h"
#include "mpi.h"
#include "../CoCo/profport.h"
#define BOOL bool
#include "../CoCo/fileops.h"

#define MAX_PATH 260

static char moduleName[4] = { "MPI" };

#define MAXPAX 4
static void (*AssertInt)(unsigned char,unsigned char)=NULL;
static unsigned char (*MemRead8)(unsigned short)=NULL;
static void (*MemWrite8)(unsigned char,unsigned short)=NULL;

static void (*PakSetCart)(unsigned char)=NULL;
static char ModuleNames[MAXPAX][MAX_LOADSTRING]={"Empty","Empty","Empty","Empty"};	
static char CatNumber[MAXPAX][MAX_LOADSTRING]={"","","",""};
static char SlotLabel[MAXPAX][MAX_LOADSTRING*2]={"Empty","Empty","Empty","Empty"};
//static 
static char ModulePaths[MAXPAX][MAX_PATH]={"","","",""};
static unsigned char *ExtRomPointers[MAXPAX]={NULL,NULL,NULL,NULL};
static unsigned int BankedCartOffset[MAXPAX]={0,0,0,0};
static unsigned char Temp,Temp2;
static char IniFile[MAX_PATH]="";

//**************************************************************
//Array of fuction pointer for each Slot
static void (*GetModuleNameCalls[MAXPAX])(char *, AG_MenuItem *)={NULL,NULL,NULL,NULL};
static void (*ConfigModuleCalls[MAXPAX])(unsigned char)={NULL,NULL,NULL,NULL};
static void (*HeartBeatCalls[MAXPAX])(void)={NULL,NULL,NULL,NULL};
static void (*PakPortWriteCalls[MAXPAX])(unsigned char,unsigned char)={NULL,NULL,NULL,NULL};
static unsigned char (*PakPortReadCalls[MAXPAX])(unsigned char)={NULL,NULL,NULL,NULL};
static void (*PakMemWrite8Calls[MAXPAX])(unsigned char,unsigned short)={NULL,NULL,NULL,NULL};
static unsigned char (*PakMemRead8Calls[MAXPAX])(unsigned short)={NULL,NULL,NULL,NULL};
static void (*ModuleStatusCalls[MAXPAX])(char *)={NULL,NULL,NULL,NULL};
static unsigned short (*ModuleAudioSampleCalls[MAXPAX])(void)={NULL,NULL,NULL,NULL};
static void (*ModuleResetCalls[MAXPAX]) (void)={NULL,NULL,NULL,NULL};
//Set callbacks for the DLL to call
static void (*SetInteruptCallPointerCalls[MAXPAX]) ( ASSERTINTERUPT)={NULL,NULL,NULL,NULL};
static void (*DmaMemPointerCalls[MAXPAX]) (MEMREAD8,MEMWRITE8)={NULL,NULL,NULL,NULL};


void SetCartSlot0(unsigned char);
void SetCartSlot1(unsigned char);
void SetCartSlot2(unsigned char);
void SetCartSlot3(unsigned char);

static unsigned char CartForSlot[MAXPAX]={0,0,0,0};
static void (*SetCarts[MAXPAX])(unsigned char)={SetCartSlot0,SetCartSlot1,SetCartSlot2,SetCartSlot3};
static void (*SetCartCalls[MAXPAX])(SETCART)={NULL,NULL,NULL,NULL};
static void (*SetIniPathCalls[MAXPAX]) (char *)={NULL,NULL,NULL,NULL};
//***************************************************************
static void *hinstLib[4]={NULL,NULL,NULL,NULL};
static unsigned char ChipSelectSlot=3,SpareSelectSlot=3,SwitchSlot=3,SlotRegister=255;
static unsigned char slotMin = 1, slotMax = 4, slotVal = 4;
static 	char moduleParams[1024];

//Function Prototypes for this module

unsigned char MountModule(unsigned char,char *);
void UnloadModule(unsigned char);
void LoadCartDLL(unsigned char,char *);
void LoadConfig(void);
void WriteConfig(void);
void ReadModuleParms(unsigned char,char *);
int FileID(char *);
void UpdateMenu(unsigned char);
void BuildMenu(void);
void UpdateConfig(unsigned char);

AG_MenuItem *menuAnchor = NULL;
AG_MenuItem *itemMenu[MAXPAX] = { NULL,NULL,NULL,NULL };
AG_MenuItem *itemEjectSlot[MAXPAX] = { NULL,NULL,NULL,NULL };
AG_MenuItem *itemLoadSlot[MAXPAX] = { NULL,NULL,NULL,NULL };
AG_MenuItem *itemSeperator = NULL;
AG_MenuItem *itemConfig = NULL;

void __attribute__ ((constructor)) initLibrary(void) {
 //
 // Function that is called when the library is loaded
 //
 //   printf("MPI is initialized\n"); 
}

void __attribute__ ((destructor)) cleanUpLibrary(void) {
 //
 // Function that is called when the library is »closed«.
 //
 //   printf("MPI is exited\n"); 
}

void MemWrite(unsigned char Data,unsigned short Address)
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
	ModName = strcpy(ModName, moduleName);

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
		AG_MenuDel(itemConfig);
		AG_MenuDel(itemSeperator);
		for(int i = 0 ; i < MAXPAX ; i++)
		{
			AG_MenuDel(itemMenu[i]);
		}
	break;

	default:
		break;
	}

	return;
}

// This captures the Function transfer point for the CPU assert interupt 
void ADDCALL AssertInterupt(ASSERTINTERUPT Dummy)
{
	AssertInt=Dummy;
	for (Temp=0;Temp<4;Temp++)
	{
		if(SetInteruptCallPointerCalls[Temp] !=NULL)
			SetInteruptCallPointerCalls[Temp](AssertInt);
	}
	return;
}

void ADDCALL PackPortWrite(unsigned char Port,unsigned char Data)
{
	if (Port == 0x7F) //Addressing the Multi-Pak
	{ 
		SpareSelectSlot= (Data & 3);
		ChipSelectSlot= ( (Data & 0x30)>>4);
		SlotRegister=Data;
		PakSetCart(0);
		if (CartForSlot[SpareSelectSlot]==1)
			PakSetCart(1);
		return;
	}
//		if ( (Port>=0x40) & (Port<=0x5F))
//		{
//			BankedCartOffset[SpareSelectSlot]=(Data & 15)<<14;
//			if ( PakPortWriteCalls[SpareSelectSlot] != NULL)
//				PakPortWriteCalls[SpareSelectSlot](Port,Data);
//		}
//		else
		for(unsigned char Temp=0;Temp<4;Temp++)
			if (PakPortWriteCalls[Temp] != NULL)
				PakPortWriteCalls[Temp](Port,Data);
	return;
}

unsigned char ADDCALL PackPortRead(unsigned char Port)
{
	if (Port == 0x7F)
	{	
		SlotRegister&=0xCC;
		SlotRegister|=(SpareSelectSlot | (ChipSelectSlot<<4));
		return(SlotRegister);
	}

//		if ( (Port>=0x40) & (Port<=0x5F))
//		{
//			if ( PakPortReadCalls[SpareSelectSlot] != NULL)
//				return(PakPortReadCalls[SpareSelectSlot](Port));
//			else
//				return(NULL);
//		}
	Temp2=0;
	for (Temp=0;Temp<4;Temp++)
	{
		if ( PakPortReadCalls[Temp] !=NULL)
		{
			Temp2=PakPortReadCalls[Temp](Port); //Find a Module that return a value 
			if (Temp2!= 0)
				return(Temp2);
		}
	}
	return(0);
}

void ADDCALL HeartBeat(void)
{
	for (Temp=0;Temp<4;Temp++)
		if (HeartBeatCalls[Temp] != NULL)
			HeartBeatCalls[Temp]();
	return;
}

//This captures the pointers to the MemRead8 and MemWrite8 functions. This allows the DLL to do DMA xfers with CPU ram.
void ADDCALL MemPointers(MEMREAD8 Temp1,MEMWRITE8 Temp2)
{
	MemRead8=Temp1;
	MemWrite8=Temp2;
	return;
}

unsigned char ADDCALL PakMemRead8(unsigned short Address)
{
	if (ExtRomPointers[ChipSelectSlot] != NULL)
		return(ExtRomPointers[ChipSelectSlot][(Address & 32767)+BankedCartOffset[ChipSelectSlot]]); //Bank Select ???
	if (PakMemRead8Calls[ChipSelectSlot] != NULL)
		return(PakMemRead8Calls[ChipSelectSlot](Address));
	return(NULL);
}

void ADDCALL PakMemWrite8(unsigned char Data,unsigned short Address)
{
	return;
}

void ADDCALL ModuleStatus(char *MyStatus)
{
	char TempStatus[64]="";
	sprintf(MyStatus,"MPI:%i,%i",ChipSelectSlot,SpareSelectSlot);
	for (Temp=0;Temp<4;Temp++)
	{
		strcpy(TempStatus,"");
		if (ModuleStatusCalls[Temp] != NULL)
		{
			ModuleStatusCalls[Temp](TempStatus);
			strcat(MyStatus,"|");
			strcat(MyStatus,TempStatus);
		}
	}
	return ;
}

unsigned short ADDCALL ModuleAudioSample(void)
{
	unsigned short TempSample=0;
	for (Temp=0;Temp<4;Temp++)
		if (ModuleAudioSampleCalls[Temp] != NULL)
			TempSample+=ModuleAudioSampleCalls[Temp]();
		
	return(TempSample) ;
}

unsigned char ADDCALL ModuleReset(void)
{
	ChipSelectSlot=SwitchSlot;	
	SpareSelectSlot=SwitchSlot;	
	for (Temp=0;Temp<4;Temp++)
	{
		BankedCartOffset[Temp]=0; //Do I need to keep independant selects?
		
		if (ModuleResetCalls[Temp] !=NULL)
			ModuleResetCalls[Temp]();

		ModuleResetCalls[Temp] = NULL;
	}
	PakSetCart(0);
	if (CartForSlot[SpareSelectSlot]==1)
		PakSetCart(1);
	return(NULL);
}

void ADDCALL SetIniPath(char *IniFilePath)
{
	strcpy(IniFile,IniFilePath);
	LoadConfig();
	return;
}

void CPUAssertInterupt(unsigned char Interupt,unsigned char Latencey)
{
	AssertInt(Interupt,Latencey);
	return;
}

void ADDCALL SetCart(SETCART Pointer)
{
	
	PakSetCart=Pointer;
	return;
}

void OKCallback(AG_Event *event)
{
	WriteConfig();
	AG_CloseFocusedWindow();
}

void SlotChange(AG_Event *event)
{
	SwitchSlot = slotVal-1;
	ReadModuleParms(SwitchSlot, moduleParams);
}

void ConfigMPI(AG_Event *event)
{
	AG_Window *win;

    if ((win = AG_WindowNewNamedS(0, "MPI Config")) == NULL)
    {
        return;
    }

    AG_WindowSetGeometryAligned(win, AG_WINDOW_ALIGNMENT_NONE, 440, 294);
    AG_WindowSetCaptionS(win, "MPI Config");
    AG_WindowSetCloseAction(win, AG_WINDOW_DETACH);

    AG_Box *hbox, *vbox, *hbox2, *vbox2;

	hbox = AG_BoxNewHoriz(win, AG_BOX_EXPAND | AG_BOX_FRAME);
	vbox = AG_BoxNewVert(hbox, 0);

	// The four slot labels
	
	int dev;
	char slotName[8];

	for (dev = MAXPAX ; dev > 0 ; dev--)
	{	
		sprintf(slotName, "Slot %i", dev);
		vbox2 = AG_BoxNewVert(vbox, AG_BOX_FRAME);
		AG_LabelNew(vbox2, 0, slotName);
		AG_Label *lbl = AG_LabelNewPolled(vbox2, AG_LABEL_EXPAND | AG_LABEL_FRAME, "%s", SlotLabel[dev-1]);
		AG_LabelSizeHint(lbl, 1, "Great big module names goes here!\n");
	}

	// Some info

	vbox = AG_BoxNewVert(hbox, 0);
	
	ReadModuleParms(SwitchSlot, moduleParams);
	AG_Label *lbl = AG_LabelNewPolled(vbox, AG_LABEL_FRAME | AG_LABEL_EXPAND, "%s", moduleParams);
		AG_LabelSizeHint(lbl, 12, "Great big module names goes here!\n"
		"Great big module names goes here!\n"
		"Great big module names goes here!\n"
		"Great big module names goes here!\n"
		"Great big module names goes here!\n"
		"Great big module names goes here!\n"
		"Great big module names goes here!\n"
		"Great big module names goes here!\n"
		"Great big module names goes here!\n"
		"Great big module names goes here!\n"
		"Great big module names goes here!\n"
		"Great big module names goes here!\n");

	// Slot Select

	hbox2 = AG_BoxNewHoriz(vbox, 0);
	vbox2 = AG_BoxNewVert(hbox2, AG_BOX_FRAME);	
	AG_LabelNew(vbox2, 0, "Slot Select");
	AG_LabelNew(vbox2, 0, "1    2    3    4");

	AG_Slider *sl = AG_SliderNew(vbox2, AG_SLIDER_HORIZ, AG_SLIDER_EXCL | AG_SLIDER_HFILL);
	AG_BindUint8(sl, "value", &slotVal);
	AG_BindUint8(sl, "min", &slotMin);
	AG_BindUint8(sl, "max", &slotMax);
	AG_SetInt(sl, "inc", 1);
	slotVal = SwitchSlot+1;
    AG_SetEvent(sl, "slider-changed", SlotChange, NULL);

	// Ok button

	AG_ButtonNewFn(hbox2, 0, "OK", OKCallback, NULL);

	AG_WindowShow(win);
}

unsigned char MountModule(unsigned char Slot,char *ModName)
{
	unsigned char ModuleType=0;
	char ModuleName[MAX_PATH]="";
	unsigned int index=0;
	strcpy(ModuleName,ModName);
	FILE *rom_handle;
	if (Slot>3)
		return(0);
	ModuleType=FileID(ModuleName);
	switch (ModuleType)
	{
	case 0: //File doesn't exist
		return(0);
	break;

	case 2: //ROM image
		UnloadModule(Slot);
		ExtRomPointers[Slot]=(unsigned char *)malloc(0x40000);
		if (ExtRomPointers[Slot]==NULL)
		{
			AG_TextMsg(AG_MSG_INFO, "Rom pointer is NULL");
			return(0); //Can Allocate RAM
		}
		rom_handle=fopen(ModuleName,"rb");
		if (rom_handle==NULL)
		{
			AG_TextMsg(AG_MSG_INFO, "File handle is NULL");
			return(0);
		}
		while ((feof(rom_handle)==0) & (index<0x40000))
			ExtRomPointers[Slot][index++]=fgetc(rom_handle);
		fclose(rom_handle);
		strcpy(ModulePaths[Slot],ModuleName);
		PathStripPath(ModuleName);
//		PathRemovePath(ModuleName);
		PathRemoveExtension(ModuleName);
		strcpy(ModuleNames[Slot],ModuleName);
		strcpy(SlotLabel[Slot],ModuleName); //JF
		UpdateMenu(Slot);
		UpdateConfig(Slot);
		CartForSlot[Slot]=1;
		return(1);
	break;

	case 1:	//DLL File
		UnloadModule(Slot);
		strcpy(ModulePaths[Slot],ModuleName);
		hinstLib[Slot] = SDL_LoadObject(ModuleName);
		if (hinstLib[Slot] == NULL)
			return(0);	//Error Can't open File

		GetModuleNameCalls[Slot]=(GETNAME)SDL_LoadFunction(hinstLib[Slot], "ModuleName"); 
		ConfigModuleCalls[Slot]=(CONFIGIT)SDL_LoadFunction(hinstLib[Slot], "ModuleConfig");
		PakPortWriteCalls[Slot]=(PACKPORTWRITE) SDL_LoadFunction(hinstLib[Slot], "PackPortWrite");
		PakPortReadCalls[Slot]=(PACKPORTREAD) SDL_LoadFunction(hinstLib[Slot], "PackPortRead");
		SetInteruptCallPointerCalls[Slot]=(SETINTERUPTCALLPOINTER)SDL_LoadFunction(hinstLib[Slot], "AssertInterupt");

		DmaMemPointerCalls[Slot]=(DMAMEMPOINTERS) SDL_LoadFunction(hinstLib[Slot], "MemPointers");
		SetCartCalls[Slot]=(SETCARTPOINTER) SDL_LoadFunction(hinstLib[Slot], "SetCart"); //HERE
		
		HeartBeatCalls[Slot]=(HEARTBEAT) SDL_LoadFunction(hinstLib[Slot], "HeartBeat");
		PakMemWrite8Calls[Slot]=(MEMWRITE8) SDL_LoadFunction(hinstLib[Slot], "PakMemWrite8");
		PakMemRead8Calls[Slot]=(MEMREAD8) SDL_LoadFunction(hinstLib[Slot], "PakMemRead8");
		ModuleStatusCalls[Slot]=(MODULESTATUS) SDL_LoadFunction(hinstLib[Slot], "ModuleStatus");
		ModuleAudioSampleCalls[Slot]=(MODULEAUDIOSAMPLE) SDL_LoadFunction(hinstLib[Slot], "ModuleAudioSample");
		ModuleResetCalls[Slot]=(MODULERESET) SDL_LoadFunction(hinstLib[Slot], "ModuleReset");
		SetIniPathCalls[Slot]=(SETINIPATH) SDL_LoadFunction(hinstLib[Slot], "SetIniPath");

		if (GetModuleNameCalls[Slot] == NULL)
		{
			UnloadModule(Slot);
			AG_TextMsg(AG_MSG_INFO, "Not a valid Module");
			return(0); //Error Not a Vcc Module 
		}
		GetModuleNameCalls[Slot](ModuleNames[Slot], menuAnchor); //Need to add address of local Dynamic menu callback function!
		strcpy(SlotLabel[Slot],ModuleNames[Slot]);
		strcat(SlotLabel[Slot],"  ");
		strcat(SlotLabel[Slot],CatNumber[Slot]);
		UpdateMenu(Slot);
		UpdateConfig(Slot);

		if (SetInteruptCallPointerCalls[Slot] !=NULL)
			SetInteruptCallPointerCalls[Slot](AssertInt);
		if (DmaMemPointerCalls[Slot] !=NULL)
			DmaMemPointerCalls[Slot](MemRead8,MemWrite8);
		if (SetIniPathCalls[Slot] != NULL)
			SetIniPathCalls[Slot](IniFile);
		if (SetCartCalls[Slot] !=NULL)
			SetCartCalls[Slot](*SetCarts[Slot]);	//Transfer the address of the SetCart routin to the pak
													//For the multpak there is 1 for each slot se we know where it came from
		if (ModuleResetCalls[Slot]!=NULL)
			ModuleResetCalls[Slot]();
		return(1);
	break;
	}
	return(0);
}

void UnloadModule(unsigned char Slot)
{
	if (ConfigModuleCalls[Slot] != NULL)
	{
		ConfigModuleCalls[Slot](0);
	}
	GetModuleNameCalls[Slot]=NULL;
	ConfigModuleCalls[Slot]=NULL;
	PakPortWriteCalls[Slot]=NULL;
	PakPortReadCalls[Slot]=NULL;
	SetInteruptCallPointerCalls[Slot]=NULL;
	DmaMemPointerCalls[Slot]=NULL;
	HeartBeatCalls[Slot]=NULL;
	PakMemWrite8Calls[Slot]=NULL;
	PakMemRead8Calls[Slot]=NULL;
	ModuleStatusCalls[Slot]=NULL;
	ModuleAudioSampleCalls[Slot]=NULL;
	ModuleResetCalls[Slot]=NULL;
	SetIniPathCalls[Slot]=NULL;
	strcpy(ModulePaths[Slot],"");
	strcpy(ModuleNames[Slot],"Empty");
	strcpy(CatNumber[Slot],"");
	strcpy(SlotLabel[Slot],"Empty");
	if (hinstLib[Slot] !=NULL)
		SDL_UnloadObject(hinstLib[Slot]); 
	if (ExtRomPointers[Slot] !=NULL)
		free(ExtRomPointers[Slot]);
	hinstLib[Slot]=NULL;
	ExtRomPointers[Slot]=NULL;
	CartForSlot[Slot]=0;
	UpdateMenu(Slot);
	UpdateConfig(Slot);
	return;
}

void LoadConfig(void)
{
	char ModName[MAX_LOADSTRING]="";
	//LoadString(g_hinstDLL,IDS_MODULE_NAME,ModName, MAX_LOADSTRING);
	strncpy(ModName, moduleName, sizeof(ModName));
	SwitchSlot=GetPrivateProfileInt(ModName,"SWPOSITION",3,IniFile);
	ChipSelectSlot=SwitchSlot;
	SpareSelectSlot=SwitchSlot;
	GetPrivateProfileString(ModName,"SLOT1","",ModulePaths[0],MAX_PATH,IniFile);
	CheckPath(ModulePaths[0]);
	GetPrivateProfileString(ModName,"SLOT2","",ModulePaths[1],MAX_PATH,IniFile);
	CheckPath(ModulePaths[1]);
	GetPrivateProfileString(ModName,"SLOT3","",ModulePaths[2],MAX_PATH,IniFile);
	CheckPath(ModulePaths[2]);
	GetPrivateProfileString(ModName,"SLOT4","",ModulePaths[3],MAX_PATH,IniFile);
	CheckPath(ModulePaths[3]);
	BuildMenu();
	for (Temp=0;Temp<4;Temp++)
		if (strlen(ModulePaths[Temp]) !=0)
			MountModule(Temp, ModulePaths[Temp]);
	return;
}

void WriteConfig(void)
{
	WritePrivateProfileInt(moduleName,"SWPOSITION",SwitchSlot,IniFile);
	ValidatePath(ModulePaths[0]);
	WritePrivateProfileString(moduleName,"SLOT1",ModulePaths[0],IniFile);
	ValidatePath(ModulePaths[1]);
	WritePrivateProfileString(moduleName,"SLOT2",ModulePaths[1],IniFile);
	ValidatePath(ModulePaths[2]);
	WritePrivateProfileString(moduleName,"SLOT3",ModulePaths[2],IniFile);
	ValidatePath(ModulePaths[3]);
	WritePrivateProfileString(moduleName,"SLOT4",ModulePaths[3],IniFile);
	return;
}

void UpdateConfig(unsigned char slot)
{
	char slotname[6] = "SLOT";
	sprintf(slotname, "SLOT%1i", slot+1);
	ValidatePath(ModulePaths[slot]);
	WritePrivateProfileString(moduleName, slotname, ModulePaths[slot],IniFile);

}

void ReadModuleParms(unsigned char Slot,char *String)
{
	strcpy(String,"Module Name: ");
	strcat(String,ModuleNames[Slot]);

	strcat(String,"\n-----------------------------------------\n");

	if (ConfigModuleCalls[Slot]!=NULL)
		strcat(String,"Has Configurable options\n");

	if (SetIniPathCalls[Slot]!=NULL)
		strcat(String,"Saves Config Info\n");

	if (PakPortWriteCalls[Slot]!=NULL)
		strcat(String,"Is IO writable\n");

	if (PakPortReadCalls[Slot]!=NULL)
		strcat(String,"Is IO readable\n");

	if (SetInteruptCallPointerCalls[Slot]!=NULL)
		strcat(String,"Generates Interupts\n");

	if (DmaMemPointerCalls[Slot]!=NULL)
		strcat(String,"Generates DMA Requests\n");

	if (HeartBeatCalls[Slot]!=NULL)
		strcat(String,"Needs Heartbeat\n");

	if (ModuleAudioSampleCalls[Slot]!=NULL)
		strcat(String,"Analog Audio Outputs\n");

	if (PakMemWrite8Calls[Slot]!=NULL)
		strcat(String,"Needs CS Write\n");

	if (PakMemRead8Calls[Slot]!=NULL)
		strcat(String,"Needs CS Read (onboard ROM)\n");

	if (ModuleStatusCalls[Slot]!=NULL)
		strcat(String,"Returns Status\n");

	if (ModuleResetCalls[Slot]!=NULL)
		strcat(String,"Needs Reset Notification\n");
	return;
}

int FileID(char *Filename)
{
	FILE *DummyHandle=NULL;
	char elf[5] = { 0x7f, 'E', 'L', 'F', 0 };
	char pe[4] = { 'M', 'Z', 0220, 0 };
	char *match = NULL;
	char Temp[5]="";
	char *Platform = SDL_GetPlatform();

	if (strcmp(Platform, "Linux") == 0)
	{
		match = elf;
	}
	else if (strcmp(Platform, "Windows") == 0)
	{
		match = pe;
	}

	if (match == NULL)
	{
		return(2);
	}

	DummyHandle=fopen(Filename,"rb");

	if (DummyHandle==NULL)
	{
		return(0);	//File Doesn't exist
	}

	Temp[0] = fgetc(DummyHandle);
	Temp[1] = fgetc(DummyHandle);
	Temp[2] = fgetc(DummyHandle);
	Temp[3] = fgetc(DummyHandle);
	Temp[4] = 0;

	fclose(DummyHandle);

	if (strcmp(Temp, match) ==0 )
	{
		return(1);	//DLL File
	}

	return(2);		//Rom Image 
}

void SetCartSlot0(unsigned char Tmp)
{
	CartForSlot[0]=Tmp;
	return;
}

void SetCartSlot1(unsigned char Tmp)
{
	CartForSlot[1]=Tmp;
	return;
}
void SetCartSlot2(unsigned char Tmp)
{
	CartForSlot[2]=Tmp;
	return;
}
void SetCartSlot3(unsigned char Tmp)
{
	CartForSlot[3]=Tmp;
	return;
}

void UpdateMenu(unsigned char slot)
{
	char slotname[MAX_PATH];

	AG_Strlcpy(slotname, SlotLabel[slot], sizeof(slotname));
	PathStripPath(slotname);

	AG_MenuSetLabel(itemEjectSlot[slot], "Eject : %s", slotname);
}

int LoadSlot(AG_Event *event)
{
	int slot = AG_INT(1);
	char *file = AG_STRING(2);
	AG_FileType *ft = AG_PTR(3);
	char TempFileName[MAX_PATH];

	AG_Strlcpy(TempFileName, file, sizeof(TempFileName));

	if (AG_FileExists(TempFileName)) 
	{
		MountModule(slot, TempFileName);
		UpdateMenu(slot);
	}
}

void BrowseSlot(AG_Event *event)
{
	int slot = AG_INT(1);
	
    AG_Window *fdw = AG_WindowNew(AG_WINDOW_DIALOG);
    AG_WindowSetCaption(fdw, "Program Paks");
    AG_WindowSetGeometryAligned(fdw, AG_WINDOW_ALIGNMENT_NONE, 500, 500);
    AG_WindowSetCloseAction(fdw, AG_WINDOW_DETACH);

    AG_FileDlg *fd = AG_FileDlgNew(fdw, AG_FILEDLG_EXPAND | AG_FILEDLG_CLOSEWIN | AG_FILEDLG_MASK_EXT);
    AG_FileDlgSetDirectory(fd, ".");

	AG_FileDlgAddType(fd, "CoCo Pak or Cartridge", "*.rom,*.ccc,*.so,*.dll",	LoadSlot, "%i", slot);
    AG_WindowShow(fdw);
}

void UnloadSlot(AG_Event *event)
{
	int slot = AG_INT(1);

	UnloadModule(slot);
	UpdateMenu(slot);
}

void BuildMenu(void)
{
	int slot = 0;
	char slotname[128];

	if (itemConfig != NULL) return;

	itemSeperator = AG_MenuSeparator(menuAnchor);

	for (slot = 0 ; slot < MAXPAX ; slot++)
	{
		sprintf(slotname, "MPI Slot %i", slot+1);
		itemMenu[slot] = AG_MenuNode(menuAnchor, slotname, NULL);
		{
			itemLoadSlot[slot] = AG_MenuAction(itemMenu[slot], "Insert", NULL, BrowseSlot, "%i", slot);
			sprintf(slotname, "Eject : %s", SlotLabel[slot]);
			itemEjectSlot[slot] = AG_MenuAction(itemMenu[slot], slotname, NULL, UnloadSlot, "%i", slot);
		}
	}

	itemConfig = AG_MenuNode(menuAnchor, "MPI Config", NULL);
	AG_MenuAction(itemConfig, "Config", NULL, ConfigMPI, NULL);
}