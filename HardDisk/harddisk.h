#ifndef __HARDDISK_H__
#define __HARDDISK_H__
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

#include <agar/core.h>
#include <agar/gui.h>

#ifdef __MINGW32__
#define ADDCALL __cdecl
#else
#define ADDCALL
#endif

void MemWrite(unsigned char,unsigned short );
unsigned char MemRead(unsigned short );

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
void ADDCALL SetIniPath (char *);

#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif
