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

#define MAX_PARAMS 3

unsigned char BaseAddr = 0x60;

enum Registers
{
	REG_Command = 0x00,
	REG_Param0H = 0x01,
	REG_Param0L = 0x02,
	REG_Param1H = 0x03,
	REG_Param1L = 0x04,
	REG_Param2H = 0x05,
	REG_Param2L = 0x06,
	REG_ParamCnt = 0x07
};

enum Commands
{
	CMD_Check,
	CMD_Test,
	CMD_MultDbl,
	CMD_DivDbl,
	CMD_AddDbl,
	CMD_SubDbl,
	CMD_NegDbl,
	CMD_PowDbl,
	CMD_SqrtDbl,
	CMD_ExpDbl,
	CMD_LogDbl,
	CMD_Log10Dbl
};

unsigned short int Params[MAX_PARAMS];

AG_MenuItem *menuAnchor = NULL;
// AG_MenuItem *itemMenu = NULL;
// AG_MenuItem *itemEjectHDD = NULL;
// AG_MenuItem *itemLoadHDD = NULL;
// AG_MenuItem *itemSeperator = NULL;

unsigned char ExecuteStatus = 0;

union Data
{
	unsigned long long llval;
	double dval;
	unsigned char bytes[8];
};

typedef union Data PDP1dbl;
typedef union Data IEEE754dbl;

PDP1dbl ReadPDP1dbl(unsigned short int address)
{
	PDP1dbl pdp1dbl;

	pdp1dbl.bytes[7] = MemRead(address);
	pdp1dbl.bytes[6] = MemRead(address+1);
	pdp1dbl.bytes[5] = MemRead(address+2);
	pdp1dbl.bytes[4] = MemRead(address+3);
	pdp1dbl.bytes[3] = MemRead(address+4);
	pdp1dbl.bytes[2] = MemRead(address+5);
	pdp1dbl.bytes[1] = MemRead(address+6);
	pdp1dbl.bytes[0] = MemRead(address+7);

	return pdp1dbl;
}

void WritePDP1dbl(unsigned short int address, PDP1dbl pdp1dbl)
{
	MemWrite8(pdp1dbl.bytes[7], address);
	MemWrite8(pdp1dbl.bytes[6], address+1);
	MemWrite8(pdp1dbl.bytes[5], address+2);
	MemWrite8(pdp1dbl.bytes[4], address+3);
	MemWrite8(pdp1dbl.bytes[3], address+4);
	MemWrite8(pdp1dbl.bytes[2], address+5);
	MemWrite8(pdp1dbl.bytes[1], address+6);
	MemWrite8(pdp1dbl.bytes[0], address+7);
}

double ConvertPDP1toIEE754(PDP1dbl pdp1dbl)
{
	IEEE754dbl iee754;
	unsigned long long signbit, exp, mantissa;

	signbit  =  pdp1dbl.llval & 0x8000000000000000;
	exp      = (pdp1dbl.llval & 0x00000000000000ff) + 0x37e;
	mantissa =  pdp1dbl.llval & 0x7fffffffffffff00;

	iee754.llval = signbit | (exp<<52) | (mantissa>>11);

	return iee754.dval;
}

PDP1dbl ConvertIEE754toPDP1(double dvalue)
{
	PDP1dbl pdp1dbl;
	IEEE754dbl ieee754dbl;
	unsigned long long signbit, exp, mantissa;

	ieee754dbl.dval = dvalue;

	signbit  =   ieee754dbl.llval & 0x8000000000000000;
	exp      = ((ieee754dbl.llval & 0x7ff0000000000000)>>52) - 0x37e;
	mantissa =   ieee754dbl.llval & 0x000fffffffffffff;

	pdp1dbl.llval = signbit | (exp) | (mantissa<<11);

	return pdp1dbl;
}

void DumpParams()
{
	for(int i = 0 ; i < MAX_PARAMS ; i++)
	{
		fprintf(stderr, "MPU : Param[%d] = %x\n", i, Params[i]);
	}
}

