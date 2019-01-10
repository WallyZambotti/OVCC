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

//#include <dlfcn.h>
#include <stdio.h>
#include "defines.h"
#include "tcc1014mmu.h"
#include "pakinterface.h"
#include "config.h"
#include "vcc.h"
#include "mc6821.h"
#include "logger.h"
#include "fileops.h"
#define HASCONFIG		1
#define HASIOWRITE		2
#define HASIOREAD		4
#define NEEDSCPUIRQ		8
#define DOESDMA			16
#define NEEDHEARTBEAT	32
#define ANALOGAUDIO		64
#define CSWRITE			128
#define CSREAD			256
#define RETURNSSTATUS	512
#define CARTRESET		1024
#define SAVESINI		2048
#define ASSERTCART		4096
#define nullptr NULL

extern AG_MenuItem *GetMenuAnchor();

// Storage for Pak ROMs
static uint8_t *ExternalRomBuffer = nullptr; 
static bool RomPackLoaded = false;

extern SystemState2 EmuState2;

static unsigned int BankedCartOffset=0;
static char DllPath[256]="";
static unsigned short ModualParms=0;
static void *hinstLib; 
static bool DialogOpen=false;
typedef void (*DYNAMICMENUCALLBACK)( char *,int, int);
typedef void (*GETNAME)(char *,char *,DYNAMICMENUCALLBACK); 
typedef void (*CONFIGIT)(unsigned char); 
typedef void (*HEARTBEAT) (void);
typedef unsigned char (*PACKPORTREAD)(unsigned char);
typedef void (*PACKPORTWRITE)(unsigned char,unsigned char);
typedef void (*ASSERTINTERUPT) (unsigned char,unsigned char);
typedef unsigned char (*MEMREAD8)(unsigned short);
typedef void (*SETCART)(unsigned char);
typedef void (*MEMWRITE8)(unsigned char,unsigned short);
typedef void (*MODULESTATUS)(char *);
typedef void (*DMAMEMPOINTERS) ( MEMREAD8,MEMWRITE8);
typedef void (*SETCARTPOINTER)(SETCART);
typedef void (*SETINTERUPTCALLPOINTER) (ASSERTINTERUPT);
typedef unsigned short (*MODULEAUDIOSAMPLE)(void);
typedef void (*MODULERESET)(void);
typedef void (*SETINIPATH)(char *);

static void (*GetModuleName)(char *, AG_MenuItem *)=NULL;
static void (*ConfigModule)(unsigned char)=NULL;
static void (*SetInteruptCallPointer) ( ASSERTINTERUPT)=NULL;
static void (*DmaMemPointer) (MEMREAD8,MEMWRITE8)=NULL;
static void (*HeartBeat)(void)=NULL;
static void (*PakPortWrite)(unsigned char,unsigned char)=NULL;
static unsigned char (*PakPortRead)(unsigned char)=NULL;
static void (*PakMemWrite8)(unsigned char,unsigned short)=NULL;
static unsigned char (*PakMemRead8)(unsigned short)=NULL;
static void (*ModuleStatus)(char *)=NULL;
static unsigned short (*ModuleAudioSample)(void)=NULL;
static void (*ModuleReset) (void)=NULL;
static void (*SetIniPath) (char *)=NULL;
static void (*PakSetCart)(SETCART)=NULL;


static char Did=0;
int FileID(char *);
typedef struct {
	char MenuName[512];
	int MenuId;
	int Type;
} Dmenu;

static Dmenu MenuItem[100];
static unsigned char MenuIndex=0;
// static HMENU hMenu = NULL;
// static HMENU hSubMenu[64] ;


static 	char Modname[MAX_PATH]="Blank";

void PakTimer(void)
{
	if (HeartBeat != NULL)
		HeartBeat();
	return;
}

void ResetBus(void)
{
	BankedCartOffset=0;
	if (ModuleReset !=NULL)
		ModuleReset();
	return;
}

void GetModuleStatus(SystemState2 *SMState)
{
	if (ModuleStatus!=NULL)
		ModuleStatus(SMState->StatusLine);
	else
		sprintf(SMState->StatusLine,"");
	return;
}

unsigned char PackPortRead (unsigned char port)
{
	if (PakPortRead != NULL)
		return(PakPortRead(port));
	else
		return(NULL);
}

void PackPortWrite(unsigned char Port,unsigned char Data)
{
	if (PakPortWrite != NULL)
	{
		PakPortWrite(Port,Data);
		return;
	}
	
	if ((Port == 0x40) && (RomPackLoaded == true)) {
		BankedCartOffset = (Data & 15) << 14;
	}

	return;
}

