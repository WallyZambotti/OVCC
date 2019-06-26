#ifndef __MPU_H__
#define __MPU_H__
/*
Copyright 2019 by Walter
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

#include <agar/core.h>
#include <agar/gui.h>
#include <stdbool.h>
#include "../CoCo/iniman.h"

#ifdef __MINGW32__
#define ADDCALL __cdecl
#else
#define ADDCALL
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef GPU_MODE_QUEUE
#define GPU_MODE_QUEUE
#endif

typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned short* pushort;
typedef unsigned char* puchar;

void ADDCALL ModuleName(char *, AG_MenuItem *);
void ADDCALL ModuleConfig(uchar);
void ADDCALL PackPortWrite(uchar, uchar);
unsigned char ADDCALL PackPortRead(uchar);
typedef unsigned char (*MEMREAD8)(ushort);
typedef void (*MEMWRITE8)(uchar, ushort);
typedef unsigned char (*MMUREAD8)(uchar, ushort);
typedef void (*MMUWRITE8)(uchar, uchar, ushort);
void ADDCALL MemPointers(MEMREAD8, MEMWRITE8);
void ADDCALL MmuPointers(MMUREAD8, MMUWRITE8);
unsigned char ADDCALL PakMemRead8(ushort);
void ADDCALL ModuleStatus (char *);
//void ADDCALL SetIniPath (char *);
unsigned char MemRead(ushort);
void MemWrite(uchar, ushort);
void MmuWrite(uchar, uchar, ushort);
unsigned char MmuRead(uchar, ushort);

#ifdef __cplusplus
} // __cplusplus defined.
#endif

enum Commands
{
	CMD_Check,
	CMD_Test,
	CMD_CompareDbl,
	CMD_MultDbl,
	CMD_DivDbl,
	CMD_AddDbl,
	CMD_SubDbl,
	CMD_NegDbl,
	CMD_PowDbl,
	CMD_SqrtDbl,
	CMD_ExpDbl,
	CMD_LogDbl,
	CMD_Log10Dbl,
	CMD_InvDbl,
	CMD_SinDbl,
	CMD_CosDbl,
	CMD_ltod,
	CMD_dtol,
	CMD_ftod,
	CMD_dtof,
	CMD_GetQueueLen = 62,
	CMD_GetTicks = 63,
	CMD_NewScreen = 64,
	CMD_DestroyScreen,
	CMD_SetColor,
	CMD_SetPixel,
	CMD_DrawLine,
	CMD_NewTexture,
	CMD_DestroyTexture,
	CMD_SetTextureTransparency,
	CMD_LoadTexture,
	CMD_RenderTexture
};


#endif