void CompareDbl()
{
	PDP1dbl pdp1dbl;
	double dvalue, dvalue1, dvalue2;
	char result = 0;

	pdp1dbl = ReadPDP1dbl(Params[1]);
	dvalue1 = ConvertPDP1toIEE754(pdp1dbl);

	pdp1dbl = ReadPDP1dbl(Params[2]);
	dvalue2 = ConvertPDP1toIEE754(pdp1dbl);

	dvalue = dvalue1 - dvalue2;

	if (dvalue < 0)
	{
		result = -1;
	}
	else if (dvalue > 0)
	{
		result = 1;
	}

	// fprintf(stderr, "MPU : CommpareDbl %d = %f - %f\n", result, dvalue1, dvalue2);

	MemWrite8(result, Params[0]);
}

void MultDbl()
{
	PDP1dbl pdp1dbl;
	double dvalue, dvalue1, dvalue2;

	pdp1dbl = ReadPDP1dbl(Params[1]);
	dvalue1 = ConvertPDP1toIEE754(pdp1dbl);

	pdp1dbl = ReadPDP1dbl(Params[2]);
	dvalue2 = ConvertPDP1toIEE754(pdp1dbl);

	dvalue = dvalue1 * dvalue2;

	// fprintf(stderr, "MPU : MultDbl %f = %f * %f\n", dvalue, dvalue1, dvalue2);

	pdp1dbl = ConvertIEE754toPDP1(dvalue);
	WritePDP1dbl(Params[0], pdp1dbl);
}

void DivDbl()
{
	PDP1dbl pdp1dbl;
	double dvalue, dvalue1, dvalue2;

	pdp1dbl = ReadPDP1dbl(Params[1]);
	dvalue1 = ConvertPDP1toIEE754(pdp1dbl);

	pdp1dbl = ReadPDP1dbl(Params[2]);
	dvalue2 = ConvertPDP1toIEE754(pdp1dbl);

	dvalue = dvalue1 / dvalue2;

	// fprintf(stderr, "MPU : DivDbl %f = %f / %f\n", dvalue, dvalue1, dvalue2);

	pdp1dbl = ConvertIEE754toPDP1(dvalue);
	WritePDP1dbl(Params[0], pdp1dbl);
}

void AddDbl()
{
	PDP1dbl pdp1dbl;
	double dvalue, dvalue1, dvalue2;

	pdp1dbl = ReadPDP1dbl(Params[1]);
	dvalue1 = ConvertPDP1toIEE754(pdp1dbl);

	pdp1dbl = ReadPDP1dbl(Params[2]);
	dvalue2 = ConvertPDP1toIEE754(pdp1dbl);

	dvalue = dvalue1 + dvalue2;

	// fprintf(stderr, "MPU : AddltDbl %f = %f + %f\n", dvalue, dvalue1, dvalue2);

	pdp1dbl = ConvertIEE754toPDP1(dvalue);
	WritePDP1dbl(Params[0], pdp1dbl);
}

void SubDbl()
{
	PDP1dbl pdp1dbl;
	double dvalue, dvalue1, dvalue2;

	pdp1dbl = ReadPDP1dbl(Params[1]);
	dvalue1 = ConvertPDP1toIEE754(pdp1dbl);

	pdp1dbl = ReadPDP1dbl(Params[2]);
	dvalue2 = ConvertPDP1toIEE754(pdp1dbl);

	dvalue = dvalue1 * dvalue2;

	// fprintf(stderr, "MPU : SubDbl %f = %f - %f\n", dvalue, dvalue1, dvalue2);

	pdp1dbl = ConvertIEE754toPDP1(dvalue);
	WritePDP1dbl(Params[0], pdp1dbl);
}

void NegDbl()
{
	PDP1dbl pdp1dbl;
	double dvalue, dvalue1, dvalue2;

	pdp1dbl = ReadPDP1dbl(Params[1]);
	dvalue1 = ConvertPDP1toIEE754(pdp1dbl);

	dvalue = -dvalue1;

	// fprintf(stderr, "MPU : NegDbl %f = %f\n", dvalue, dvalue1);

	pdp1dbl = ConvertIEE754toPDP1(dvalue);
	WritePDP1dbl(Params[0], pdp1dbl);
}