unsigned char PackMem8Read (unsigned short Address)
{
	if (PakMemRead8!=NULL)
		return(PakMemRead8(Address&32767));
	if (ExternalRomBuffer!=NULL)
		return(ExternalRomBuffer[(Address & 32767)+BankedCartOffset]);
	return(0);
}

void PackMem8Write(unsigned char Port,unsigned char Data)
{
	return;
}

unsigned short PackAudioSample(void)
{
	if (ModuleAudioSample !=NULL)
		return(ModuleAudioSample());
	return(NULL);
}

int InsertModule (char *ModulePath)
{
//	char Modname[MAX_LOADSTRING]="Blank";
	char CatNumber[MAX_LOADSTRING]="";
	char Temp[MAX_LOADSTRING]="";
	char String[1024]="";
	char TempIni[MAX_PATH]="";
	unsigned char FileType=0;
	FileType=FileID(ModulePath);

	switch (FileType)
	{
	case 0:		//File doesn't exist
		return(NOMODULE);
		break;

	case 2:		//File is a ROM image
		UnloadDll(1);
		load_ext_rom(ModulePath);
		strncpy(Modname,ModulePath,MAX_PATH);
		PathStripPath(Modname);
		PathRemoveExtension(Modname);
		UpdateCartridgeMenu(Modname); //Refresh Menus
		UpdateOnBoot(Modname);
		EmuState2.ResetPending=2;
		SetCart(1);
		return(0);
	break;

	case 1:		//File is a shared library
		UnloadDll(1);
		strcpy(Modname, "");
		ValidatePath(ModulePath);
		if (ModulePath[0] != 0 && (ModulePath[0] != '/' || ModulePath[1] != ':'))
		{
			strncpy(Modname, "./", MAX_PATH);
		}
		strncat(Modname, ModulePath, MAX_PATH);
		//PathStripPath(Modname);
		//void *mylib = dlopen(Modname, RTLD_NOW);
		//char *error = dlerror();
		hinstLib = SDL_LoadObject(Modname);
		if (hinstLib ==NULL)
			return(NOMODULE);
		strncpy(Modname, ModulePath, MAX_PATH);
		//fprintf(stderr, "Insert Module : Found module\n");
		SetCart(0);
		GetModuleName = SDL_LoadFunction(hinstLib, "ModuleName");
		ConfigModule = SDL_LoadFunction(hinstLib, "ModuleConfig");
		PakPortWrite = SDL_LoadFunction(hinstLib, "PackPortWrite");
		PakPortRead = SDL_LoadFunction(hinstLib, "PackPortRead");
		SetInteruptCallPointer = SDL_LoadFunction(hinstLib, "AssertInterupt");
		DmaMemPointer = SDL_LoadFunction(hinstLib, "MemPointers");
		HeartBeat = SDL_LoadFunction(hinstLib, "HeartBeat");
		PakMemWrite8 = SDL_LoadFunction(hinstLib, "PakMemWrite8");
		PakMemRead8 = SDL_LoadFunction(hinstLib, "PakMemRead8");
		ModuleStatus = SDL_LoadFunction(hinstLib, "ModuleStatus");
		ModuleAudioSample = SDL_LoadFunction(hinstLib, "ModuleAudioSample");
		ModuleReset = SDL_LoadFunction(hinstLib, "ModuleReset");
		SetIniPath = SDL_LoadFunction(hinstLib, "SetIniPath");
		PakSetCart = SDL_LoadFunction(hinstLib, "SetCart");
		if (GetModuleName == NULL)
		{
			AG_UnloadDSO(hinstLib); 
			hinstLib=NULL;
			return(NOTVCC);
		}
		BankedCartOffset=0;
		if (DmaMemPointer != NULL)
			DmaMemPointer(MemRead8, MemWrite8);
		if (SetInteruptCallPointer != NULL)
			SetInteruptCallPointer(CPUAssertInterupt);

		//fprintf(stderr, "Insert Module : Calling GetModuleName %lx\n", (unsigned long)GetModuleName);
		UpdateOnBoot(Modname);
		GetModuleName(Modname, GetMenuAnchor());  //Instanciate the menus from HERE!
		UpdateCartridgeMenu(Modname); //Refresh Menus
		sprintf(Temp,"Configure %s",Modname);

		strcat(String,"Module Name: ");
		strcat(String,Modname);
		strcat(String,"\n");
		if (ConfigModule!=NULL)
		{
			ModualParms|=1;
			strcat(String,"Has Configurable options\n");
		}
		if (PakPortWrite!=NULL)
		{
			ModualParms|=2;
			strcat(String,"Is IO writable\n");
		}
		if (PakPortRead!=NULL)
		{
			ModualParms|=4;
			strcat(String,"Is IO readable\n");
		}
		if (SetInteruptCallPointer!=NULL)
		{
			ModualParms|=8;
			strcat(String,"Generates Interupts\n");
		}
		if (DmaMemPointer!=NULL)
		{
			ModualParms|=16;
			strcat(String,"Generates DMA Requests\n");
		}
		if (HeartBeat!=NULL)
		{
			ModualParms|=32;
			strcat(String,"Needs Heartbeat\n");
		}
		if (ModuleAudioSample!=NULL)
		{
			ModualParms|=64;
			strcat(String,"Analog Audio Outputs\n");
		}
		if (PakMemWrite8!=NULL)
		{
			ModualParms|=128;
			strcat(String,"Needs ChipSelect Write\n");
		}
		if (PakMemRead8!=NULL)
		{
			ModualParms|=256;
			strcat(String,"Needs ChipSelect Read\n");
		}
		if (ModuleStatus!=NULL)
		{
			ModualParms|=512;
			strcat(String,"Returns Status\n");
		}
		if (ModuleReset!=NULL)
		{
			ModualParms|=1024;
			strcat(String,"Needs Reset Notification\n");
		}
		if (SetIniPath!=NULL)
		{
			ModualParms|=2048;
			GetIniFilePath(TempIni);
			//fprintf(stderr, "Insert Module : Calling SetIniPath %lx(%s)\n", (unsigned long)SetIniPath, TempIni);
			SetIniPath(TempIni);
		}
		if (PakSetCart!=NULL)
		{
			ModualParms|=4096;
			strcat(String,"Can Assert CART\n");
			PakSetCart(SetCart);
		}
		strcpy(DllPath,ModulePath);
		EmuState2.ResetPending=2;
		return(0);
		break;
	}
	return(NOMODULE);
}

