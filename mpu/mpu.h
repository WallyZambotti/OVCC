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

void ADDCALL ModuleName(char *, AG_MenuItem *);
void ADDCALL ModuleConfig(unsigned char);
void ADDCALL PackPortWrite(unsigned char, unsigned char);
unsigned char ADDCALL PackPortRead(unsigned char);
typedef unsigned char (*MEMREAD8)(unsigned short);
typedef void (*MEMWRITE8)(unsigned char,unsigned short);
void ADDCALL MemPointers(MEMREAD8, MEMWRITE8);
unsigned char ADDCALL PakMemRead8 (unsigned short);
void ADDCALL ModuleStatus (char *);
//void ADDCALL SetIniPath (char *);

#ifdef __cplusplus
} // __cplusplus defined.
#endif

void MemWrite(unsigned char,unsigned short );
unsigned char MemRead(unsigned short );

void CompareDbl(unsigned short, unsigned short, unsigned short);
void MultDbl(unsigned short, unsigned short, unsigned short);
void DivDbl(unsigned short, unsigned short, unsigned short);
void AddDbl(unsigned short, unsigned short, unsigned short);
void SubDbl(unsigned short, unsigned short, unsigned short);
void NegDbl(unsigned short, unsigned short);
void PowDbl(unsigned short, unsigned short, unsigned short);
void SqrtDbl(unsigned short, unsigned short);
void ExpDbl(unsigned short, unsigned short);
void LogDbl(unsigned short, unsigned short);
void Log10Dbl(unsigned short, unsigned short);
void InvDbl(unsigned short, unsigned short);
void ltod(unsigned short, unsigned short);
void dtol(unsigned short, unsigned short);
void ftod(unsigned short, unsigned short);
void dtof(unsigned short, unsigned short);

void SetScreen(unsigned short, unsigned short, unsigned short, unsigned short);
void SetColor(unsigned short );
void SetPixel(unsigned short, unsigned short);
void DrawLine(unsigned short, unsigned short, unsigned short, unsigned short);


#endif
