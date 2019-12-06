#ifndef __TCC1014MMU_H__
#define __TCC1014MMU_H__

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

typedef unsigned char  UINT8,  *PUINT8;
typedef unsigned short UINT16, *PUINT16;
typedef unsigned int   UINT32, *PUINT32;
typedef unsigned long  UINT64, *PUINT64;

extern UINT8 (*MemRead8)(UINT16);
extern void (*MemWrite8)(UINT8, UINT16);

extern UINT8 (*MmuRead8)(UINT8, UINT16);
extern void (*MmuWrite8)(UINT8, UINT8, UINT16);

extern UINT16 MemRead16(UINT16);
extern void MemWrite16(UINT16, UINT16);
extern UINT32 MemRead32(UINT16);
extern void MemWrite32(UINT32, UINT16);

extern unsigned char *GetPakExtMem();
PUINT8 (*MmuInit)(UINT8);
extern PUINT8 (*Getint_rom_pointer)(void);
extern void (*SetMapType)(UINT8);
extern void (*CopyRom)(void);
extern void (*Set_MmuTask)(UINT8);
extern void (*SetMmuRegister)(UINT8, UINT8);
extern void (*Set_MmuEnabled)(UINT8);
extern void (*SetRomMap)(UINT8);
extern void (*SetVectors)(UINT8);
extern void (*MmuReset)(void);
extern void (*SetDistoRamBank)(UINT8);
extern void SetMMUStat(unsigned char);

extern void SetHWMmu();
extern void SetSWMmu();

static void SetMmuPrefix(UINT8);
static void freePhysicalMemPages();
static int load_int_rom(char *);

#define _128K	0	
#define _512K	1
#define _2M		2

#endif
