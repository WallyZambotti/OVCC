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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include "fileops.h"
#include "defines.h"
#include "mpu.h"
#include "fpu.h"
#include "gpu.h"
#include "gpuclock.h"
#include "gpuprimitives.h"
#include "gputextures.h"

#define GPU_Nil_Arg ((unsigned short)0)

static char moduleName[4] = { "MPU" };

static char IniFile[MAX_PATH] = { 0 };

typedef void (*ASSERTINTERUPT) (unsigned char,unsigned char);
typedef void (*DMAMEMPOINTERS) (MEMREAD8, MEMWRITE8);
typedef void (*MMUMEMPOINTERS) (MMUREAD8, MMUWRITE8);
static void (*AssertInt)(unsigned char,unsigned char)=NULL;
static unsigned char (*MemRead8)(unsigned short)=NULL;
static void (*MemWrite8)(unsigned char,unsigned short)=NULL;
static unsigned char (*MmuRead8)(unsigned char, unsigned short)=NULL;
static void (*MmuWrite8)(unsigned char,unsigned char, unsigned short)=NULL;
static void LoadConfig(void);
static void SaveConfig(void);
static void BuildMenu(void);
static void UpdateMenu(void);

#define MAX_PARAMS 5

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
	REG_Param4H,
	REG_Param4L,
	REG_Param5H,
	REG_Param5L,
	REG_Param6H,
	REG_Param6L,
	REG_Param7H,
	REG_Param7L,
	REG_ParamCnt
};

unsigned short int Params[MAX_PARAMS];

#ifdef GPU_MODE_QUEUE
#define GPU_MODE_QUEUE
#endif

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

		case CMD_InvDbl:
			InvDbl(Params[0], Params[1]);
		break;

		case CMD_SinDbl:
			SinDbl(Params[0], Params[1]);
		break;

		case CMD_CosDbl:
			CosDbl(Params[0], Params[1]);
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

		case CMD_GetQueueLen:
			GetQueueLen(Params[0]);
		break;

		case CMD_GetTicks:
			GetHighResTicks(Params[0]);
		break;

		case CMD_NewScreen:
			NewScreen(Params[0], Params[1], Params[2], Params[3], Params[4]);
		break;

		case CMD_DestroyScreen:
#ifdef GPU_MODE_QUEUE
			QueueGPUrequest(cmd, Params[0]);
#else			
			DestroyScreen(Params[0]);
#endif
		break;

		case CMD_SetColor:
#ifdef GPU_MODE_QUEUE
			QueueGPUrequest(cmd, Params[0], Params[1]);
#else			
			SetColor(Params[0], Params[1]);
#endif
		break;

		case CMD_SetPixel:
#ifdef GPU_MODE_QUEUE
			QueueGPUrequest(cmd, Params[0], Params[1], Params[2]);
#else			
			SetPixel(Params[0], Params[1], Params[2]);
#endif
		break;

		case CMD_DrawLine:
#ifdef GPU_MODE_QUEUE
				QueueGPUrequest(cmd, Params[0], Params[1], Params[2], Params[3], Params[4]);
#else			
				DrawLine(Params[0], Params[1], Params[2], Params[3]), Params[4];
#endif
		break;

		case CMD_NewTexture:
			NewTexture(Params[0], Params[1], Params[2], Params[3]);
		break;

		case CMD_DestroyTexture:
#ifdef GPU_MODE_QUEUE
			QueueGPUrequest(cmd, Params[0]);
#else			
			DestroyTexture(Params[0]);
#endif
		break;

		case CMD_SetTextureTransparency:
#ifdef GPU_MODE_QUEUE
			QueueGPUrequest(cmd, Params[0], Params[1], Params[2]);
#else			
			SetTextureTransparency(Params[0], Params[1], Params[2]);
#endif
		break;

		case CMD_LoadTexture:
#ifdef GPU_MODE_QUEUE
			QueueGPUrequest(cmd, Params[0], Params[1], Params[2]);
#else			
			LoadTexture(Params[0], Params[1], Params[2]);
#endif
		break;

		case CMD_RenderTexture:
			RenderTexture(Params[0], Params[1], Params[2], Params[3], Params[4]);
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

void MmuWrite(unsigned char Data, unsigned char Bank, unsigned short Address)
{
	MmuWrite8(Data,Bank,Address);
	return;
}

unsigned char MmuRead(unsigned char Bank, unsigned short Address)
{
	return(MmuRead8(Bank,Address));
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

#ifdef GPU_MODE_QUEUE
	StartGPUQueue();
#endif
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
#ifdef GPU_MODE_QUEUE
		StopGPUqueue();
#endif
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
		case REG_Param4H:
		case REG_Param4L:
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

// This captures the pointers to the MemRead8 and MemWrite8 functions. This allows the DLL to do DMA xfers with CPU ram.

void ADDCALL MemPointers(MEMREAD8 Temp1, MEMWRITE8 Temp2)
{
	MemRead8=Temp1;
	MemWrite8=Temp2;
	// fprintf(stderr, "MPU : MemPointers\n");
	return;
}

// This captures the pointers to the MemRead8 and MemWrite8 functions. This allows the DLL to do DMA xfers with MMU ram.

void ADDCALL MmuPointers(MMUREAD8 Temp1, MMUWRITE8 Temp2)
{
	MmuRead8=Temp1;
	MmuWrite8=Temp2;
	// fprintf(stderr, "MPU : MmuPointers\n");
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