/**
Load a ROM pack
return total bytes loaded, or 0 on failure
*/
int load_ext_rom(char filename[MAX_PATH])
{
	const size_t PAK_MAX_MEM = 0x40000;

	// If there is an existing ROM, ditch it
	if (ExternalRomBuffer != nullptr) {
		free(ExternalRomBuffer);
	}
	
	// Allocate memory for the ROM
	ExternalRomBuffer = (uint8_t*)malloc(PAK_MAX_MEM);

	// If memory was unable to be allocated, fail
	if (ExternalRomBuffer == nullptr) {
		_MessageBox("cant allocate ram");
		return 0;
	}
	
	// Open the ROM file, fail if unable to
	FILE *rom_handle = fopen(filename, "rb");
	if (rom_handle == nullptr) return 0;
	
	// Load the file, one byte at a time.. (TODO: Get size and read entire block)
	size_t index=0;
	while ((feof(rom_handle) == 0) && (index < PAK_MAX_MEM)) {
		ExternalRomBuffer[index++] = fgetc(rom_handle);
	}
	fclose(rom_handle);
	
	UnloadDll(1);
	BankedCartOffset=0;
	RomPackLoaded=true;
	
	return index;
}

void UnloadDll(short int config)
{
	if ((DialogOpen==true) & (EmuState2.EmulationRunning==1))
	{
		_MessageBox("Close Configuration Dialog before unloading");
		return;
	}
	
	if (config && ConfigModule != NULL) 
	{
		ConfigModule(0); // 0 = Release Resources (Menus etc)
	}
	ConfigModule=NULL;
	GetModuleName=NULL;
	ConfigModule=NULL;
	PakPortWrite=NULL;
	PakPortRead=NULL;
	SetInteruptCallPointer=NULL;
	DmaMemPointer=NULL;
	HeartBeat=NULL;
	PakMemWrite8=NULL;
	PakMemRead8=NULL;
	ModuleStatus=NULL;
	ModuleAudioSample=NULL;
	ModuleReset=NULL;
	if (hinstLib !=NULL)
		SDL_UnloadObject(hinstLib); 
	hinstLib=NULL;
	return;
}

void GetCurrentModule(char *DefaultModule)
{
	strcpy(DefaultModule,DllPath);
	return;
}

void UpdateBusPointer(void)
{
	if (SetInteruptCallPointer!=NULL)
		SetInteruptCallPointer(CPUAssertInterupt);
	return;
}

void UnloadPack(void)
{
	UnloadDll(1);
	strcpy(DllPath,"");
	strcpy(Modname,"");
	RomPackLoaded=false;
	SetCart(0);
	
	if (ExternalRomBuffer != nullptr) {
		free(ExternalRomBuffer);
	}
	ExternalRomBuffer=nullptr;

	EmuState2.ResetPending=2;
	UpdateCartridgeMenu(Modname);
	UpdateOnBoot(Modname);
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

void UpdateCartridgeMenu(char *modname)
{
	extern UpdateCartridgeEject(char*);

	UpdateCartridgeEject(modname);
}