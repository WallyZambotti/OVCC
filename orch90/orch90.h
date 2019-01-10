#ifndef __ORCH90_H__
#define __ORCH90_H__
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

#ifdef __MINGW32__
#define ADDCALL __cdecl
#else
#define ADDCALL
#endif

typedef void (*SETCART)(unsigned char);

void MemWrite(unsigned char,unsigned short );
unsigned char MemRead(unsigned short );
void CPUAssertInterupt(unsigned char,unsigned char);


//Misc
#define MAX_LOADSTRING 100
#define QUERY 255
#define MAX_PATH 260

//Common CPU defs
#define IRQ		1
#define FIRQ	2
#define NMI		3

#ifdef __cplusplus
extern "C"
{
#endif

void ADDCALL ModuleName(char *, void *);
void ADDCALL PackPortWrite(unsigned char,unsigned char);
unsigned char ADDCALL PackPortRead(unsigned char);
unsigned char ADDCALL PackPortRead(unsigned char);
unsigned char ADDCALL SetCart(SETCART);
unsigned char ADDCALL PakMemRead8(unsigned short );
unsigned short ADDCALL ModuleAudioSample(void);

#ifdef __cplusplus
} // __cplusplus defined.
#endif
#endif
