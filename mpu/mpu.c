// /*
// Copyright 2019 by Walter Zambotti
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
// // 

typedef int BOOL;

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include "fileops.h"
#include "defines.h"
#include "mpu.h"

static char moduleName[4] = { "MPU" };

static char IniFile[MAX_PATH] = { 0 };

typedef void (*ASSERTINTERUPT) (unsigned char,unsigned char);
typedef void (*DMAMEMPOINTERS) (MEMREAD8, MEMWRITE8);
static void (*AssertInt)(unsigned char,unsigned char)=NULL;
static unsigned char (*MemRead8)(unsigned short)=NULL;
static void (*MemWrite8)(unsigned char,unsigned short)=NULL;
static void LoadConfig(void);
static void SaveConfig(void);
static void BuildMenu(void);
static void UpdateMenu(void);

#define MAX_PARAMS 4

unsigned char BaseAddr = 0x60;

enum Registers
{
	REG_Command,
	REG_Param0H,
	REG_Param0L,
	REG_Param1H,
	REG_Param1L,
	REG_Param2H,
	REG_Param2L,
	REG_Param3H,
	REG_Param3L,
	REG_ParamCnt
};

unsigned short int Params[MAX_PARAMS];

AG_MenuItem *menuAnchor = NULL;
// AG_MenuItem *itemMenu = NULL;
// AG_MenuItem *itemEjectHDD = NULL;
// AG_MenuItem *itemLoadHDD = NULL;
// AG_MenuItem *itemSeperator = NULL;

unsigned char ExecuteStatus = 0;

void DumpParams()
{
	for(int i = 0 ; i < MAX_PARAMS ; i++)
	{
		fprintf(stderr, "MPU : Param[%d] = %x\n", i, Params[i]);
	}
}

void ExecuteCommand(unsigned char cmd)
{
	ExecuteStatus = cmd;

	switch (cmd)
	{
		case CMD_Check:
			fprintf(stderr, "MPU Alive\n");
		break;

		case CMD_Test:
			DumpParams();
		break;

		case CMD_CompareDbl:
			CompareDbl(Params[0], Params[1], Params[2]);
		break;

		case CMD_MultDbl:
			MultDbl(Params[0], Params[1], Params[2]);
		break;

		case CMD_DivDbl:
			DivDbl(Params[0], Params[1], Params[2]);
		break;

		case CMD_AddDbl:
			AddDbl(Params[0], Params[1], Params[2]);
		break;

		case CMD_SubDbl:
			SubDbl(Params[0], Params[1], Params[2]);
		break;

		case CMD_NegDbl:
			NegDbl(Params[0], Params[1]);
		break;

		case CMD_PowDbl:
			PowDbl(Params[0], Params[1], Params[2]);
		break;

		case CMD_SqrtDbl:
			SqrtDbl(Params[0], Params[1]);
		break;

		case CMD_ExpDbl:
			ExpDbl(Params[0], Params[1]);
		break;

		case CMD_LogDbl:
			LogDbl(Params[0], Params[1]);
		break;

		case CMD_Log10Dbl:
			Log10Dbl(Params[0], Params[1]);
		break;

		case CMD_Inv:
			InvDbl(Params[0], Params[1]);
		break;

		case CMD_ltod:
			ltod(Params[0], Params[1]);
		break;

		case CMD_dtol:
			dtol(Params[0], Params[1]);
		break;

		case CMD_ftod:
			ftod(Params[0], Params[1]);
		break;

		case CMD_dtof:
			dtof(Params[0], Params[1]);
		break;

		case CMD_SetScreen:
			QueueGPUrequest(cmd, Params[0], Params[1], Params[2], Params[3]);
		break;

		case CMD_SetColor:
			QueueGPUrequest(cmd, Params[0], 0, 0, 0);
		break;

		case CMD_SetPixel:
			QueueGPUrequest(cmd, Params[0], Params[1], 0, 0);
		break;

		case CMD_DrawLine:
			QueueGPUrequest(cmd, Params[0], Params[1], Params[2], Params[3]);
		break;

		default:
			fprintf(stderr, "MPU : uknown command %d\n", cmd);
		break;
	}

	ExecuteStatus = 0;
}

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

	if (menuAnchor != NULL) 
	{
		BuildMenu();
	}

	// fprintf(stderr, "MPU : ModuleName\n");

	StartGPUQueue();

	return ;
}