void PowDbl()
{
	PDP1dbl pdp1dbl;
	double dvalue, dvalue1, dvalue2;

	pdp1dbl = ReadPDP1dbl(Params[1]);
	dvalue1 = ConvertPDP1toIEE754(pdp1dbl);

	pdp1dbl = ReadPDP1dbl(Params[2]);
	dvalue2 = ConvertPDP1toIEE754(pdp1dbl);

	dvalue = pow(dvalue1, dvalue2);

	// fprintf(stderr, "MPU : PowDbl %f = pow(%f, %f)\n", dvalue, dvalue1, dvalue2);

	pdp1dbl = ConvertIEE754toPDP1(dvalue);
	WritePDP1dbl(Params[0], pdp1dbl);
}

void SqrtDbl()
{
	PDP1dbl pdp1dbl;
	double dvalue, dvalue1, dvalue2;

	pdp1dbl = ReadPDP1dbl(Params[1]);
	dvalue1 = ConvertPDP1toIEE754(pdp1dbl);

	dvalue = sqrt(dvalue1);

	// fprintf(stderr, "MPU : SqrtDbl %f = sqrt(%f)\n", dvalue, dvalue1);

	pdp1dbl = ConvertIEE754toPDP1(dvalue);
	WritePDP1dbl(Params[0], pdp1dbl);
}

void ExpDbl()
{
	PDP1dbl pdp1dbl;
	double dvalue, dvalue1, dvalue2;

	pdp1dbl = ReadPDP1dbl(Params[1]);
	dvalue1 = ConvertPDP1toIEE754(pdp1dbl);

	dvalue = exp(dvalue1);

	// fprintf(stderr, "MPU : ExpDbl %f = exp(%f)\n", dvalue, dvalue1);

	pdp1dbl = ConvertIEE754toPDP1(dvalue);
	WritePDP1dbl(Params[0], pdp1dbl);
}

void LogDbl()
{
	PDP1dbl pdp1dbl;
	double dvalue, dvalue1, dvalue2;

	pdp1dbl = ReadPDP1dbl(Params[1]);
	dvalue1 = ConvertPDP1toIEE754(pdp1dbl);

	dvalue = log(dvalue1);
	
	// fprintf(stderr, "MPU : LogDbl %f = log(%f)\n", dvalue, dvalue1);

	pdp1dbl = ConvertIEE754toPDP1(dvalue);
	WritePDP1dbl(Params[0], pdp1dbl);
}

void Log10Dbl()
{
	PDP1dbl pdp1dbl;
	double dvalue, dvalue1, dvalue2;

	pdp1dbl = ReadPDP1dbl(Params[1]);
	dvalue1 = ConvertPDP1toIEE754(pdp1dbl);

	dvalue = log10(dvalue1);
	
	// fprintf(stderr, "MPU : Log10Dbl %f = log10(%f)\n", dvalue, dvalue1);

	pdp1dbl = ConvertIEE754toPDP1(dvalue);
	WritePDP1dbl(Params[0], pdp1dbl);
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

		case CMD_MultDbl:
			MultDbl();
		break;

		case CMD_DivDbl:
			DivDbl();
		break;

		case CMD_AddDbl:
			AddDbl();
		break;

		case CMD_SubDbl:
			SubDbl();
		break;

		case CMD_NegDbl:
			NegDbl();
		break;

		case CMD_PowDbl:
			PowDbl();
		break;

		case CMD_SqrtDbl:
			SqrtDbl();
		break;

		case CMD_ExpDbl:
			ExpDbl();
		break;

		case CMD_LogDbl:
			LogDbl();
		break;

		case CMD_Log10Dbl:
			Log10Dbl();
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

	//fprintf(stderr, "MPU : ModuleName\n");

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