void ADDCALL ModuleConfig(unsigned char func)
{
	switch (func)
	{
	case 0: // Destroy Menus
		// AG_MenuDel(itemEjectHDD);
		// AG_MenuDel(itemLoadHDD);
		// AG_MenuDel(itemMenu);
		// AG_MenuDel(itemSeperator);
		StopGPUqueue();
		break;

	case 1: // Update ini file
		strcpy(IniFile, iniman->files[iniman->lastfile].name);

	break;

	default:
		break;
	}

	//fprintf(stderr, "MPU : ModuleConfig\n");
	return;
}

void ADDCALL PackPortWrite(unsigned char Port, unsigned char Data)
{
	switch (Port-BaseAddr)
	{
		case REG_Command:
			ExecuteCommand(Data);
		break;

		case REG_Param0H:
		case REG_Param0L:
		case REG_Param1H:
		case REG_Param1L:
		case REG_Param2H:
		case REG_Param2L:
		case REG_Param3H:
		case REG_Param3L:
		{
			int idx = (Port-BaseAddr-1)^1;
			*((unsigned char*)Params+(idx)) = Data;
			//fprintf(stderr, "MPU : Params[%d] = %x\n", idx, Params[idx]);
		}
		break;
	}

	return;
}

// unsigned char ADDCALL PackPortRead(unsigned char Port)
// {
// 	if (Port == BaseAddr)
// 	{
// 		return ExecuteStatus;
// 	}
// }

// //This captures the pointers to the MemRead8 and MemWrite8 functions. This allows the DLL to do DMA xfers with CPU ram.

void ADDCALL MemPointers(MEMREAD8 Temp1, MEMWRITE8 Temp2)
{
	MemRead8=Temp1;
	MemWrite8=Temp2;
	//fprintf(stderr, "MPU : MemPointers\n");
	return;
}

void ADDCALL SetIniPath(INIman *InimanP)
{
	//strcpy(IniFile,IniFilePath);
	strcpy(IniFile, InimanP->files[InimanP->lastfile].name);
	InitPrivateProfile(InimanP);
	iniman = InimanP;
	LoadConfig();
	return;
}

static void LoadConfig(void)
{
	BaseAddr = GetPrivateProfileInt("MPU", "BaseAddress", 0x60, IniFile);
	//fprintf(stderr, "MPU : loadConfig : BaseAddr %s %d\n", IniFile, BaseAddr);

	return;
}

static void SaveConfig(void)
{
	//fprintf(stderr, "MPU : SaveConfig : BaseAddr %s %d\n", IniFile, BaseAddr);
	WritePrivateProfileInt("MPU", "BaseAddress", BaseAddr, IniFile);
	return;
}

static void BuildMenu(void)
{
	// if (itemMenu == NULL)
	// {
	// 	itemSeperator = AG_MenuSeparator(menuAnchor);
	//     itemMenu = AG_MenuNode(menuAnchor, "Hard Disk", NULL);
    //     {
    //         itemLoadHDD = AG_MenuAction(itemMenu, "Insert Disk", NULL, LoadHardDisk, NULL);
    //         itemEjectHDD = AG_MenuAction(itemMenu, 
    //             "Eject :                            ", 
    //             NULL, UnloadHardDisk, NULL);
	// 	}
	// }
}

static void UpdateMenu(void)
{
	// char hddname[MAX_PATH];

	// AG_Strlcpy(hddname, HDDfilename, sizeof(hddname));
	// PathStripPath(hddname);

	// AG_MenuSetLabel(itemEjectHDD, "Eject : %s", hddname);
}
